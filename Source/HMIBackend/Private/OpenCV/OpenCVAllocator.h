// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

namespace OpenCVAllocator
{
	static FCriticalSection UnrealAllocationsSetMutex;
	static TSet<void*> UnrealAllocationsSet;
	static uint64 UnrealAllocationsCount = 0;

	static void* UnrealMalloc(size_t Count, uint32_t Alignment)
	{
		void* Address = FMemory::Malloc(static_cast<SIZE_T>(Count), static_cast<uint32>(Alignment));

		{
			FScopeLock Lock(&UnrealAllocationsSetMutex);
			UnrealAllocationsSet.Add(Address);
			UnrealAllocationsCount++;
		}

		return Address;
	}

	static void UnrealFree(void* Original)
	{
		// Only free allocations made by Unreal. Any allocations made before new/delete was overridden will have to leak.
		{
			FScopeLock Lock(&UnrealAllocationsSetMutex);

			if (!UnrealAllocationsSet.Contains(Original))
			{
				return;
			}

			UnrealAllocationsSet.Remove(Original);
			--UnrealAllocationsCount;
		}

		FMemory::Free(Original);
	}
}
