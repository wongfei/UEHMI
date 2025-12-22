#pragma once

#include "HMIMinimal.h"
#include "Audio/HMIPcm.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "HMIBuffer.generated.h"

enum
{
	HMI_BUFFER_ALIGNMENT = 16
};

struct HMI_API FHMIBuffer
{
	protected:

	void* Data = 0;
	SIZE_T Size = 0;
	uint32 Alignment = HMI_BUFFER_ALIGNMENT;
	FName Tag = NAME_None;

	public:

	void* GetData() { return Data; }
	const void* GetData() const { return Data; }
	SIZE_T GetSize() const { return Size; }

	const FName& GetTag() const { return Tag; }
	void SetTag(const FName& InTag) { Tag = InTag; }

	bool IsValid() const { return Size ? true : false; }
	bool IsEmpty() const { return !Size; }

	uint8* CastU8() { return (uint8*)Data; }
	const uint8* CastU8() const { return (const uint8*)Data; }

	int16* CastS16() { return (int16*)Data; }
	const int16* CastS16() const { return (const int16*)Data; }

	float* CastF32() { return (float*)Data; }
	const float* CastF32() const { return (const float*)Data; }

	template<typename T>
	operator TArrayView<T>() { return { (T*)GetData(), (int32)(GetSize() / sizeof(T)) }; }

	template<typename T>
	operator TArrayView<const T>() const { return { (const T*)GetData(), (int32)(GetSize() / sizeof(T)) }; }

	FHMIBuffer(uint32 NewAlignment = HMI_BUFFER_ALIGNMENT) : Alignment(NewAlignment) {}
	~FHMIBuffer() { Purge(); }

	// move
	FHMIBuffer(FHMIBuffer&&) = default;
	FHMIBuffer& operator=(FHMIBuffer&& Other)
	{
		TakeOwnershipFrom(Other);
		return *this;
	}
	void TakeOwnershipFrom(FHMIBuffer& Other)
	{
		Purge();
		Data = Other.Data;
		Size = Other.Size;
		Alignment = Other.Alignment;
		Other.Data = 0;
		Other.Size = 0;
	}

	// copy
	FHMIBuffer(const FHMIBuffer&) = delete;
	FHMIBuffer& operator=(const FHMIBuffer& Other) = delete;

	void Resize(SIZE_T NewSize)
	{
		if (NewSize)
		{
			if (Data)
				Data = FMemory::Realloc(Data, NewSize, Alignment);
			else
				Data = FMemory::Malloc(NewSize, Alignment);
			check(Data);
			Size = NewSize;
		}
		else
		{
			Purge();
		}
	}

	void Purge()
	{
		if (Data)
		{
			FMemory::Free(Data);
			Data = nullptr;
		}
		Size = 0;
	}

	void CopyFrom(const void* SrcData, SIZE_T SrcSize)
	{
		Resize(SrcSize);
		if (Size)
			FMemory::Memcpy(Data, SrcData, Size);
	}

	template<typename T>
	void CopyFrom(const TArrayView<const T>& Src)
	{
		CopyFrom(Src.GetData(), (SIZE_T)Src.Num() * sizeof(T));
	}

	template<typename T>
	void CopyFrom(const TArray<T>& Src)
	{
		CopyFrom(Src.GetData(), (SIZE_T)Src.Num() * sizeof(T));
	}

	void CopyFrom(const FHMIBuffer& Src)
	{
		CopyFrom(Src.Data, Src.Size);
	}

	void Append(const void* SrcData, SIZE_T SrcSize)
	{
		const SIZE_T NewSize = Size + SrcSize;
		if (NewSize > Size)
		{
			const SIZE_T OldSize = Size;
			Resize(NewSize);
			FMemory::Memcpy((uint8*)Data + OldSize, SrcData, SrcSize);
		}
	}

	void Append(const FHMIBuffer& Src)
	{
		Append(Src.Data, Src.Size);
	}
};

// WAVE

struct HMI_API FHMIWaveFormat
{
	static const FName PCM_S16;
	static const FName PCM_F32;

	FName FormatId = NAME_None;
	uint32 SampleRate = 0;
	uint8 SampleBits = 0;
	uint8 NumChannels = 0;

	static FHMIWaveFormat Make_PCM_S16(uint32 InSampleRate, uint8 InNumChannels) { return {PCM_S16, InSampleRate, 16, InNumChannels}; }
	static FHMIWaveFormat Make_PCM_F32(uint32 InSampleRate, uint8 InNumChannels) { return {PCM_F32, InSampleRate, 32, InNumChannels}; }
};

struct HMI_API FHMIWaveBuffer : public FHMIBuffer
{
	FHMIWaveFormat Format;

	FHMIWaveBuffer(uint32 NewAlignment = HMI_BUFFER_ALIGNMENT) : 
		FHMIBuffer(NewAlignment) {}

	FHMIWaveBuffer(const FHMIWaveFormat& InFormat, uint32 NewAlignment = HMI_BUFFER_ALIGNMENT) : 
		FHMIBuffer(NewAlignment), Format(InFormat) {}

	auto GetFormat() const			{ return Format; }
	auto GetFormatId() const		{ return Format.FormatId; }
	auto GetSampleRate() const		{ return Format.SampleRate; }
	auto GetSampleBits() const		{ return Format.SampleBits; }
	auto GetSampleBytes() const		{ return Format.SampleBits / 8; }
	auto GetNumChannels() const		{ return Format.NumChannels; }

	int GetNumFrames() const		{ const int Divisor = GetSampleBytes() * GetNumChannels(); return Divisor ? GetSize() / Divisor : 0; }
	int GetNumSamples() const		{ const int Divisor = GetSampleBytes(); return Divisor ? GetSize() / Divisor : 0; }
	float GetDuration() const		{ return Format.SampleRate ? GetNumFrames() / (float)Format.SampleRate : 0.0f; }
};

USTRUCT(BlueprintType, Category="HMI|Wave")
struct HMI_API FHMIWaveHandle
{
	GENERATED_BODY()

	mutable TSharedPtr<FHMIWaveBuffer> Buf;

	static FHMIWaveHandle Alloc()
	{
		return FHMIWaveHandle(MakeShared<FHMIWaveBuffer>());
	}

	static FHMIWaveHandle Alloc(const FHMIWaveFormat& InFormat)
	{
		return FHMIWaveHandle(MakeShared<FHMIWaveBuffer>(InFormat));
	}

	static FHMIWaveHandle Alloc_PCM_S16(uint32 InSampleRate, uint8 InNumChannels)
	{
		return FHMIWaveHandle(MakeShared<FHMIWaveBuffer>(FHMIWaveFormat::Make_PCM_S16(InSampleRate, InNumChannels)));
	}

	FHMIWaveHandle() = default;
	~FHMIWaveHandle() = default;

	// move
	FHMIWaveHandle(FHMIWaveHandle&&) = default;
	FHMIWaveHandle& operator=(FHMIWaveHandle&&) = default;

	// copy
	FHMIWaveHandle(const FHMIWaveHandle&) = default;
	FHMIWaveHandle& operator=(const FHMIWaveHandle&) = default;

	FHMIWaveHandle(TSharedPtr<FHMIWaveBuffer>&& Other)
	{
		Buf = MoveTemp(Other);
	}

	template<typename T>
	FHMIWaveHandle(const FHMIWaveFormat& Format, const TArray<T>& Other)
	{
		Buf = MakeShared<FHMIWaveBuffer>();
		Buf->Format = Format;
		Buf->CopyFrom(Other);
	}

	bool IsValid() const			{ return Buf ? Buf->IsValid() : false; }
	bool IsEmpty() const			{ return Buf ? Buf->IsEmpty() : true; }
	auto GetSize() const			{ return Buf ? Buf->GetSize() : 0; }
	auto GetData()					{ return Buf ? Buf->GetData() : nullptr; }
	auto GetData() const			{ return Buf ? (const void*)Buf->GetData() : nullptr; }

	auto CastU8()					{ return Buf ? (uint8*)Buf->GetData() : nullptr; }
	auto CastU8() const				{ return Buf ? (const uint8*)Buf->GetData() : nullptr; }
	auto CastS16()					{ return Buf ? (int16*)Buf->GetData() : nullptr; }
	auto CastS16() const			{ return Buf ? (const int16*)Buf->GetData() : nullptr; }
	auto CastF32()					{ return Buf ? (float*)Buf->GetData() : nullptr; }
	auto CastF32() const			{ return Buf ? (const float*)Buf->GetData() : nullptr; }

	auto GetTag() const				{ return Buf ? Buf->GetTag() : NAME_None; }
	auto GetFormat() const			{ return Buf ? Buf->GetFormat() : FHMIWaveFormat{}; }
	auto GetFormatId() const		{ return Buf ? Buf->GetFormatId() : NAME_None; }
	auto GetSampleRate() const		{ return Buf ? Buf->GetSampleRate() : 0; }
	auto GetSampleBits() const		{ return Buf ? Buf->GetSampleBits() : 0; }
	auto GetNumChannels() const		{ return Buf ? Buf->GetNumChannels() : 0; }
	auto GetNumFrames() const		{ return Buf ? Buf->GetNumFrames() : 0; }
	auto GetNumSamples() const		{ return Buf ? Buf->GetNumSamples() : 0; }
	auto GetDuration() const		{ return Buf ? Buf->GetDuration() : 0.0f; }
	
	void EnsureValid() { if (!Buf) { Buf = MakeShared<FHMIWaveBuffer>(); } }
	void Reset() { Buf.Reset(); }

	void SetTag(const FName& Tag) { EnsureValid(); Buf->SetTag(Tag); }
	void Resize(SIZE_T NewSize) { EnsureValid(); Buf->Resize(NewSize); }

	void CopyFrom(const void* SrcData, SIZE_T SrcSize) { EnsureValid(); Buf->CopyFrom(SrcData, SrcSize); }

	template<typename T> 
	void CopyFrom(const TArrayView<const T>& Src) { EnsureValid(); Buf->CopyFrom(Src); }

	template<typename T> 
	void CopyFrom(const TArray<T>& Src) { EnsureValid(); Buf->CopyFrom(Src); }

	template<typename T>
	TArrayView<T> GetView() { return { (T*)GetData(), (int32)(GetSize() / sizeof(T)) }; }

	template<typename T>
	TArrayView<const T> GetView() const { return { (const T*)GetData(), (int32)(GetSize() / sizeof(T)) }; }

	template<typename T>
	operator TArrayView<T>() { return GetView<T>(); }

	template<typename T>
	operator TArrayView<const T>() const { return GetView<const T>(); }
};

// UTILS

UCLASS(ClassGroup="HMI|Utils")
class HMI_API UHMIBufferStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	public:

	UFUNCTION(BlueprintCallable, Category="HMI|Utils")
	static void SetWaveTag(UPARAM(ref) FHMIWaveHandle& Wave, FName Tag) { Wave.SetTag(Tag); }

	UFUNCTION(BlueprintPure, Category="HMI|Utils")
	static FName GetWaveTag(const FHMIWaveHandle& Wave) { return Wave.GetTag(); }

	UFUNCTION(BlueprintPure, Category="HMI|Utils")
	static TArray<uint8> GetWaveBytes(const FHMIWaveHandle& Wave) { return TArray<uint8>(Wave.GetView<const uint8>()); }

	UFUNCTION(BlueprintCallable, Category="HMI|Utils")
	static bool SaveWaveBuffer(const FHMIWaveHandle& Wave, const FString& Filepath);

	UFUNCTION(BlueprintCallable, Category="HMI|Utils")
	FHMIWaveHandle LoadWaveBuffer(const FString& Filepath);

	UFUNCTION(BlueprintCallable, Category="HMI|Utils")
	static FHMIWaveHandle ConvertSoundToWaveBuffer(class USoundWave* SoundWave);

	static bool IsValidSampleRate(int SampleRate);
	static bool IsValidFormat(const FHMIWaveFormat& Format);
	static bool IsValidVoice(const FHMIWaveHandle& Wave);
};
