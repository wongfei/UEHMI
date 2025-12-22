#include "GGML/HMIChat_Llama.h"
#include "HMIProcessorImpl.h"
#include "HMISubsystemStatics.h"

#if HMI_WITH_LLAMA
	#include <string>
	#include <vector>
	#include <functional>

	#include "HMIThirdPartyBegin.h"
	#include "llama.h"
	#include "HMIThirdPartyEnd.h"

	#if !PLATFORM_WINDOWS
		#define _strdup strdup
	#endif
#endif

HMI_PROC_DECLARE_STATS(Chat_Llama)

#define LOGPREFIX "[Chat_Llama] "

const FName UHMIChat_Llama::ImplName = TEXT("Chat_Llama");

class UHMIChat* UHMIChat_Llama::GetOrCreateChat_Llama(UObject* WorldContextObject, 
	FName Name,
	FString ModelName,
	int ContextSize,
	int NumThreads,
	int NumGpuLayers
)
{
	UHMIChat* Proc = UHMISubsystemStatics::GetOrCreateChat(WorldContextObject, ImplName, Name);

	Proc->SetProcessorParam("ModelName", ModelName);
	Proc->SetProcessorParam("NumThreads", NumThreads);

	Proc->SetProcessorParam("n_ctx", ContextSize);
	Proc->SetProcessorParam("n_gpu_layers", NumGpuLayers);

	return Proc;
}

UHMIChat_Llama::UHMIChat_Llama(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ProviderName = ImplName;
	ProcessorName = ImplName.ToString();

	SetProcessorParam("ModelName", TEXT("Llama-3.2_1b_Uncensored_RP_Aesir.gguf"));
	SetProcessorParam("NumThreads", 2); // 0 = default, -1 = max
	
	// llama specific
	SetProcessorParam("n_ctx", 8192); // 0 = from model
	SetProcessorParam("n_gpu_layers", 0); // 0 = gpu offload disabled, 999 = auto

	SetProcessorParam("penalty_last_n", 128); // last n tokens to penalize (0 = disable penalty, -1 = context size)
	SetProcessorParam("penalty_repeat", 1.15f); // 1.0 = disabled

	SetProcessorParam("top_k", 40); // 0..vocab_size, >500 usually pointless
	SetProcessorParam("min_p", 0.1f); // 0..1, 0 = disabled
}

UHMIChat_Llama::UHMIChat_Llama(FVTableHelper& Helper) : Super(Helper)
{
}

UHMIChat_Llama::~UHMIChat_Llama()
{
}

#if HMI_WITH_LLAMA

class FLlamaContext
{
	public:
	
	int32_t penalty_last_n = 128;
	float penalty_repeat = 1.15f;
	float penalty_freq = 0.0f;    // 0.0 = disabled
	float penalty_present = 0.0f; // 0.0 = disabled

	int top_k = 40;
	float top_p = 0.9f;
	float min_p = 0.1f;
	float temperature = 0.8f;
	int max_tokens = 0;

	UHMIChat_Llama* proc = nullptr;
	FString error_text;
	llama_model_params model_params{};
	llama_context_params ctx_params{};
	llama_model* model = nullptr;
	const llama_vocab* vocab = nullptr;
	llama_context* ctx = nullptr;
	llama_sampler* smpl = nullptr;
	std::vector<llama_chat_message> messages;
	std::vector<char> formatted;
	std::string response;
	std::string piece;
	std::atomic<bool> cancel_flag;
	double start_process_ts = 0;

	FLlamaContext(UHMIChat_Llama* in_proc) : proc(in_proc) {}
	~FLlamaContext() { Release(); }

	bool Init(std::string model_path);
	void Release();
	void ResetState();

	void AddMessage(const FString& Role, const FString& Text);
	bool ProcessMessages(std::function<void(const std::string&)> progress_func);
	bool Generate(std::string prompt, std::function<void(const std::string&)> progress_func);

	void Cancel() { cancel_flag = true; }
	bool GetCancelFlag() const { return cancel_flag.load(); }
};

bool UHMIChat_Llama::Proc_Init()
{
	if (!Super::Proc_Init())
		return false;

	ModelName = GetProcessorString("ModelName");
	HMI_GUARD_PARAM_NOT_EMPTY(ModelName);

	FString ModelPath = UHMIStatics::LocateDataFile(ModelName);
	if (!FPaths::FileExists(ModelPath))
	{
		ProcError(FString::Printf(TEXT("Model not found: %s"), *ModelPath));
		return false;
	}

	auto TmpContext = MakeUnique<FLlamaContext>(this);
	if (!TmpContext->Init(TCHAR_TO_UTF8(*ModelPath)))
		return false;

	Context = MoveTemp(TmpContext);
	return true;
}

void UHMIChat_Llama::Proc_Release()
{
	Context.Reset();

	Super::Proc_Release();
}

int64 UHMIChat_Llama::ProcessInput(FHMIChatInput&& Input)
{
	const int64 OperationId = EnqueOperation(InputQueue, MoveTemp(Input), GetWorldContext());
	StartOrWakeProcessor();
	return OperationId;
}

void UHMIChat_Llama::CancelOperation(bool PurgeQueue)
{
	Super::CancelOperation(PurgeQueue);

	if (Context)
		Context->Cancel();

	if (PurgeQueue)
		PurgeInputQueue(InputQueue);
}

bool UHMIChat_Llama::Proc_DoWork(int& QueueLength)
{
	FHMIChat_Llama_Operation Operation;

	if (!DequeOperation(InputQueue, Operation, QueueLength))
		return false;

	auto& InputText = Operation.Input.Text;

	if (InputText.IsEmpty())
	{
		HandleOperationComplete(false, FString(TEXT("Empty input")), MoveTemp(Operation.Input));
		return false;
	}

	HMI_PROC_PRE_WORK_STATS(Chat_Llama)

	UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "<<< \"%s\""), *InputText);

	ResetChatOperation();
	Context->ResetState();

	Context->max_tokens = FMath::Max(Operation.Input.MaxTokens, 0);
	Context->temperature = FMath::Clamp(Operation.Input.Temperature, 0.0f, 2.0f);
	Context->top_p = FMath::Clamp(Operation.Input.TopP, 0.0f, 1.0f);

	Context->penalty_last_n = FMath::Max(GetProcessorInt("penalty_last_n"), -1);
	Context->penalty_repeat = FMath::Clamp(GetProcessorFloat("penalty_repeat"), 1.0f, 2.0f);

	Context->top_k = FMath::Max(GetProcessorInt("top_k"), 0);
	Context->min_p = FMath::Clamp(GetProcessorFloat("min_p"), 0.0f, 1.0f);

	for (const auto& Msg : Operation.Input.History)
	{
		Context->AddMessage(Msg.Role, Msg.Text);
	}
	Context->AddMessage(TEXT("user"), InputText);

	const bool Success = Context->ProcessMessages([this, &Operation](const std::string& chunk)
	{
		HandleDeltaContent(Operation.Input, FString(UTF8_TO_TCHAR(chunk.c_str())));
	});

	Context->ResetState();

	if (!Context->GetCancelFlag())
	{
		HandleOperationComplete(Success, MoveTemp(Context->error_text), MoveTemp(Operation.Input));
	}

	return Success;
}

void UHMIChat_Llama::Proc_PostWorkStats()
{
	HMI_PROC_POST_WORK_STATS(Chat_Llama)
}

//
// FLlamaContext
//

DEFINE_LOG_CATEGORY_STATIC(LogLlama, Warning, All);

static void LlamaLogCallback(enum ggml_log_level level, const char* text, void* user_data)
{
	#define LLAMA_LOG(Verbosity) UE_LOG(LogLlama, Verbosity, TEXT(LOGPREFIX "%s"), UTF8_TO_TCHAR(text))
	switch (level)
	{
		case GGML_LOG_LEVEL_DEBUG: LLAMA_LOG(VeryVerbose); break;
		case GGML_LOG_LEVEL_INFO:  LLAMA_LOG(Verbose); break;
		case GGML_LOG_LEVEL_WARN:  LLAMA_LOG(Warning); break;
		case GGML_LOG_LEVEL_ERROR: LLAMA_LOG(Error); break;
	}
	#undef LLAMA_LOG
}

bool FLlamaContext::Init(std::string model_path)
{
	llama_log_set(&LlamaLogCallback, nullptr);

	// model

	model_params = llama_model_default_params();
	model_params.n_gpu_layers = FMath::Max(proc->GetProcessorInt("n_gpu_layers"), 0);

	UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "llama_model_load_from_file"));
	model = llama_model_load_from_file(model_path.c_str(), model_params);
	if (!model) {
		proc->ProcError(TEXT("llama_model_load_from_file"));
		return false;
	}

	int n_ctx_train = llama_model_n_ctx_train(model);
	UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "n_ctx_train=%d"), n_ctx_train);
	if (n_ctx_train <= 0) {
		proc->ProcError(TEXT("llama_model_n_ctx_train"));
		return false;
	}

	vocab = llama_model_get_vocab(model);
	if (!vocab) {
		proc->ProcError(TEXT("llama_model_get_vocab"));
		return false;
	}

	// context

	int n_ctx = proc->GetProcessorInt("n_ctx");
	int n_threads = proc->GetNumBackendThreads();

	if (n_ctx <= 0) {
		n_ctx = n_ctx_train;
	}
	else if (n_ctx > n_ctx_train) {
		UE_LOG(LogHMI, Warning, TEXT(LOGPREFIX "n_ctx=%d > n_ctx_train=%d"), n_ctx, n_ctx_train);
		n_ctx = n_ctx_train;
	}

	ctx_params = llama_context_default_params();
	ctx_params.n_ctx = n_ctx;
	ctx_params.n_batch = n_ctx;
	if (n_threads > 0) {
		ctx_params.n_threads = n_threads;
		ctx_params.n_threads_batch = n_threads;
	}

	return true;
}

void FLlamaContext::Release()
{
	Cancel();
	ResetState();

	if (model) {
		UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "llama_model_free"));
		llama_model_free(model);
		model = nullptr;
	}

	llama_log_set(nullptr, nullptr);
}

void FLlamaContext::ResetState()
{
	if (smpl) {
		UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "llama_sampler_free"));
		llama_sampler_free(smpl);
		smpl = nullptr;
	}

	if (ctx) {
		UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "llama_free"));
		llama_free(ctx);
		ctx = nullptr;
	}

	for (auto& msg : messages) {
		free(const_cast<char*>(msg.content));
	}
	messages.clear();
}

void FLlamaContext::AddMessage(const FString& Role, const FString& Text)
{
	const char* role = "";
	if (Role.Equals(TEXT("system"))) { role = "system"; }
	else if (Role.Equals(TEXT("user"))) { role = "user"; }
	else if (Role.Equals(TEXT("assistant"))) { role = "assistant"; }
	else { UE_LOG(LogHMI, Warning, TEXT(LOGPREFIX "Unsupported role: %s"), *Role); }

	if (strlen(role) > 0 && !Text.IsEmpty()) {
		messages.push_back({ role, _strdup(TCHAR_TO_UTF8(*Text)) });
	}
}

bool FLlamaContext::ProcessMessages(std::function<void(const std::string&)> progress_func)
{
	start_process_ts = proc->GetTimestamp();

	cancel_flag = false;
	response.clear();
	error_text.Reset();

	// model

	UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "llama_init_from_model"));

	ctx = llama_init_from_model(model, ctx_params);
	if (!ctx) {
		error_text = TEXT("llama_init_from_model");
		return false;
	}

	formatted.resize(llama_n_ctx(ctx));

	// samplers

	smpl = llama_sampler_chain_init(llama_sampler_chain_default_params());
	if (!smpl) {
		error_text = TEXT("llama_sampler_chain_init");
		return false;
	}

	// penalties -> dry (dedup) -> top_n_sigma -> top_k -> top_p -> min_p -> typical_p -> temperature -> xtc (extra truncation)

	llama_sampler_chain_add(smpl, llama_sampler_init_penalties(penalty_last_n, penalty_repeat, penalty_freq, penalty_present));
	llama_sampler_chain_add(smpl, llama_sampler_init_top_k(top_k));
	llama_sampler_chain_add(smpl, llama_sampler_init_top_p(top_p, 1));
	llama_sampler_chain_add(smpl, llama_sampler_init_min_p(min_p, 1));
	llama_sampler_chain_add(smpl, llama_sampler_init_temp(temperature));
	llama_sampler_chain_add(smpl, llama_sampler_init_dist(LLAMA_DEFAULT_SEED)); // LAST!

	// template

	#if 0
	for (auto& msg : messages) {
		UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "MSG # %S # %S"), msg.role, msg.content);
	}
	#endif

	const char* tmpl = llama_model_chat_template(model, nullptr);
	if (!tmpl) {
		error_text = TEXT("llama_model_chat_template");
		return false;
	}

	int new_len = llama_chat_apply_template(tmpl, messages.data(), messages.size(), true, formatted.data(), formatted.size());
	if (new_len > (int)formatted.size()) {
		formatted.resize(new_len);
		new_len = llama_chat_apply_template(tmpl, messages.data(), messages.size(), true, formatted.data(), formatted.size());
	}

	if (new_len < 0) {
		error_text = TEXT("llama_chat_apply_template");
		return false;
	}

	const bool res = Generate(std::string(formatted.begin(), formatted.begin() + new_len), progress_func);

	return res;
}

bool FLlamaContext::Generate(std::string prompt, std::function<void(const std::string&)> progress_func)
{
	const size_t n_ctx = llama_n_ctx(ctx);

	//const bool is_first = llama_memory_seq_pos_max(llama_get_memory(ctx), 0) == -1;
	const bool is_first = true;

	size_t n_prompt_tokens = -llama_tokenize(vocab, prompt.c_str(), prompt.size(), nullptr, 0, is_first, true);
	if (n_prompt_tokens == 0 || n_prompt_tokens >= INT32_MAX) {
		error_text = TEXT("llama_tokenize");
		return false;
	}

	std::vector<llama_token> prompt_tokens(n_prompt_tokens);
	n_prompt_tokens = llama_tokenize(vocab, prompt.c_str(), prompt.size(), prompt_tokens.data(), prompt_tokens.size(), is_first, true);
	if (n_prompt_tokens != prompt_tokens.size()) {
		error_text = TEXT("llama_tokenize");
		return false;
	}

	llama_batch batch = llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());
	llama_token new_token_id;
	size_t generated_count = 0;

	while (!cancel_flag)
	{
		size_t n_ctx_used = llama_memory_seq_pos_max(llama_get_memory(ctx), 0) + 1;
		if (n_ctx_used + batch.n_tokens > n_ctx) {
			error_text = TEXT("max_ctx");
			return false;
		}

		int code = llama_decode(ctx, batch);
		if (code != 0) {
			error_text = TEXT("llama_decode");
			return false;
		}

		new_token_id = llama_sampler_sample(smpl, ctx, -1);
		generated_count++;

		if (llama_vocab_is_eog(vocab, new_token_id)) {
			n_ctx_used = llama_memory_seq_pos_max(llama_get_memory(ctx), 0) + 1;
			UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "generated_tokens=%zu n_ctx_used=%zu"), (size_t)generated_count, (size_t)n_ctx_used);
			return true; // end of generation
		}

		if (max_tokens > 0 && generated_count > max_tokens) {
			error_text = TEXT("max_tokens");
			return true; // truncate output
		}

		size_t piece_size = -llama_token_to_piece(vocab, new_token_id, nullptr, 0, 0, true);
		if (piece_size == 0 || piece_size >= INT32_MAX) {
			error_text = TEXT("llama_token_to_piece");
			return false;
		}

		piece.resize(piece_size);
		piece_size = llama_token_to_piece(vocab, new_token_id, piece.data(), piece_size, 0, true);
		if (piece_size != piece.size()) {
			error_text = TEXT("llama_token_to_piece");
			return false;
		}

		if (generated_count == 1) {
			UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "FirstTokenLatency=%f"), (proc->GetTimestamp() - start_process_ts));
		}

		response.append(piece);
		progress_func(piece);

		batch = llama_batch_get_one(&new_token_id, 1);
	}

	response.clear();
	return false;
}

#endif // HMI_WITH_LLAMA

#undef LOGPREFIX
