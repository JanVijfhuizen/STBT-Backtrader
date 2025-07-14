#pragma once

namespace jv::bt
{
	constexpr uint32_t OUTPUT_MSG_MAX_SIZE = 86;

	struct OutputMsg final
	{
		enum Type
		{
			standard,
			error,
			warning
		} type;
		char buffer[OUTPUT_MSG_MAX_SIZE]{};
		glm::vec4 color{1};

		__declspec(dllexport) [[nodiscard]] static OutputMsg Create(const char* str, Type type = Type::standard);
		__declspec(dllexport) [[nodiscard]] static Array<OutputMsg> CreateMultiple(Arena& arena, const char* str);
	};
}
