#include "pch.h"
#include "Graphics/Mesh.h"

namespace jv::gr
{
	Mesh CreateMesh(float* vertices, unsigned int* indices, 
		const VertType vertType, const MeshType renderType, 
		const uint32_t vSize, const uint32_t iSize)
	{
		Mesh mesh{};
		mesh.indicesLength = iSize;

		int mType = -1;
		switch (renderType)
		{
		case MeshType::mStatic:
			mType = GL_STATIC_DRAW;
			break;
		case MeshType::mDynamic:
			mType = GL_DYNAMIC_DRAW;
			break;
		}

		float vertSize;
		switch (vertType)
		{
		case VertType::triangle:
			vertSize = sizeof(float) * 3;
			break;
		case VertType::line:
			vertSize = sizeof(float);
			break;
		}

		glGenVertexArrays(1, &mesh.vao);
		glGenBuffers(1, &mesh.vbo);
		glGenBuffers(1, &mesh.ebo);
		glBindVertexArray(mesh.vao);

		glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
		glBufferData(GL_ARRAY_BUFFER, vertSize * vSize, vertices, mType);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * iSize, indices, mType);

		switch (vertType)
		{
		case VertType::triangle:
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertSize, nullptr);
			break;
		case VertType::line:
			glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, vertSize, nullptr);
			break;
		}
		
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