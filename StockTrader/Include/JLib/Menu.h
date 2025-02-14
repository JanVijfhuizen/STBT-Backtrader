#pragma once
#include <Jlib/Arena.h>
#include <Jlib/VectorUtils.h>

namespace jv 
{
	template <typename T>
	class MenuItem
	{
		virtual void Load(T& t) {};
		virtual void Update(T& t) = 0;
		virtual void Unload(T& t) {};
	};

	template <typename T>
	struct Menu final
	{
		uint64_t scope;
		uint64_t itemScope;
		Vector<MenuItem<T>*> items;

		uint32_t index;

		MenuItem<T>*& Add();
		void SetIndex(Arena& arena, T& t, uint32_t i);
		void Update(T& t);
		[[nodiscard]] uint32_t GetIndex() const;
		void ClearItemScope(Arena& arena);

		[[nodiscard]] static Menu<T> CreateMenu(Arena& arena, uint32_t length);
		static void DestroyMenu(Arena& arena, Menu<T>& menu);
	};

	template<typename T>
	MenuItem<T>*& Menu<T>::Add()
	{
		assert(items.count < items.length);
		return items.Add();
	}

	template<typename T>
	void Menu<T>::SetIndex(Arena& arena, T& t, const uint32_t i)
	{
		assert(i < items.length || i == -1);

		if (index != -1)
		{
			arena.DestroyScope(itemScope);
			items[index]->Unload(t);
		}
		
		index = i;
		if (index != -1)
		{
			itemScope = arena.CreateScope();
			items[index]->Load(t);
		}
	}

	template<typename T>
	void Menu<T>::Update(T& t)
	{
		if (index == -1)
			return;
		items[index]->Update(t);
	}

	template<typename T>
	uint32_t Menu<T>::GetIndex() const
	{
		return index + 1;
	}

	template<typename T>
	void Menu<T>::ClearItemScope(Arena& arena)
	{
		arena.DestroyScope(itemScope);
	}

	template<typename T>
	Menu<T> Menu<T>::CreateMenu(Arena& arena, const uint32_t length)
	{
		Menu<T> menu{};
		menu.scope = arena.CreateScope();
		menu.items = CreateVector<MenuItem<T>*>(arena, length);
		menu.index = -1;
		return menu;
	}
	template<typename T>
	void Menu<T>::DestroyMenu(Arena& arena, Menu<T>& menu)
	{
		arena.DestroyScope(menu.scope);
	}
}