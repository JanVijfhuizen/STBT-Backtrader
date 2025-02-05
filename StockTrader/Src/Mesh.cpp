#include "pch.h"
#include "Mesh.h"

namespace jv::gr
{
	Mesh CreateMesh(glm::vec3* vertices, unsigned int* indices, const glm::ivec2 size)
	{
		Mesh mesh{};
		mesh.indicesLength = size.y;

		glGenBuffers(1, &mesh.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * size.x * 3, vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
		glEnableVertexAttribArray(0);

		glGenVertexArrays(1, &mesh.vao);
		glBindVertexArray(mesh.vao);
		glBindBuffer(GL_ARRAY_BUFFER, mesh.vao);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * size.x * 3, vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
		glEnableVertexAttribArray(0);

		glGenBuffers(1, &mesh.ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * size.y, indices, GL_STATIC_DRAW);

		return mesh;
	}
	void DestroyMesh(const Mesh& mesh)
	{
		glDeleteBuffers(1, &mesh.vao);
		glDeleteBuffers(1, &mesh.ebo);
		glDeleteBuffers(1, &mesh.vbo);
	}
	void Mesh::Draw()
	{
		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, indicesLength, GL_UNSIGNED_INT, 0);
	}
}