#include "pch.h"
#include "Graphics/Renderer.h"
#include <JLib/Math.h>
#include <Ext/ImGuiTextUtils.h>

namespace jv::gr
{
	void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	Renderer CreateRenderer(const RendererCreateInfo info)
	{
		Renderer renderer{};
		renderer.resolution = info.resolution;

		glfwInit();
		// Version 3 is nice and available on nearly all platforms.
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_RESIZABLE, info.resizeable ? GLFW_TRUE : GLFW_FALSE);

		renderer.window = glfwCreateWindow(info.resolution.x, info.resolution.y, info.title, NULL, NULL);
		assert(renderer.window);
		glfwMakeContextCurrent(renderer.window);

		const auto result = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		assert(result);
		glViewport(0, 0, info.resolution.x, info.resolution.y);
		glfwSetFramebufferSizeCallback(renderer.window, FramebufferSizeCallback);

		glm::vec3 planeVertices[] =
		{
			{0.5f,  0.5f, 0.0f },
			{0.5f, -0.5f, 0.0f },
			{-0.5f, -0.5f, 0.0f },
			{-0.5f,  0.5f, 0.0f }
		};
		unsigned int planeIndices[] =
		{
			0, 1, 3,
			1, 2, 3
		};

		float lineVertices[] =
		{
			0, 1
		};

		unsigned int lineIndices[] =
		{
			0, 1
		};

		float pointVertex = 0;
		unsigned int pointIndex = 0;

		renderer.planeMesh = gr::CreateMesh(reinterpret_cast<float*>(planeVertices), planeIndices, gr::VertType::triangle, gr::MeshType::mStatic, 4, 6);
		renderer.lineMesh = gr::CreateMesh(lineVertices, lineIndices, gr::VertType::line, gr::MeshType::mStatic, 2, 2);
		renderer.pointMesh = gr::CreateMesh(&pointVertex, &pointIndex, gr::VertType::points, gr::MeshType::mStatic, 1, 1);
		renderer.defaultShader = gr::LoadShader("Shaders/Triangle.vert", "Shaders/Triangle.frag");
		renderer.lineShader = gr::LoadShader("Shaders/Line.vert", "Shaders/Line.frag");
		renderer.pointShader = gr::LoadShader("Shaders/Point.vert", "Shaders/Point.frag");

		renderer.BindMesh(renderer.planeMesh);
		renderer.BindShader(renderer.defaultShader);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		ImGui::StyleColorsDark();
		ImGui_ImplGlfw_InitForOpenGL(renderer.window, true);
		ImGui_ImplOpenGL3_Init("#version 330");
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		glEnable(GL_PROGRAM_POINT_SIZE);
		
		return renderer;
	}
	void DestroyRenderer(const Renderer& renderer)
	{
		gr::DestroyMesh(renderer.planeMesh);
		gr::DestroyMesh(renderer.lineMesh);
		gr::DestroyMesh(renderer.pointMesh);
		gr::DestroyShader(renderer.defaultShader);
		gr::DestroyShader(renderer.lineShader);
		gr::DestroyShader(renderer.pointShader);
		glfwTerminate();
	}
	bool Renderer::Render()
	{
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		const bool shouldClose = glfwWindowShouldClose(window);
		if (shouldClose)
			return true;

		glfwSwapBuffers(window);
		glfwPollEvents();
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		return false;
	}
	void Renderer::Draw(const VertType vertType)
	{
		switch (vertType)
		{
		case VertType::triangle:
			glDrawElements(GL_TRIANGLES, boundIndicesLength, GL_UNSIGNED_INT, 0);
			break;
		case VertType::line:
			glDrawElements(GL_LINES, boundIndicesLength, GL_UNSIGNED_INT, 0);
			break;
		case VertType::points:
			glDrawElements(GL_POINTS, boundIndicesLength, GL_UNSIGNED_INT, 0);
			break;
		}
	}

	void Renderer::EnableWireframe(const bool enable)
	{
		if (enable)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	void Renderer::BindShader(const Shader shader)
	{
		glUseProgram(shader.program);
	}
	void Renderer::BindMesh(const Mesh mesh)
	{
		glBindVertexArray(mesh.vao);
		boundIndicesLength = mesh.indicesLength;
	}
	float Renderer::GetAspectRatio()
	{
		return resolution.y / resolution.x;
	}
	void Renderer::SetLineWidth(const float width)
	{
		glLineWidth(width);
	}
	void Renderer::DrawPlane(const glm::vec2 position, const glm::vec2 scale, const glm::vec4 color)
	{
		BindMesh(planeMesh);
		BindShader(defaultShader);
		gr::SetShaderUniform2f(defaultShader,
			gr::GetShaderUniform(defaultShader, "position"), position);
		gr::SetShaderUniform2f(defaultShader,
			gr::GetShaderUniform(defaultShader, "scale"), scale);
		gr::SetShaderUniform4f(defaultShader,
			gr::GetShaderUniform(defaultShader, "color"), color);
		Draw(VertType::triangle);
	}
	void Renderer::DrawLine(const glm::vec2 start, const glm::vec2 end, const glm::vec4 color)
	{
		BindMesh(lineMesh);
		BindShader(lineShader);
		gr::SetShaderUniform2f(lineShader,
			gr::GetShaderUniform(lineShader, "start"), start);
		gr::SetShaderUniform2f(lineShader,
			gr::GetShaderUniform(lineShader, "end"), end);
		gr::SetShaderUniform4f(lineShader,
			gr::GetShaderUniform(lineShader, "color"), color);
		Draw(VertType::line);
	}

	void Renderer::DrawPoint(const glm::vec2 position, const glm::vec4 color, const float size)
	{
		BindMesh(pointMesh);
		BindShader(pointShader);
		gr::SetShaderUniform2f(pointShader,
			gr::GetShaderUniform(pointShader, "pos"), position);
		gr::SetShaderUniform4f(pointShader,
			gr::GetShaderUniform(pointShader, "color"), color);
		gr::SetShaderUniform1f(pointShader,
			gr::GetShaderUniform(pointShader, "size"), size);
		
		Draw(VertType::points);
	}

	bool TryDrawTitle(const char* title, glm::vec2 position, glm::vec2 scale, float aspectRatio, bool interactable)
	{
		bool interacted = false;
		if (title)
		{
			glm::vec2 convPos = position;
			convPos += glm::vec2(1);
			convPos *= .5f;
			convPos.y = 1.f - convPos.y;

			convPos *= RESOLUTION;

			glm::vec2 winSize = { scale.x * RESOLUTION.x / 4, 36 };

			convPos.x -= winSize.x * aspectRatio;
			convPos.y += scale.y * RESOLUTION.y / 4;
			winSize.x *= 2;
			winSize.x *= aspectRatio;

			const float WIN_OFFSET = 6;

			ImGuiWindowFlags FLAGS = 0;
			//FLAGS |= ImGuiWindowFlags_NoBackground;
			FLAGS |= ImGuiWindowFlags_NoTitleBar;
			ImGui::Begin(title, nullptr, WIN_FLAGS | FLAGS);
			ImGui::SetWindowPos({ convPos.x, convPos.y + WIN_OFFSET });
			ImGui::SetWindowSize({ winSize.x, winSize.y });

			if (interactable)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
				interacted = ImGui::ButtonCenter(title);
				ImGui::PopStyleColor();
			}
			else
				ImGui::TextCenter(title);

			ImGui::End();
		}
		return interacted;
	}

	bool Renderer::DrawScatterGraph(const DrawScatterGraphInfo info)
	{
		glm::vec2 aspScale = info.scale * glm::vec2(info.aspectRatio, 1) * .5f;

		const auto pos = info.position;
		const auto c = glm::vec4(1);
		DrawLine(glm::vec2(pos.x - aspScale.x, pos.y), glm::vec2(pos.x + aspScale.x, pos.y), c);
		DrawLine(glm::vec2(pos.x, pos.y - aspScale.y), glm::vec2(pos.x, pos.y + aspScale.y), c);

		for (uint32_t i = 0; i < info.length; i++)
		{
			auto p = info.points[i];
			p *= info.aspectRatio;

			auto color = info.colors && info.colorIndices ? 
				info.colors[info.colorIndices[i]] : info.colors ? info.colors[0] : glm::vec4(0, 1, 0, 1);
			DrawPoint(pos + p, color, info.pointSize);
		}

		return TryDrawTitle(info.title, info.position, info.scale, info.aspectRatio, info.textIsButton);
	}

	bool Renderer::DrawDistributionGraph(const DrawDistributionGraphInfo info)
	{
		glm::vec2 aspScale = info.scale * glm::vec2(info.aspectRatio, 1) * .5f;

		const auto pos = info.position;
		const auto c = glm::vec4(1);
		DrawLine(glm::vec2(pos.x - aspScale.x, pos.y), glm::vec2(pos.x + aspScale.x, pos.y), c);
		DrawLine(glm::vec2(pos.x, pos.y - aspScale.y), glm::vec2(pos.x, pos.y + aspScale.y), c);

		const float stepSize = info.scale.x / info.length * aspScale.x * info.zoom;
		const float off = stepSize * (info.length - 1) / 2;
		const float org = -off + info.position.x;

		float ceiling = 0;
		for (uint32_t i = 0; i < info.length; i++)
			ceiling = Max(ceiling, info.values[i]);
		if (info.overrideCeiling != -1)
			ceiling = info.overrideCeiling;

		for (uint32_t i = 0; i < info.length; i++)
		{
			float xStart = org + stepSize * i;
			float xEnd = xStart + stepSize;

			const float height = (float)info.values[i] / ceiling * .5f;

			const float dir = (2 * !info.inverse - 1);
			const float yOrg = pos.y + height / 2 * dir;

			glm::vec2 a = glm::vec2(xStart, yOrg);
			glm::vec2 b = glm::vec2((xEnd - xStart), height);

			auto color = info.color;
			const float cMul = i % 2 == 0 ? 1 : .6f;
			color *= cMul;
			color.a = 1;

			DrawPlane(a, b, color);
		}

		return TryDrawTitle(info.title, info.position, info.scale, info.aspectRatio, info.textIsButton);
	}

	bool Renderer::DrawLineGraph(const DrawLineGraphInfo info)
	{
		if (info.length == 0)
			return false;

		glm::vec2 aspScale = info.scale * glm::vec2(info.aspectRatio, 1);

		if (!info.noBackground)
		{
			DrawPlane(info.position, aspScale, glm::vec4(1));
			DrawPlane(info.position, aspScale * (1.f - graphBorderThickness), glm::vec4(0));
		}

		const uint32_t l = info.stopAt == -1 ? info.length : info.stopAt;
		const uint32_t l2 = Min(info.length, info.maxLinesDrawn);
		const uint32_t len = Min(l, info.maxLinesDrawn);
		const float stepSize = (float)l / len;

		float lineWidth = 1.f / (l2 - 1) * aspScale.x;
		float off = lineWidth * (l2 - 1) / 2;
		float org = -off + info.position.x;
		float ceiling = 0;
		float floor = FLT_MAX;

		for (uint32_t i = 0; i < info.length; i++)
		{
			const auto& point = info.points[i];
			ceiling = jv::Max<float>(ceiling, point.high);
			floor = jv::Min<float>(floor, point.low);
		}

		if (!info.normalize)
			floor = Min<float>(0, floor);

		float pOpen = 0;
		float pClose = 0;
		float pHigh = 0;
		float pLow = 0;

		for (uint32_t i = 0; i < len; i++)
		{
			float xStart = org + lineWidth * (i - 1);
			float xEnd = xStart + lineWidth;

			const uint32_t curPt = round(stepSize * i);

			float open = 0;
			float close = 0;
			float high = 0;
			float low = 0;
			float divBy = 0;

			auto& point = info.points[curPt];

			open += point.open;
			close += point.close;
			high += point.high;
			low += point.low;

			const float yPos = jv::RLerp<float>(close, floor, ceiling) * aspScale.y - aspScale.y / 2;		

			if (i > 0)
			{
				if (info.type == GraphType::line)
				{
					const float yPosPrev = jv::RLerp<float>(pClose, floor, ceiling) * aspScale.y - aspScale.y / 2;
					DrawLine(glm::vec2(xStart, yPosPrev + info.position.y), glm::vec2(xEnd, yPos + info.position.y), info.color);
				}
				if (info.type == GraphType::candle)
				{
					const auto color = open < close ? glm::vec4(0, 1, 0, 1) : glm::vec4(1, 0, 0, 1);
					const float yPos2 = jv::RLerp<float>((open + close) / 2, floor, ceiling) * aspScale.y - aspScale.y / 2;

					const auto pos = glm::vec2(xStart + lineWidth / 2, yPos2);
					const float width = (xEnd - xStart) * candleThickness;
					const float height = (open - close) / (ceiling - floor) * aspScale.y;

					low = jv::RLerp<float>(low, floor, ceiling) * aspScale.y - aspScale.y / 2;
					high = jv::RLerp<float>(high, floor, ceiling) * aspScale.y - aspScale.y / 2;
					DrawLine(glm::vec2(pos.x, low + info.position.y), glm::vec2(pos.x, high + info.position.y), glm::vec4(1, 1, 1, 1));
					DrawPlane(pos + glm::vec2(0, info.position.y), glm::vec2(width, height), color);
				}
			}

			pOpen = open;
			pClose = close;
			pHigh = high;
			pLow = low;
		}

		bool interacted = false;

		if (info.title)
		{
			glm::vec2 convPos = info.position;
			convPos += glm::vec2(1);
			convPos *= .5f;
			convPos.y = 1.f - convPos.y;

			convPos *= RESOLUTION;

			glm::vec2 winSize = { info.scale.x * RESOLUTION.x / 4, 36 };

			convPos.x -= winSize.x * info.aspectRatio;
			convPos.y += info.scale.y * RESOLUTION.y / 4;
			winSize.x *= 2;
			winSize.x *= info.aspectRatio;

			const float WIN_OFFSET = 6;
			
			ImGuiWindowFlags FLAGS = ImGuiWindowFlags_NoFocusOnAppearing;
			//FLAGS |= ImGuiWindowFlags_NoBackground;
			FLAGS |= ImGuiWindowFlags_NoTitleBar;
			ImGui::Begin(info.title, nullptr, WIN_FLAGS | FLAGS);
			ImGui::SetWindowPos({ convPos.x, convPos.y + WIN_OFFSET });
			ImGui::SetWindowSize({ winSize.x, winSize.y });

			std::string text = info.title;
			const float start = info.points[0].close;
			const float end = info.points[l - 1].close;
			const float pct = end / start - 1.f;
			text += " [";
			text += pct >= 0 ? "+" : "-";
			text += std::to_string(static_cast<uint32_t>(abs(pct) * 100));
			text += "%]";

			ImVec4 tradeCol = pct >= 0 ? ImVec4{ 0, 1, 0, 1 } : ImVec4{ 1, 0, 0, 1 };
			ImGui::PushStyleColor(ImGuiCol_Text, tradeCol);
			
			if (info.textIsButton)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
				interacted = ImGui::ButtonCenter(text);
				ImGui::PopStyleColor();
			}			
			else
				ImGui::TextCenter(text);

			ImGui::PopStyleColor();
			ImGui::End();
		}

		return interacted;
	}
	RenderProxy Renderer::GetProxy()
	{
		RenderProxy proxy{};
		proxy.renderer = this;
		return proxy;
	}
	void RenderProxy::DrawPlane(glm::vec2 position, glm::vec2 scale, glm::vec4 color)
	{
		renderer->DrawPlane(position, scale, color);
	}
	void RenderProxy::DrawLine(glm::vec2 start, glm::vec2 end, glm::vec4 color)
	{
		renderer->DrawLine(start, end, color);
	}
	void RenderProxy::DrawPoint(glm::vec2 position, glm::vec4 color, float size)
	{
		renderer->DrawPoint(position, color, size);
	}
	bool RenderProxy::DrawScatterGraph(DrawScatterGraphInfo info)
	{
		return renderer->DrawScatterGraph(info);
	}
	bool RenderProxy::DrawDistributionGraph(DrawDistributionGraphInfo info)
	{
		return renderer->DrawDistributionGraph(info);
	}
	bool RenderProxy::DrawLineGraph(DrawLineGraphInfo info)
	{
		return renderer->DrawLineGraph(info);
	}
	float RenderProxy::GetAspectRatio()
	{
		return renderer->GetAspectRatio();
	}
}