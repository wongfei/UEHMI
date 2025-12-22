#include "HMILanguageDetector.h"

#if HMI_WITH_CLD2
#include "cld_zero_iq.h"
#include <string>
#endif

static const FName UnknownLanguage("un");

FName UHMILanguageDetector::DetectLanguage(const FString& Text)
{
	#if HMI_WITH_CLD2
	if (!Text.IsEmpty())
	{
		std::string buf(TCHAR_TO_UTF8(*Text));
		const char* lang = DetectLanguageZeroIQ(buf.c_str(), (int)buf.size());
		return lang ? FName(lang) : UnknownLanguage;
	}
	#endif

	return UnknownLanguage;
}
