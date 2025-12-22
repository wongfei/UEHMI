#include "Audio/HMIPcm.h"

#include "DSP/FloatArrayMath.h"

void FPcm::S16_F32(const int16* Src, int SrcCount, float* Dst, int DstCount)
{
	const int Num = FMath::Min(SrcCount, DstCount);
	const float Scale = 1.0f / 32768.0f;

	#if 0

	for (int i = 0; i < Num; ++i)
	{
		Dst[i] = float(Src[i]) * Scale;
	}
	for (int i = Num; i < DstCount; ++i)
	{
		Dst[i] = 0.0f;
	}

	#else

	Audio::ArrayPcm16ToFloat(TArrayView<const int16>(Src, Num), TArrayView<float>(Dst, Num));

	if (Num < DstCount)
		memset(&Dst[Num], 0, (DstCount - Num) * sizeof(Dst[0]));

	#endif
}

void FPcm::F32_S16(const float* Src, int SrcCount, int16* Dst, int DstCount)
{
	const int Num = FMath::Min(SrcCount, DstCount);

	#if 0

	for (int i = 0; i < Num; ++i)
	{
		Dst[i] = Src[i] * 32768.0f;
	}

	for (int i = Num; i < DstCount; ++i)
	{
		Dst[i] = 0;
	}

	#else

	Audio::ArrayFloatToPcm16(TArrayView<const float>(Src, Num), TArrayView<int16>(Dst, Num));

	if (Num < DstCount)
		memset(&Dst[Num], 0, (DstCount - Num) * sizeof(Dst[0]));

	#endif
}
