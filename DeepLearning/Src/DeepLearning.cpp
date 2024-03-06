#include "pch.h"
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

	DeepMotherCreateInfo motherCreateInfo{};
	auto mother = DeepMother::Create(arena, motherCreateInfo);
	return 0;
}
