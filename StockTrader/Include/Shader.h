#pragma once

namespace jv::gr 
{
	struct Shader final
	{
		unsigned int vertex, fragment, program;
	};

	[[nodiscard]] Shader LoadShader(const char* vertex, const char* fragment);
	[[nodiscard]] unsigned int LoadShaderSource(const char* file, int type);
	void LinkShader(Shader& shader);
	void DestroyShader(const Shader& shader);
}
