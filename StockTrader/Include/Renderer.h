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
		glm::vec2 resolution;
		GLFWwindow* window;
		Mesh planeMesh;
		Mesh lineMesh;
		Shader defaultShader;
		Shader lineShader;
		uint32_t boundIndicesLength;

		[[nodiscard]] bool Render();
		void Draw(VertType vertType);
		void EnableWireframe(bool enable);
		void BindShader(Shader shader);
		void BindMesh(Mesh mesh);

		[[nodiscard]] float GetAspectRatio();

		void DrawPlane(glm::vec2 position, glm::vec2 scale, glm::vec4 color);
		void DrawLine(glm::vec2 start, glm::vec2 end, glm::vec4 color);
	};

	[[nodiscard]] Renderer CreateRenderer(RendererCreateInfo info);
	void DestroyRenderer(Renderer& renderer);
}