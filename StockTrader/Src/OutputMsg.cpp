#include "pch.h"
#include "OutputMsg.h"
#include "Jlib/ArrayUtils.h"

namespace jv::bt
{
	OutputMsg OutputMsg::Create(const char* str, const Type type)
	{
		OutputMsg msg{};
		msg.type = type;
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

		memcpy(msg.buffer, prefix, prefixLen);
		memcpy(&msg.buffer[prefixLen], str, Min(l, OUTPUT_MSG_MAX_SIZE - prefixLen));
		return msg;
	}

	Array<OutputMsg> OutputMsg::CreateMultiple(Arena& arena, const char* str)
	{
		std::stringstream ss(str);
		std::string line;
		uint32_t c = 0;

		while (getline(ss, line))
			c += ceil((float)line.length() / OUTPUT_MSG_MAX_SIZE);

		ss.clear();
		ss.seekg(0);

		auto arr = CreateArray<OutputMsg>(arena, c);

		uint32_t i = 0;
		while (getline(ss, line))
		{
			const uint32_t lineLen = line.length();
			const uint32_t chunks = ceil((float)lineLen / OUTPUT_MSG_MAX_SIZE);
			for (uint32_t j = 0; j < chunks; j++)
			{
				const uint32_t ind = j * OUTPUT_MSG_MAX_SIZE;
				const uint32_t s = Min(lineLen - ind, OUTPUT_MSG_MAX_SIZE);
				auto sub = line.substr(ind, s);
				while(sub[0] == ' ')
					sub.erase(0, 1);

				arr[i++] = Create(sub.c_str());
			}
		}
			
		return arr;
	}
}