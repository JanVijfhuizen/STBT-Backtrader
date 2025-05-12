#pragma once

namespace jv::bt
{
	constexpr uint32_t OUTPUT_MSG_MAX_SIZE = 128;

	struct OutputMsg final
	{
		enum Type
		{
			standard,
			error,
			warning
		};

		char buffer[OUTPUT_MSG_MAX_SIZE]{};

		static OutputMsg Create(const char* str, const Type type = Type::standard)
		{
			OutputMsg msg{};
			const uint32_t l = strlen(str);

			const char* prefix = "";
		
			switch (type)
			{
			case jv::bt::OutputMsg::error:
				prefix = "ERROR: ";
				break;
			case jv::bt::OutputMsg::warning:
				prefix = "WARNING: ";
				break;
			default:
				break;
			}

			const uint32_t prefixLen = strlen(prefix);
			assert(OUTPUT_MSG_MAX_SIZE > prefixLen);

			memcpy(msg.buffer,prefix, prefixLen);
			memcpy(&msg.buffer[prefixLen], str, Min(l, OUTPUT_MSG_MAX_SIZE - prefixLen));
			return msg;
		}
	};
}
