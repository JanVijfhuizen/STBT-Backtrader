#pragma once
#include <cstdint>
#include <JLib/Math.h>

namespace jv
{
	template <typename T>
	struct Queue;

	// Nonlinear data container that uses a First In First Out structure.
	template <typename T>
	struct QueueIterator final
	{
		const Queue<T>* ptr = nullptr;
		uint32_t index = 0;

		T& operator*() const;
		T& operator->() const;

		const QueueIterator& operator++();
		QueueIterator operator++(int);

		friend bool operator==(const QueueIterator& a, const QueueIterator& b)
		{
			return a.index == b.index;
		}

		friend bool operator!= (const QueueIterator& a, const QueueIterator& b)
		{
			return !(a == b);
		}
	};

	template <typename T>
	struct Queue final
	{
		T* ptr = nullptr;
		uint32_t length = 0;
		uint32_t count = 0;
		uint32_t front = 0;

		__declspec(dllexport) [[nodiscard]] T& operator[](uint32_t i) const;
		__declspec(dllexport) [[nodiscard]] QueueIterator<T> begin() const;
		__declspec(dllexport) [[nodiscard]] QueueIterator<T> end() const;

		__declspec(dllexport) T& Add();
		__declspec(dllexport) [[nodiscard]] T& Peek() const;
		__declspec(dllexport) T Pop();

		__declspec(dllexport) [[nodiscard]] uint32_t GetIndex(uint32_t i) const;
	};

	template <typename T>
	T& QueueIterator<T>::operator*() const
	{
		assert(ptr);
		assert(index <= ptr->length);
		return (*ptr)[index];
	}

	template <typename T>
	T& QueueIterator<T>::operator->() const
	{
		assert(ptr);
		assert(index <= ptr->length);
		return (*ptr)[index];
	}

	template <typename T>
	const QueueIterator<T>& QueueIterator<T>::operator++()
	{
		++index;
		return *this;
	}

	template <typename T>
	QueueIterator<T> QueueIterator<T>::operator++(int)
	{
		Iterator temp(ptr, ptr->length, index);
		++index;
		return temp;
	}

	template <typename T>
	T& Queue<T>::operator[](const uint32_t i) const
	{
		assert(i < count);
		return ptr[GetIndex(i)];
	}

	template <typename T>
	QueueIterator<T> Queue<T>::begin() const
	{
		QueueIterator<T> it{};
		it.index = 0;
		it.ptr = this;
		return it;
	}

	template <typename T>
	QueueIterator<T> Queue<T>::end() const
	{
		QueueIterator<T> it{};
		it.index = count;
		it.ptr = this;
		return it;
	}

	template <typename T>
	T& Queue<T>::Add()
	{
		auto& ret = ptr[GetIndex(count++)];
		if (count > length)
			front = (front + 1) % length;
		count = Min<uint32_t>(count, length);
		return ret;
	}

	template <typename T>
	T& Queue<T>::Peek() const
	{
		assert(count > 0);
		return ptr[GetIndex(count - 1)];
	}

	template <typename T>
	T Queue<T>::Pop()
	{
		assert(count > 0);
		return ptr[GetIndex(--count)];
	}

	template <typename T>
	uint32_t Queue<T>::GetIndex(const uint32_t i) const
	{
		return (i + front + length) % length;
	}
}
