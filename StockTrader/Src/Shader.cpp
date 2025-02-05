#include "pch.h"
#include "Shader.h"

namespace jv::gr
{
	Shader LoadShader(const char* vertex, const char* fragment)
	{
		Shader shader{};
		shader.vertex = LoadShaderSource(vertex, GL_VERTEX_SHADER);
		shader.fragment = LoadShaderSource(fragment, GL_FRAGMENT_SHADER);

		LinkShader(shader);

		int success;
		char infoLog[512];

		glGetProgramiv(shader.program, GL_LINK_STATUS, &success);
		if (!success) 
		{
			glGetProgramInfoLog(shader.program, 512, NULL, infoLog);
			assert(false);
		}

		return shader;
	}
	unsigned int LoadShaderSource(const char* file, const int type)
	{
		std::string str;
		std::ifstream stream(file);
		std::stringstream buffer;
		assert(stream.is_open());
		buffer << stream.rdbuf();
		str = buffer.str();
		stream.close();

		unsigned int handle = glCreateShader(type);
		const char* ptr = str.c_str();
		glShaderSource(handle, 1, &ptr, NULL);
		glCompileShader(handle);

		int success;
		char infoLog[512];

		glGetShaderiv(handle, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(handle, 512, NULL, infoLog);
			std::cout << "SHADER COMPILATION_FAILED IN " << file << "\n" << infoLog << std::endl;
			assert(false);
		}

		assert(success);
		return handle;
	}
	void LinkShader(Shader& shader)
	{
		shader.program = glCreateProgram();
		glAttachShader(shader.program, shader.vertex);
		glAttachShader(shader.program, shader.fragment);
		glLinkProgram(shader.program);
	}
	void DestroyShader(const Shader& shader)
	{
		glDeleteProgram(shader.program);
		glDeleteShader(shader.vertex);
		glDeleteShader(shader.fragment);
	}
}