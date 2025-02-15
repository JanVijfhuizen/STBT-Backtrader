#pragma once
#include <Jlib/Arena.h>
#include <Jlib/VectorUtils.h>

namespace jv 
{
	template <typename T>
	class MenuItem
	{
	public:
		bool reload = false;

		virtual void Init(Arena& arena, T& t) {};
		virtual void Load(T& t) {};
		virtual bool Update(T& t, uint32_t& index) = 0;
		virtual void Unload(T& t) {};
		void RequestReload();
	};

	template <typename T>
	struct Menu final
	{
		uint64_t scope;
		uint64_t itemScope;
		Vector<MenuItem<T>*> items;

		uint32_t index;

		MenuItem<T>*& Add();
		void Init(Arena& arena, T& t);
		void SetIndex(Arena& arena, T& t, uint32_t i);
		bool Update(Arena& arena, T& t);
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
	void Menu<T>::Init(Arena& arena, T& t)
	{
		for (auto item : items)
			item->Init(arena, t);
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
	bool Menu<T>::Update(Arena& arena, T& t)
	{
		if (index == -1)
			return false;
		uint32_t i = -1;
		auto item = items[index];
		const auto ret = item->Update(t, i);
		if (item->reload)
		{
			SetIndex(arena, t, i);
			item->reload = false;
		}
			
		if (i != -1)
			SetIndex(arena, t, i);
		return ret;
	}

	template<typename T>
	uint32_t Menu<T>::GetIndex() const
	{
		return index;
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

	template<typename T>
	void MenuItem<T>::RequestReload()
	{
		reload = true;
	}
}