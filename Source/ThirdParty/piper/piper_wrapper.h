#include <stdint.h>

#if defined(_WIN32) || defined(_WIN64)
	#ifdef PIPER_EXPORTS
		#define PIPER_API __declspec(dllexport)
	#elif PIPER_IMPORTS
		#define PIPER_API __declspec(dllimport)
	#else
		#define PIPER_API
	#endif
#else
	#define PIPER_API
#endif

#define PIPER_FUNC(ret, name, args)\
	typedef ret (* name##_ft) args;\
	PIPER_API ret name args

#define PIPER_FP(name) name##_ft name##_fp

struct piper_context;
typedef void* piper_buffer_ptr;

extern "C" {

PIPER_FUNC (const char*, piper_get_error, ());

PIPER_FUNC (piper_context*, piper_init, (const char* model_path, const char* data_path, bool use_cuda));
PIPER_FUNC (void, piper_release, (piper_context* context));

PIPER_FUNC (int, piper_get_voice_sample_rate, (piper_context* context));
PIPER_FUNC (int, piper_get_voice_sample_bytes, (piper_context* context));
PIPER_FUNC (int, piper_get_voice_channels, (piper_context* context));
PIPER_FUNC (int, piper_get_num_speakers, (piper_context* context));

PIPER_FUNC (piper_buffer_ptr, piper_alloc_buffer, ());
PIPER_FUNC (void, piper_free_buffer, (piper_buffer_ptr bufp));
PIPER_FUNC (void*, piper_get_buffer_data, (piper_buffer_ptr bufp));
PIPER_FUNC (size_t, piper_get_buffer_size, (piper_buffer_ptr bufp));

PIPER_FUNC (int, piper_text_to_buffer, (piper_context* context, const char* text, piper_buffer_ptr bufp, int speaker_id, float length_scale, volatile int* pcancel_flag));
PIPER_FUNC (int, piper_text_to_file, (piper_context* context, const char* text, const char* filename, int speaker_id, float length_scale));

}

// dont forget to update -> piper_get_api
struct piper_api
{
	PIPER_FP(piper_get_error);

	PIPER_FP(piper_init);
	PIPER_FP(piper_release);

	PIPER_FP(piper_get_voice_sample_rate);
	PIPER_FP(piper_get_voice_sample_bytes);
	PIPER_FP(piper_get_voice_channels);
	PIPER_FP(piper_get_num_speakers);

	PIPER_FP(piper_alloc_buffer);
	PIPER_FP(piper_free_buffer);
	PIPER_FP(piper_get_buffer_data);
	PIPER_FP(piper_get_buffer_size);

	PIPER_FP(piper_text_to_buffer);
	PIPER_FP(piper_text_to_file);
};

extern "C" {

PIPER_FUNC (int, piper_get_api, (struct piper_api* api));

}
