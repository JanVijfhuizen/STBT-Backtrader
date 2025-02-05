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
	[[nodiscard]] unsigned int GetShaderUniform(const Shader& shader, const char* name);
	void SetShaderUniform2f(const Shader& shader, unsigned int i, glm::vec2 v);
	void SetShaderUniform4f(const Shader& shader, unsigned int i, glm::vec4 v);
}
