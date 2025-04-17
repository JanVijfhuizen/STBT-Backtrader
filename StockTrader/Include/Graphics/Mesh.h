#pragma once

namespace jv::gr 
{
	enum class MeshType 
	{
		mStatic,
		mDynamic
	};

	enum class VertType
	{
		line, triangle, points
	};

	struct Mesh final
	{
		unsigned int vbo, ebo, vao;
		uint32_t indicesLength;
	};

	[[nodiscard]] Mesh CreateMesh(float* vertices, unsigned int* indices, VertType vertType, MeshType renderType, uint32_t vSize, uint32_t iSize);
	void DestroyMesh(const Mesh& mesh);
}
