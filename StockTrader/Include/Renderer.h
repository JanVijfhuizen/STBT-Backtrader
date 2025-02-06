#pragma once
#include <Mesh.h>
#include <Shader.h>

namespace jv::gr 
{
	enum class GraphType 
	{
		line,
		candle
	};

	struct GraphPoint final
	{
		float open;
		float high;
		float low;
		float close;
	};

	struct RendererCreateInfo final 
	{
		const char* title;
		glm::vec2 resolution{800, 600};
		bool resizeable = false;
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

		float graphBorderThickness = .05f;
		float candleThickness = .6f;

		__declspec(dllexport) [[nodiscard]] bool Render();
		__declspec(dllexport) void Draw(VertType vertType);
		__declspec(dllexport) void EnableWireframe(bool enable);
		__declspec(dllexport) void BindShader(Shader shader);
		__declspec(dllexport) void BindMesh(Mesh mesh);

		__declspec(dllexport) [[nodiscard]] float GetAspectRatio();

		__declspec(dllexport) void DrawPlane(glm::vec2 position, glm::vec2 scale, glm::vec4 color);
		__declspec(dllexport) void DrawLine(glm::vec2 start, glm::vec2 end, glm::vec4 color);
		__declspec(dllexport) void DrawGraph(glm::vec2 position, glm::vec2 scale, GraphPoint* points, 
			uint32_t length, GraphType type, bool noBackground);
	};

	__declspec(dllexport) [[nodiscard]] Renderer CreateRenderer(RendererCreateInfo info);
	__declspec(dllexport) void DestroyRenderer(Renderer& renderer);
}