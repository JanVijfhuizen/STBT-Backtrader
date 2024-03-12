#include "pch.h"

#include <iostream>

#include "DeepInstance.h"
#include "DeepMother.h"
#include "JLib/Arena.h"

void* Alloc(const uint32_t size)
{
	return malloc(size);
}

void Free(void* ptr)
{
	return free(ptr);
}

int main()
{
	srand(static_cast <unsigned> (time(0)));

	jv::ArenaCreateInfo arenaCreateInfo{};
	arenaCreateInfo.alloc = Alloc;
	arenaCreateInfo.free = Free;
	auto arena = jv::Arena::Create(arenaCreateInfo);
	auto tempArena = jv::Arena::Create(arenaCreateInfo);

	DeepMotherCreateInfo motherCreateInfo{};
	auto mother = DeepMother::Create(arena, motherCreateInfo);

	DeepInstance deepInstance{};
	for (int i = 0; i < 5; ++i)
		mother.AddNode(arena, deepInstance);

	for (int i = 0; i < 1000; ++i)
	{
		const auto scope = tempArena.CreateScope();
		const auto metaData = mother.Apply(tempArena, deepInstance);
		mother.Mutate(arena, metaData, deepInstance);
		tempArena.DestroyScope(scope);
	}

	const auto metaData = mother.Apply(tempArena, deepInstance);
	for (int i = 0; i < 1000; ++i)
	{
		const float inp = sin(.1f * i);

		for (int j = 0; j < 3; ++j)
			mother.InputValue(j, inp, .1f);

		mother.Update(metaData, .1f);
		std::cout << mother.ReadOutput(3) << " / " << mother.ReadOutput(4) << std::endl;
	}
	return 0;
}
