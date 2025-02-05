#pragma once

namespace jv::gr 
{
	enum class MeshType 
	{
		mStatic,
		mDynamic
	};

	struct Mesh final
	{
		unsigned int vbo, ebo, vao;
		uint32_t indicesLength;
	};

	[[nodiscard]] Mesh CreateMesh(glm::vec3* vertices, unsigned int* indices, MeshType type, uint32_t vSize, uint32_t iSize);
	void DestroyMesh(const Mesh& mesh);
}
