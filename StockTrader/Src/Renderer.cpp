#include "pch.h"
#include "Renderer.h"
#include <JLib/Math.h>

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
		
		return renderer;
	}
	void DestroyRenderer(Renderer& renderer)
	{
		gr::DestroyMesh(renderer.planeMesh);
		gr::DestroyMesh(renderer.lineMesh);
		gr::DestroyShader(renderer.defaultShader);
		gr::DestroyShader(renderer.lineShader);
		glfwTerminate();
	}
	bool Renderer::Render()
	{
		const bool shouldClose = glfwWindowShouldClose(window);
		if (shouldClose)
			return true;

		glfwSwapBuffers(window);
		glfwPollEvents();
		glClear(GL_COLOR_BUFFER_BIT);
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
	void Renderer::DrawGraph(const glm::vec2 position, const glm::vec2 scale, 
		GraphPoint* points, const uint32_t length, const GraphType type, const bool noBackground)
	{
		if (!noBackground)
		{
			DrawPlane(position, scale, glm::vec4(1));
			DrawPlane(position, scale * (1.f - graphBorderThickness), glm::vec4(0));
		}

		float lineWidth = 1.f / (length - 1) * scale.x;
		float org = -lineWidth * (length - 1) / 2 + position.x;
		float ceiling = 0;
		float floor = FLT_MAX;

		for (uint32_t j = 0; j < length; j++)
		{
			const auto& point = points[j];
			ceiling = jv::Max<float>(ceiling, point.high);
			floor = jv::Min<float>(floor, point.low);
		}

		for (uint32_t j = 1; j < length; j++)
		{
			float xStart = org + lineWidth * (j - 1);
			float xEnd = xStart + lineWidth;

			const auto& cur = points[j];
			const float yPos = jv::RLerp<float>(cur.close, floor, ceiling) * scale.y - scale.y / 2;
			const auto& prev = points[j - 1];
			const float yPosPrev = jv::RLerp<float>(prev.close, floor, ceiling) * scale.y - scale.y / 2;
			
			if (type == GraphType::line)
			{
				DrawLine(glm::vec2(xStart, yPosPrev), glm::vec2(xEnd, yPos), glm::vec4(1, 0, 0, 1));
			}
			if (type == GraphType::candle)
			{
				const float open = cur.open;
				const float close = cur.close;

				const auto color = open < close ? glm::vec4(0, 1, 0, 1) : glm::vec4(1, 0, 0, 1);
				const float yPos2 = jv::RLerp<float>((open + close) / 2, floor, ceiling) * scale.y - scale.y / 2;

				const auto pos = glm::vec2(xStart + lineWidth / 2, yPos2);
				const float width = (xEnd - xStart) * candleThickness;
				const float height = (open - close) / (ceiling - floor) * scale.y;

				// low/high
				float low = cur.low;
				float high = cur.high;
				
				low = jv::RLerp<float>(low, floor, ceiling) * scale.y - scale.y / 2;
				high = jv::RLerp<float>(high, floor, ceiling) * scale.y - scale.y / 2;
				DrawLine(glm::vec2(pos.x, low), glm::vec2(pos.x, high), glm::vec4(1, 1, 1, 1));

				DrawPlane(pos, glm::vec2(width, height), color);
			}
		}
	}
}