#include "ONNX/HMIFaceDetector_FER.h"
#include "HMIProcessorImpl.h"
#include "HMISubsystemStatics.h"

#if HMI_WITH_FER
	#include <memory>
	#include <string>
	#include <vector>
	#include <array>

	#include "HMIThirdPartyBegin.h"
	#include "onnxruntime_cxx_api.h"
	#include "HMIThirdPartyEnd.h"

	#include "OpenCV/OpenCVIncludesBegin.h"
	#include "opencv2/opencv.hpp"
	#include "opencv2/dnn.hpp"
	#include "OpenCV/OpenCVIncludesEnd.h"
#endif

HMI_PROC_DECLARE_STATS(FaceDetector_FER)

#define LOGPREFIX "[FaceDetector_FER] "

const FName UHMIFaceDetector_FER::ImplName = TEXT("FaceDetector_FER");

class UHMIFaceDetector* UHMIFaceDetector_FER::GetOrCreateFaceDetector_FER(UObject* WorldContextObject, 
	FName Name,
	int CaptureDeviceId,
	int CaptureBackendId,
	float MinConfidence
)
{
	UHMIFaceDetector* Proc = UHMISubsystemStatics::GetOrCreateFaceDetector(WorldContextObject, ImplName, Name);

	Proc->SetProcessorParam("CaptureDeviceId", CaptureDeviceId);
	Proc->SetProcessorParam("CaptureBackendId", CaptureBackendId);
	Proc->SetProcessorParam("MinConfidence", MinConfidence);

	return Proc;
}

UHMIFaceDetector_FER::UHMIFaceDetector_FER(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ProviderName = ImplName;
	ProcessorName = ImplName.ToString();

	SetProcessorParam("CaptureDeviceId", 0);
	SetProcessorParam("CaptureBackendId", 0); // cv::VideoCaptureAPIs::CAP_ANY=0
	SetProcessorParam("MinConfidence", 0.6f);

	ExpressionMapping = NewObject<UHMIIndexMapping>();

	#if HMI_WITH_FER
		#define ADD_MAPPING(Id, Name) ExpressionMapping->HMI_AddMapping(Id, #Name)
		ADD_MAPPING(0, neutral);
		ADD_MAPPING(1, happiness);
		ADD_MAPPING(2, surprise);
		ADD_MAPPING(3, sadness);
		ADD_MAPPING(4, anger);
		ADD_MAPPING(5, disgust);
		ADD_MAPPING(6, fear);
		ADD_MAPPING(7, contempt);
		#undef ADD_MAPPING
	#endif
}

UHMIFaceDetector_FER::UHMIFaceDetector_FER(FVTableHelper& Helper) : Super(Helper)
{
}

UHMIFaceDetector_FER::~UHMIFaceDetector_FER()
{
}

#if HMI_WITH_FER

// TODO: rewrite this mess

class FFERImpl
{
	public:

	UHMIFaceDetector_FER* context = nullptr;

	int cap_device = 0;
	int cap_api = cv::VideoCaptureAPIs::CAP_ANY;
	bool dbg_disable_camera = false;

	std::string dnn_proto;
	std::string dnn_model;
	std::wstring fer_model;

	std::unique_ptr<cv::dnn::Net> face_net;
	std::unique_ptr<Ort::Env> env;
	std::unique_ptr<Ort::Session> session;
	std::string input_name_str;
	std::string output_name_str;
	std::vector<const char*> input_names;
	std::vector<const char*> output_names;

	std::unique_ptr<cv::VideoCapture> capture;
	std::unique_ptr<cv::Mat> frame;
	volatile int cancel_flag = false;

	mutable FCriticalSection faces_mux;
	TArray<FHMIFaceData> faces_priv;
	TArray<FHMIFaceData> faces_out;
	uint32_t faces_hash = 0;
	void GetFaces(TArray<FHMIFaceData>& Faces) const;

	FFERImpl(UHMIFaceDetector_FER* InContext) : context(InContext) {}
	~FFERImpl() { Release(); }

	bool Init();
	void Release();
	bool Update(float min_confidence);
	void Cancel() { cancel_flag = true; }
};

bool UHMIFaceDetector_FER::Proc_Init()
{
	if (!Super::Proc_Init())
		return false;

	FString DnnProto = UHMIStatics::LocateDataFile(TEXT("deploy.prototxt"));
	FString DnnModel = UHMIStatics::LocateDataFile(TEXT("res10_300x300_ssd_iter_140000.caffemodel"));
	FString FerModel = UHMIStatics::LocateDataFile(TEXT("emotion-ferplus-8.onnx"));

	if (!FPaths::FileExists(DnnProto))
	{
		ProcError(FString::Printf(TEXT("Not found: %s"), *DnnProto));
		return false;
	}
	if (!FPaths::FileExists(DnnModel))
	{
		ProcError(FString::Printf(TEXT("Not found: %s"), *DnnModel));
		return false;
	}
	if (!FPaths::FileExists(FerModel))
	{
		ProcError(FString::Printf(TEXT("Not found: %s"), *FerModel));
		return false;
	}

	auto TmpImpl = MakeUnique<FFERImpl>(this);

	TmpImpl->dnn_proto.assign(TCHAR_TO_UTF8(*DnnProto));
	TmpImpl->dnn_model.assign(TCHAR_TO_UTF8(*DnnModel));
	TmpImpl->fer_model.assign(*FerModel);

	TmpImpl->cap_device = GetProcessorInt("CaptureDeviceId");
	TmpImpl->cap_api = GetProcessorInt("CaptureBackendId");

	if (!TmpImpl->Init())
	{
		return false;
	}

	Impl = MoveTemp(TmpImpl);
	return true;
}

bool FFERImpl::Init()
{
	face_net = std::make_unique<cv::dnn::Net>();
	*face_net = cv::dnn::readNetFromCaffe(dnn_proto, dnn_model);
	if (face_net->empty())
	{
		context->ProcError(TEXT("Invalid DNN model"));
		return false;
	}

	env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_ERROR);

	Ort::SessionOptions opt;
	opt.SetIntraOpNumThreads(4);
	opt.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

	session = std::make_unique<Ort::Session>(*env, fer_model.c_str(), opt);
	size_t num_inputs = session->GetInputCount();
	if (num_inputs == 0)
	{
		context->ProcError(TEXT("Invalid ONNX model"));
		return false;
	}

	Ort::AllocatorWithDefaultOptions allocator;

	auto input_name = session->GetInputNameAllocated(0, allocator);
	input_name_str.assign(input_name.get());
	input_names.push_back(input_name_str.c_str());

	auto output_name = session->GetOutputNameAllocated(0, allocator);
	output_name_str.assign(output_name.get());
	output_names.push_back(output_name_str.c_str());

	frame = std::make_unique<cv::Mat>();

	if (dbg_disable_camera)
	{
		*frame = cv::imread(TCHAR_TO_UTF8(*UHMIStatics::LocateDataFile(TEXT("faces.png"))), cv::IMREAD_COLOR_BGR);
	}
	else
	{
		UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "Init VideoCapture"));

		#if PLATFORM_WINDOWS
		if (!cap_api)
			cap_api = cv::CAP_DSHOW; // works better on windows
		#endif

		capture = std::make_unique<cv::VideoCapture>(cap_device, cap_api);
		if (!capture->isOpened())
		{
			context->ProcError(TEXT("Failed to init video capture"));
			return false;
		}

		UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "CAP_PROP_BACKEND %d"), (int)capture->get(cv::CAP_PROP_BACKEND));
		UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "CAP_PROP_FRAME_WIDTH %d"), (int)capture->get(cv::CAP_PROP_FRAME_WIDTH));
		UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "CAP_PROP_FRAME_HEIGHT %d"), (int)capture->get(cv::CAP_PROP_FRAME_HEIGHT));
		UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "CAP_PROP_FPS %.2f"), capture->get(cv::CAP_PROP_FPS));
		UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "CAP_PROP_FORMAT %d"), (int)capture->get(cv::CAP_PROP_FORMAT));
		UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "CAP_PROP_FOURCC %d"), (int)capture->get(cv::CAP_PROP_FOURCC));
	}

	return true;
}

void FFERImpl::Release()
{
	UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "FFERImpl::Release"));

	capture.reset();
	frame.reset();
	session.reset();
	env.reset();
	face_net.reset();

	UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "FFERImpl::Release DONE"));
}

void UHMIFaceDetector_FER::Proc_Release()
{
	Impl.Reset();

	Super::Proc_Release();
}

void UHMIFaceDetector_FER::DetectOnce()
{
	StartOrWakeProcessor();
}

void UHMIFaceDetector_FER::DetectRealtime(bool Enable)
{
	IsRealtime = Enable;
	StartOrWakeProcessor();
}

void UHMIFaceDetector_FER::CancelOperation(bool PurgeQueue)
{
	Super::CancelOperation(PurgeQueue);

	if (Impl)
		Impl->Cancel();
}

bool UHMIFaceDetector_FER::Proc_DoWork(int& QueueLength)
{
	HMI_PROC_PRE_WORK_STATS(FaceDetector_FER)

	const float MinConfidence = FMath::Clamp(GetProcessorFloat("MinConfidence"), 0.0f, 1.0f);

	if (Impl->Update(MinConfidence))
	{
		AsyncTask(ENamedThreads::GameThread, [this]()
		{
			OnFaceDetectComplete.Broadcast();
		});
	}

	QueueLength = IsRealtime ? 1 : 0;
	return true;
}

void UHMIFaceDetector_FER::Proc_PostWorkStats()
{
	HMI_PROC_POST_WORK_STATS(FaceDetector_FER)
}

static std::vector<float> softmax(const std::vector<float>& logits)
{
	std::vector<float> exps(logits.size());
	float maxv = *std::max_element(logits.begin(), logits.end());
	double sum = 0.0;
	for (size_t i = 0; i < logits.size(); ++i) {
		exps[i] = std::exp((double)logits[i] - maxv);
		sum += exps[i];
	}
	for (float& v : exps) {
		v = (float)(v / sum);
	}
	return std::move(exps);
}

static std::vector<float> preprocess_face(const cv::Mat& face_gray) // TODO: hardcoded
{
	cv::Mat resized;
	cv::resize(face_gray, resized, cv::Size(64, 64), 0, 0, cv::INTER_AREA);
	// Convert to float but keep 0..255 range as many FER+ consumers expect that.
	resized.convertTo(resized, CV_32F);
	std::vector<float> tensor(1 * 1 * 64 * 64);
	// NCHW order: batch=1, channel=1, H, W
	for (int y = 0; y < 64; ++y) {
		for (int x = 0; x < 64; ++x) {
			tensor[y * 64 + x] = resized.at<float>(y, x);
		}
	}
	return std::move(tensor);
}

inline void HashBytes(uint32_t& hash, const void* ptr, size_t len)
{
	const uint8_t* p = (const uint8_t*)ptr;
	for (size_t i = 0; i < len; i++)
		hash = (hash ^ p[i]) * 16777619u;
};

bool FFERImpl::Update(float min_confidence)
{
	cancel_flag = false;

	faces_priv.Reset();

	auto& frame_ref = *frame;

	if (!dbg_disable_camera)
	{
		*capture >> frame_ref;
	}

	if (cancel_flag || frame_ref.empty()) 
		return false;

	const int frame_height = frame_ref.rows;
	const int frame_width = frame_ref.cols;

	cv::Mat blob = cv::dnn::blobFromImage(frame_ref, 1.0, cv::Size(300, 300), cv::Scalar(104.0, 177.0, 123.0), false, false); // TODO: hardcoded
	if (cancel_flag) 
		return false;

	face_net->setInput(blob);
	cv::Mat detections = face_net->forward(); // [1, 1, num_detections, 7]
	cv::Mat detection_mat(detections.size[2], detections.size[3], CV_32F, detections.ptr<float>()); // TODO: hardcoded
	if (cancel_flag) 
		return false;

	/*
		detection_mat.at<float>(face_id, N)
		0	image_id
		1	class_id
		2	confidence
		3	x_min
		4	y_min
		5	x_max
		6	y_max
	*/

	int num_faces = 0;
	for (int det_id = 0; det_id < detection_mat.rows; det_id++)
	{
		const float confidence = detection_mat.at<float>(det_id, 2);
		if (confidence < min_confidence) 
			continue;
		++num_faces;
	}

	if (faces_priv.Num() != num_faces)
	{
		faces_priv.SetNumZeroed(num_faces, EAllowShrinking::No); // dont use SetNumUninitialized here!
	}

	int face_id = 0;
	for (int det_id = 0; det_id < detection_mat.rows; det_id++)
	{
		if (cancel_flag) 
			return false;

		const float confidence = detection_mat.at<float>(det_id, 2); // TODO: hardcoded
		if (confidence < min_confidence) 
			continue;

		const int x1 = static_cast<int>(detection_mat.at<float>(det_id, 3) * frame_width); // TODO: hardcoded
		const int y1 = static_cast<int>(detection_mat.at<float>(det_id, 4) * frame_height);
		const int x2 = static_cast<int>(detection_mat.at<float>(det_id, 5) * frame_width);
		const int y2 = static_cast<int>(detection_mat.at<float>(det_id, 6) * frame_height);

		cv::Rect face_rect(x1, y1, x2 - x1, y2 - y1);
		face_rect &= cv::Rect(0, 0, frame_width, frame_height);
		if (face_rect.width <= 0 || face_rect.height <= 0) 
			continue;

		cv::Mat face_gray;
		cv::cvtColor(frame_ref(face_rect), face_gray, cv::COLOR_BGR2GRAY);

		std::vector<float> input_data = preprocess_face(face_gray);
		if (cancel_flag) 
			return false;

		auto mem_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);
		const std::array<int64_t, 4> input_shape = { 1, 1, 64, 64 }; // TODO: hardcoded

		auto input_tensor = Ort::Value::CreateTensor<float>(
			mem_info, 
			input_data.data(), input_data.size(), 
			input_shape.data(), input_shape.size()
		);

		auto output_tensors = session->Run(
			Ort::RunOptions{ nullptr }, 
			input_names.data(), &input_tensor, 1, 
			output_names.data(), 1
		);

		if (cancel_flag) 
			return false;

		if (output_tensors.empty())
		{
			throw std::exception("empty output_tensors");
			return false;
		}

		const int model_output_count = 8; // TODO: hardcoded
		//const int n_out = output_tensors[0].GetTensorSizeInBytes() / sizeof(float);

		const auto& tensor = output_tensors[0].GetTensorTypeAndShapeInfo();
		const int n_out = (int)tensor.GetElementCount();

		if (n_out != model_output_count)
		{
			throw std::exception("invalid output_tensors");
			return false;
		}

		const float* output_data = output_tensors[0].GetTensorMutableData<float>();
		std::vector<float> logits(output_data, output_data + model_output_count);
		std::vector<float> probs = softmax(logits);

		const int best_idx = std::max_element(probs.begin(), probs.end()) - probs.begin();
		const float best_prob = probs[best_idx];

		check(probs.size() == model_output_count);

		#if 1
		{
			FHMIFaceData& face = faces_priv[face_id];

			face.Confidence = confidence;
			face.Rect = { face_rect.x, face_rect.y, face_rect.width, face_rect.height };

			face.ExpressionMapping = context->ExpressionMapping;

			if (face.Expressions.Num() != model_output_count)
			{
				face.Expressions.SetNumUninitialized(model_output_count, EAllowShrinking::No);
			}

			for (int id = 0; id < model_output_count; id++)
			{
				face.Expressions[id] = probs[id];
			}

			++face_id;
		}
		#endif
	}

	uint32_t hash = 2166136261u; // FNV-1a basis

	for (int id = 0; id < num_faces; id++)
	{
		const FHMIFaceData& face = faces_priv[id];

		HashBytes(hash, &face.Rect.X, sizeof(face.Rect));
		HashBytes(hash, face.Expressions.GetData(), face.Expressions.Num() * sizeof(float));
	}

	const bool faces_updated = (faces_hash != hash);
	faces_hash = hash;

	if (faces_updated)
	{
		faces_priv.Sort([](const FHMIFaceData& A, const FHMIFaceData& B){ return A.Confidence > B.Confidence; });

		{
			FScopeLock LOCK(&faces_mux);
			faces_out = faces_priv;
		}
	}

	return faces_updated;
}

void FFERImpl::GetFaces(TArray<FHMIFaceData>& Faces) const
{
	FScopeLock LOCK(&faces_mux);
	Faces = faces_out;
}

void UHMIFaceDetector_FER::GetFaces(UPARAM(ref) TArray<FHMIFaceData>& Faces)
{
	if (Impl)
		Impl->GetFaces(Faces);
}

#endif // HMI_WITH_FER

#undef LOGPREFIX
