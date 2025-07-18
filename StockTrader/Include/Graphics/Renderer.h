#pragma once
#include "Mesh.h"
#include "Shader.h"
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

	struct DrawLineGraphInfo final
	{
		glm::vec2 position;
		GraphPoint* points;
		uint32_t length;

		const char* title = nullptr;
		glm::vec2 scale{ 1 };
		float aspectRatio = 1;
		GraphType type = GraphType::line;
		bool noBackground = true;
		bool normalize = true;
		glm::vec4 color{1, 0, 0, 1};
		uint32_t stopAt = UINT32_MAX;
		uint32_t maxLinesDrawn = 100;
		bool textIsButton = false;
	};

	struct DrawDistributionGraphInfo final
	{
		glm::vec2 position;
		glm::vec4 color = glm::vec4(1, 0, 0, 1);
		uint32_t length;
		float* values;

		const char* title = nullptr;
		glm::vec2 scale{ 1 };
		float aspectRatio = 1;
		bool noBackground = false;
		bool textIsButton = false;
		bool inverse = false;
		float zoom = 1;
		uint32_t overrideCeiling = -1;
	};

	struct DrawScatterGraphInfo final
	{
		glm::vec2 position;
		glm::vec2* points;
		uint32_t* colorIndices = nullptr;
		glm::vec4* colors = nullptr;
		uint32_t length;

		const char* title = nullptr;
		glm::vec2 scale{ 1 };
		float aspectRatio = 1;
		bool noBackground = false;
		bool textIsButton = false;
		float pointSize = 4;
	};

	struct RenderProxy final
	{
		friend struct Renderer;

		__declspec(dllexport) void DrawPlane(glm::vec2 position, glm::vec2 scale, glm::vec4 color);
		__declspec(dllexport) void DrawLine(glm::vec2 start, glm::vec2 end, glm::vec4 color);
		__declspec(dllexport) void DrawPoint(glm::vec2 position, glm::vec4 color, float size);
		__declspec(dllexport) bool DrawScatterGraph(DrawScatterGraphInfo info);
		__declspec(dllexport) bool DrawDistributionGraph(DrawDistributionGraphInfo info);
		__declspec(dllexport) bool DrawLineGraph(DrawLineGraphInfo info);
		__declspec(dllexport) [[nodiscard]] float GetAspectRatio();

	private:
		Renderer* renderer;
	};

	struct Renderer final
	{
		glm::vec2 resolution;
		GLFWwindow* window;
		Mesh planeMesh;
		Mesh lineMesh;
		Mesh pointMesh;
		Shader defaultShader;
		Shader lineShader;
		Shader pointShader;
		uint32_t boundIndicesLength;

		float graphBorderThickness = .02f;
		float candleThickness = .6f;

		[[nodiscard]] bool Render();
		void Draw(VertType vertType);
		void EnableWireframe(bool enable);
		void BindShader(Shader shader);
		void BindMesh(Mesh mesh);

		[[nodiscard]] float GetAspectRatio();
		void SetLineWidth(float width);

		void DrawPlane(glm::vec2 position, glm::vec2 scale, glm::vec4 color);
		void DrawLine(glm::vec2 start, glm::vec2 end, glm::vec4 color);
		void DrawPoint(glm::vec2 position, glm::vec4 color, float size);
		bool DrawScatterGraph(DrawScatterGraphInfo info);
		bool DrawDistributionGraph(DrawDistributionGraphInfo info);
		bool DrawLineGraph(DrawLineGraphInfo info);

		[[nodiscard]] RenderProxy GetProxy();
	};

	[[nodiscard]] Renderer CreateRenderer(RendererCreateInfo info);
	void DestroyRenderer(const Renderer& renderer);
}