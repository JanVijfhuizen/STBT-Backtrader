#include "pch.h"

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
		mother.Update(metaData, 0.1f);
	return 0;
}
