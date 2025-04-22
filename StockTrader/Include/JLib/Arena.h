#pragma once
#include <cstdint>

namespace jv
{
	struct Arena;

	struct ArenaCreateInfo final
	{
		void* (*alloc)(uint32_t size);
		void (*free)(void* ptr);
		void* memory = nullptr;
		uint32_t memorySize = 4096 * 256 * 32;
	};

	struct ArenaAllocMetaData final
	{
		uint32_t size;
	};

	// Handles manual memory allocation.
	// Allocates one or more large chunks of memory which can then be (re)used for smaller linear allocations.
	struct Arena final
	{
		struct Scope final
		{
			struct Unpacked final
			{
				uint32_t depth;
				uint32_t front;
			};

			union
			{
				uint64_t handle;
				Unpacked unpacked;
			};
		};

		ArenaCreateInfo info;
		void* memory;
		uint32_t front = 0;
		Arena* next = nullptr;

		__declspec(dllexport) [[nodiscard]] static Arena Create(const ArenaCreateInfo& info);
		__declspec(dllexport) static void Destroy(const Arena& arena);

		__declspec(dllexport) void* Alloc(uint32_t size);
		__declspec(dllexport) void Free(const void* ptr);
		__declspec(dllexport) void Clear();
		__declspec(dllexport) [[nodiscard] ]uint32_t GetTotalUsedMemory() const;

		template <typename T>
		__declspec(dllexport) [[nodiscard]] T* New(size_t count = 1);

		// A scope can be used to instantly delete everything that was made after the scope's creation.
		__declspec(dllexport) [[nodiscard]] uint64_t CreateScope() const;
		__declspec(dllexport) void DestroyScope(uint64_t handle);
	};

	template <typename T>
	T* Arena::New(const size_t count)
	{
		void* ptr = Alloc(sizeof(T) * count);
		T* ptrType = static_cast<T*>(ptr);
		for (uint32_t i = 0; i < count; ++i)
			new(&ptrType[i]) T();
		return ptrType;
	}
}
