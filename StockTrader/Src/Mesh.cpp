#include "pch.h"
#include "Mesh.h"

namespace jv::gr
{
	Mesh CreateMesh(glm::vec3* vertices, unsigned int* indices, 
		const MeshType type, const uint32_t vSize, const uint32_t iSize)
	{
		Mesh mesh{};
		mesh.indicesLength = iSize;

		int mType = -1;
		switch (type)
		{
		case MeshType::mStatic:
			mType = GL_STATIC_DRAW;
			break;
		case MeshType::mDynamic:
			mType = GL_DYNAMIC_DRAW;
			break;
		}

		glGenVertexArrays(1, &mesh.vao);
		glGenBuffers(1, &mesh.vbo);
		glGenBuffers(1, &mesh.ebo);
		glBindVertexArray(mesh.vao);

		glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vSize * 3, vertices, mType);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * iSize, indices, mType);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		return mesh;
	}
	void DestroyMesh(const Mesh& mesh)
	{
		glDeleteVertexArrays(1, &mesh.vao);
		glDeleteBuffers(1, &mesh.ebo);
		glDeleteBuffers(1, &mesh.vbo);
	}
}