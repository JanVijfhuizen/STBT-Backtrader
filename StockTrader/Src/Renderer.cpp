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

		renderer.planeMesh = gr::CreateMesh(reinterpret_cast<float*>(planeVertices), planeIndices, gr::VertType::triangle, gr::MeshType::mStatic, 4, 6);
		renderer.lineMesh = gr::CreateMesh(lineVertices, lineIndices, gr::VertType::line, gr::MeshType::mStatic, 2, 2);
		renderer.defaultShader = gr::LoadShader("Shaders/Triangle.vert", "Shaders/Triangle.frag");
		renderer.lineShader = gr::LoadShader("Shaders/Line.vert", "Shaders/Line.frag");

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
		
		return renderer;
	}
	void DestroyRenderer(const Renderer& renderer)
	{
		gr::DestroyMesh(renderer.planeMesh);
		gr::DestroyMesh(renderer.lineMesh);
		gr::DestroyShader(renderer.defaultShader);
		gr::DestroyShader(renderer.lineShader);
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
	bool Renderer::DrawGraph(DrawGraphInfo info)
	{
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
		float org = -lineWidth * (l2 - 1) / 2 + info.position.x;
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
			
			ImGuiWindowFlags FLAGS = 0;
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
			text += "%%]";

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
}