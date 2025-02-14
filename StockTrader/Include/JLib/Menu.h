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
		Vector<MenuItem<T>> items;

		T* t;
		uint32_t index;

		void Add(MenuItem<T>* ptr);
		void SetIndex(Arena& arena, uint32_t i);
		void Update();

		[[nodiscard]] static Menu<T> CreateMenu(Arena& arena, uint32_t length, T* t);
		static void DestroyMenu(Arena& arena, Menu<T>& menu);
	};

	template<typename T>
	void Menu<T>::Add(MenuItem<T>* ptr)
	{
		assert(items.count < items.length);
		items.Add() = ptr;
	}

	template<typename T>
	void Menu<T>::SetIndex(Arena& arena, const uint32_t i)
	{
		assert(i < items.length || i == -1);

		if (index != -1)
		{
			arena.DestroyScope(itemScope);
			items[index]->Unload(*t);
		}
		
		index = i;
		if (index != -1)
		{
			itemScope = arena.CreateScope();
			items[index]->Load(*t);
		}
	}

	template<typename T>
	void Menu<T>::Update()
	{
		if (index == -1)
			return;
		items[index]->Update(*t);
	}

	template<typename T>
	Menu<T> Menu<T>::CreateMenu(Arena& arena, const uint32_t length, T* t)
	{
		Menu<T> menu{};
		menu.scope = arena.CreateScope();
		menu.items = CreateVector<MenuItem<T>*>(arena, length);
		menu.index = -1;
		menu.t = t;
		return menu;
	}
	template<typename T>
	void Menu<T>::DestroyMenu(Arena& arena, Menu<T>& menu)
	{
		arena.DestroyScope(menu.scope);
	}
}