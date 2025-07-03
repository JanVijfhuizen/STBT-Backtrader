#pragma once
#include <cstdint>
#include <JLib/KeyPair.h>

namespace jv
{
	// Nonlinear data container used for fast lookup (O(1)).
	template <typename T>
	struct Map final
	{
		KeyPair<T>* ptr = nullptr;
		uint64_t length = 0;
		uint32_t count = 0;

		void Insert(const T& instance, uint64_t key);
		[[nodiscard]] T* Contains(uint64_t key) const;
		void Erase(T& value);
		[[nodiscard]] uint64_t Hash(uint64_t key) const;
	};

	template <typename T>
	void Map<T>::Insert(const T& instance, const uint64_t key)
	{
		assert(count < length);

		// If it already contains this value, replace the old one with the newer value.
		if (Contains(key))
			return;

		const uint64_t hash = Hash(key);

		for (uint64_t i = 0; i < length; ++i)
		{
			const uint64_t index = (hash + i) % length;
			auto& keyPair = ptr[index];
			// Set to true the first time the key group has been found.
			if (keyPair.key != SIZE_MAX)
				continue;

			keyPair.key = key;
			keyPair.value = instance;
			++count;
			break;
		}
	}

	template <typename T>
	T* Map<T>::Contains(const uint64_t key) const
	{
		assert(count <= length);

		// Get and use the hash as an index.
		const uint64_t hash = Hash(key);

		for (uint64_t i = 0; i < length; ++i)
		{
			const uint64_t index = (hash + i) % length;
			auto& keyPair = ptr[index];

			// If the hash is different, continue.
			if (keyPair.key == key)
				return &keyPair.value;
		}

		return nullptr;
	}

	template <typename T>
	void Map<T>::Erase(T& value)
	{
		uint64_t index;
		const bool contains = Contains(value, index);
		assert(contains);
		assert(count > 0);

		auto& keyPair = ptr[index];

		// Check how big the key group is.
		uint64_t i = 1;
		while (i < length)
		{
			const uint64_t otherIndex = (index + i) % length;
			auto& otherKeyPair = ptr[otherIndex];
			if (otherKeyPair.key != keyPair.key)
				break;
			++i;
		}

		// Setting the key pair value to the default value.
		keyPair = {};
		// Move the key group one place backwards by swapping the first and last index.
		T t = ptr[index];
		ptr[index] = ptr[index + i - 1];
		ptr[index + i - 1] = t;
		--count;
	}

	template <typename T>
	uint64_t Map<T>::Hash(const uint64_t key) const
	{
		return key % length;
	}
}
