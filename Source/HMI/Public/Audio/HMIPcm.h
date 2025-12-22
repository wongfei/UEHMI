#pragma once

#include "HMIMinimal.h"

class HMI_API FPcm
{
	public:

	static void S16_F32(const int16* Src, int SrcCount, float* Dst, int DstCount);
	static void F32_S16(const float* Src, int SrcCount, int16* Dst, int DstCount);

	static void S16_F32(const TArrayView<const int16>& Src, TArrayView<float> Dst)
	{
		S16_F32(Src.GetData(), Src.Num(), Dst.GetData(), Dst.Num());
	}

	static void F32_S16(const TArrayView<const float>& Src, TArrayView<int16> Dst)
	{
		F32_S16(Src.GetData(), Src.Num(), Dst.GetData(), Dst.Num());
	}

	static void S16_F32(const TArrayView<const int16>& Src, TArray<float>& Dst, int DstCount = 0)
	{
		if (!DstCount) DstCount = Src.Num();
		Dst.SetNumUninitialized(DstCount, EAllowShrinking::No);
		S16_F32(Src.GetData(), Src.Num(), Dst.GetData(), DstCount);
	}

	static void F32_S16(const TArrayView<const float>& Src, TArray<int16>& Dst, int DstCount = 0)
	{
		if (!DstCount) DstCount = Src.Num();
		Dst.SetNumUninitialized(DstCount, EAllowShrinking::No);
		F32_S16(Src.GetData(), Src.Num(), Dst.GetData(), DstCount);
	}
};
