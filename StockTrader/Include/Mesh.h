#pragma once

namespace jv::gr 
{
	struct Mesh final
	{
		unsigned int vbo, ebo, vao;
		uint32_t indicesLength;

		void Draw();
	};

	[[nodiscard]] Mesh LoadMesh(const char* vertices, const char* indices);
	[[nodiscard]] Mesh CreateMesh(glm::vec3* vertices, unsigned int* indices, glm::ivec2 size);
	void DestroyMesh(const Mesh& mesh);
}
