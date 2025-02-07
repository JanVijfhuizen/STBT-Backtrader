#pragma once

namespace jv::gr 
{
	struct Shader final
	{
		unsigned int vertex, fragment, program;
	};

	__declspec(dllexport) [[nodiscard]] Shader LoadShader(const char* vertex, const char* fragment);
	__declspec(dllexport) [[nodiscard]] unsigned int LoadShaderSource(const char* file, int type);
	__declspec(dllexport) void LinkShader(Shader& shader);
	__declspec(dllexport) void DestroyShader(const Shader& shader);
	__declspec(dllexport) [[nodiscard]] unsigned int GetShaderUniform(const Shader& shader, const char* name);
	__declspec(dllexport) void SetShaderUniform2f(const Shader& shader, unsigned int i, glm::vec2 v);
	__declspec(dllexport) void SetShaderUniform4f(const Shader& shader, unsigned int i, glm::vec4 v);
}
