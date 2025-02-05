#pragma once
#include <Mesh.h>
#include <Shader.h>

namespace jv::gr 
{
	struct RendererCreateInfo final 
	{
		const char* title;
		glm::vec2 resolution{800, 600};
	};

	struct Renderer final
	{
		GLFWwindow* window;
		Mesh fallbackMesh;
		Shader fallbackShader;
		uint32_t boundIndicesLength;

		[[nodiscard]] bool Render();
		void Draw();
		void EnableWireframe(bool enable);
		void BindShader(Shader shader);
		void BindMesh(Mesh mesh);
	};

	[[nodiscard]] Renderer CreateRenderer(RendererCreateInfo info);
	void DestroyRenderer(Renderer& renderer);
}