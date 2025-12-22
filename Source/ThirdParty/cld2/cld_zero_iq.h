#pragma once

#if defined(_WIN32) || defined(_WIN64)
	#ifdef CLD2_EXPORTS
		#define CLD2_API __declspec(dllexport)
	#elif CLD2_IMPORTS
		#define CLD2_API __declspec(dllimport)
	#else
		#define CLD2_API
	#endif
#else
	#define CLD2_API
#endif

extern "C" CLD2_API const char* DetectLanguageZeroIQ(const char* buffer, int buffer_length);
