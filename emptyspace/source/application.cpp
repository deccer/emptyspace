#include <emptyspace/types.hpp>
#include <emptyspace/application.hpp>
#include <emptyspace/graphics/attributeformat.hpp>
#include <emptyspace/graphics/geometry.hpp>
#include <emptyspace/graphics/instancebuffer.hpp>
#include <emptyspace/graphics/vertexpositioncolornormaluv.hpp>
#include <emptyspace/graphics/shader.hpp>
#include <emptyspace/math/camera.hpp>
#include <emptyspace/physics.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <vector>
#include <iostream>
#include <sstream>

void OnFramebufferResized(GLFWwindow* window, int width, int height);
void OnMouseMove(GLFWwindow* window, double xpos, double ypos);

static double oldxpos;
static double oldypos;

Application::Application()
	: _window(nullptr), _windowHeight(720), _windowWidth(1280)
{
	if (!glfwInit())
	{
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	_window = glfwCreateWindow(_windowWidth, _windowHeight, "emptyspace", nullptr, nullptr);
	if (_window == nullptr)
	{
	}

	if (glfwRawMouseMotionSupported())
	{
		glfwSetInputMode(_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}

	glfwSetFramebufferSizeCallback(_window, OnFramebufferResized);
	glfwSetCursorPosCallback(_window, OnMouseMove);

	const auto primaryMonitor = glfwGetPrimaryMonitor();
	int screenWidth;
	int screenHeight;
	int xpos;
	int ypos;

	glfwGetMonitorWorkarea(primaryMonitor, &xpos, &ypos, &screenWidth, &screenHeight);
	glfwSetWindowPos(_window, screenWidth / 2 - (_windowWidth / 2), screenHeight / 2 - (_windowHeight / 2));

	glfwMakeContextCurrent(_window);
	glfwSwapInterval(1);
	if (!gladLoadGL())
	{
	}

	_physicsScene = new PhysicsScene();
}

Application::~Application()
{
	glfwDestroyWindow(_window);
	_window = nullptr;
	glfwTerminate();
}

void Application::Run()
{
	Initialize();

	auto t1 = glfwGetTime();

	auto deltaTimeAverage = 0.0f;
	auto deltaTimeAverageSquared = 0.0f;

	auto framesToAverage = 100;
	auto frameCounter = 0;

	glfwSwapInterval(0);

	while (!glfwWindowShouldClose(_window))
	{
		const auto t2 = glfwGetTime();
		const auto deltaTime = f32(t2 - t1);
		t1 = t2;

		deltaTimeAverage += deltaTime;
		deltaTimeAverageSquared += (deltaTime * deltaTime);
		frameCounter++;

		if (frameCounter == framesToAverage)
		{
			deltaTimeAverage /= framesToAverage;
			deltaTimeAverageSquared /= framesToAverage;
			const auto deltaTimeStandardError = sqrt(deltaTimeAverageSquared - deltaTimeAverage * deltaTimeAverage) /
				sqrt(framesToAverage);

			char str[76];
			sprintf_s(str, "emptyspace, frame = %.3fms +/- %.4fms, fps = %.1f, %d frames", (deltaTimeAverage * 1000.0f),
			          1000.0f * deltaTimeStandardError, 1.0f / deltaTimeAverage, framesToAverage);
			glfwSetWindowTitle(_window, str);

			framesToAverage = static_cast<int>(1.0f / deltaTimeAverage);

			deltaTimeAverage = 0.0f;
			deltaTimeAverageSquared = 0.0f;
			frameCounter = 0;
		}

		Update(deltaTime);
		Draw(deltaTime);

		glfwSwapBuffers(_window);
	}

	Cleanup();
}

void Application::Cleanup() const
{
	delete _planeGeometry;
	delete _cubeGeometry;

	delete _basicShader;
	delete _basicShaderInstanced;

	delete _asteroidInstanceBuffer;
}

static float angle = 0.0f;

void Application::Draw(const f32 deltaTime) const
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	_basicShader->Use();
	
	_basicShader->SetValue("u_model", glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 0.0f)));
	_planeGeometry->Bind();
	_planeGeometry->DrawElements();

	angle += static_cast<float>(deltaTime);
	const auto rotationMatrix = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
	_basicShader->SetValue("u_model", glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * rotationMatrix);
	_cubeGeometry->Bind();
	_cubeGeometry->DrawElements();
	
	_basicShaderInstanced->Use();
	_cubeGeometry->Bind();
	_asteroidInstanceBuffer->Bind();
	_cubeGeometry->DrawElementsInstanced(1000);
}

void Application::HandleInput(const f32 deltaTime) const
{
	if (glfwGetKey(_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(_window, true);
	}

	_camera->MovementSpeed = SPEED;
	if (glfwGetKey(_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
	{
		_camera->MovementSpeed = SPEED * 40;
	}

	if (glfwGetKey(_window, GLFW_KEY_W) == GLFW_PRESS)
	{
		_physicsScene->Boost(Direction::Forward);
		_camera->ProcessKeyboard(CameraMovement::Forward, deltaTime);
	}
	
	if (glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS)
	{
		_physicsScene->Boost(Direction::Backward);
		_camera->ProcessKeyboard(CameraMovement::Backward, deltaTime);
	}
	
	if (glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS)
	{
		_physicsScene->Boost(Direction::Left);
		_camera->ProcessKeyboard(CameraMovement::Left, deltaTime);
	}
	
	if (glfwGetKey(_window, GLFW_KEY_D) == GLFW_PRESS)
	{
		_physicsScene->Boost(Direction::Right);
		_camera->ProcessKeyboard(CameraMovement::Right, deltaTime);
	}
	
	if (glfwGetKey(_window, GLFW_KEY_SPACE) == GLFW_PRESS)
	{
		_physicsScene->Boost(Direction::Up);
		_camera->ProcessKeyboard(CameraMovement::Up, deltaTime);
	}
	
	if (glfwGetKey(_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
	{
		_physicsScene->Boost(Direction::Down);
		_camera->ProcessKeyboard(CameraMovement::Down, deltaTime);
	}
}

// shamelessly stolen from the learnopengl tutorial
std::vector<glm::mat4> CreateAsteroidInstances(s32 instanceCount)
{
	std::vector<glm::mat4> modelMatrices;

	srand(glfwGetTime()); // initialize random seed	
	const auto radius = 50.0f;
	const auto offset = 25.5f;
	for (unsigned int i = 0; i < instanceCount; i++)
	{
		auto model = glm::mat4(1.0f);
		// 1. translation: displace along circle with 'radius' in range [-offset, offset]
		const auto angle = static_cast<float>(i) / static_cast<float>(instanceCount) * 360.0f;
		auto displacement = (rand() % static_cast<int>(2.0f * offset * 100)) / 100.0f - offset;
		
		const auto x = sin(angle) * radius + displacement;
		displacement = (rand() % static_cast<int>(2.0f * offset * 100)) / 100.0f - offset;
		
		const auto y = displacement * 0.4f; // keep height of field smaller compared to width of x and z
		displacement = (rand() % static_cast<int>(2.0f * offset * 100)) / 100.0f - offset;
		
		const auto z = cos(angle) * radius + displacement;
		model = glm::translate(model, glm::vec3(x, y, z));

		// 2. scale: Scale between 0.05 and 0.25f
		const auto scale = (rand() % 60) / 100.0f + 0.05;
		model = glm::scale(model, glm::vec3(scale));

		// 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
		const auto rotAngle = f32(rand() % 360);
		model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

		// 4. now add to list of matrices
		modelMatrices.push_back(model);
	}

	return modelMatrices;
}

void Application::Initialize()
{
	std::clog << glGetString(GL_VERSION) << '\n';

	glViewport(0, 0, _windowWidth, _windowHeight);
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.05f, 0.05f, 0.05f, 0.05f);

	const std::vector<VertexPositionColorNormalUv> planeVertices =
	{
		VertexPositionColorNormalUv(glm::vec3(-0.5f, 0.5f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f)),
		VertexPositionColorNormalUv(glm::vec3(0.5f, 0.5f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f)),
		VertexPositionColorNormalUv(glm::vec3(0.5f, -0.5f, -1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f)),
		VertexPositionColorNormalUv(glm::vec3(-0.5f, -0.5f, -1.0f), glm::vec3(1.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 1.0f)),
	};

	const std::vector<VertexPositionColorNormalUv> cubeVertices =
	{
		VertexPositionColorNormalUv(glm::vec3(-0.5f, 0.5f,-0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f,-1.0f), glm::vec2(0.0f, 0.0f)),
		VertexPositionColorNormalUv(glm::vec3(0.5f, 0.5f,-0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f,-1.0f), glm::vec2(1.0f, 0.0f)),
		VertexPositionColorNormalUv(glm::vec3(0.5f,-0.5f,-0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f,-1.0f), glm::vec2(1.0f, 1.0f)),
		VertexPositionColorNormalUv(glm::vec3(-0.5f,-0.5f,-0.5f), glm::vec3(1.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f,-1.0f), glm::vec2(0.0f, 1.0f)),

		VertexPositionColorNormalUv(glm::vec3(0.5f, 0.5f,-0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)),
		VertexPositionColorNormalUv(glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f)),
		VertexPositionColorNormalUv(glm::vec3(0.5f,-0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f)),
		VertexPositionColorNormalUv(glm::vec3(0.5f,-0.5f,-0.5f), glm::vec3(1.0f, 0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f)),

		VertexPositionColorNormalUv(glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f)),
		VertexPositionColorNormalUv(glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f)),
		VertexPositionColorNormalUv(glm::vec3(-0.5f,-0.5f, 0.5f), glm::vec3(1.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)),
		VertexPositionColorNormalUv(glm::vec3(0.5f,-0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f)),

		VertexPositionColorNormalUv(glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f)),
		VertexPositionColorNormalUv(glm::vec3(-0.5f, 0.5f,-0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)),
		VertexPositionColorNormalUv(glm::vec3(-0.5f,-0.5f,-0.5f), glm::vec3(1.0f, 0.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f)),
		VertexPositionColorNormalUv(glm::vec3(-0.5f,-0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f)),

		VertexPositionColorNormalUv(glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f)),
		VertexPositionColorNormalUv(glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f)),
		VertexPositionColorNormalUv(glm::vec3(0.5f, 0.5f,-0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f)),
		VertexPositionColorNormalUv(glm::vec3(-0.5f, 0.5f,-0.5f), glm::vec3(1.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 1.0f)),

		VertexPositionColorNormalUv(glm::vec3(0.5f,-0.5f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f,-1.0f, 0.0f), glm::vec2(1.0f, 0.0f)),
		VertexPositionColorNormalUv(glm::vec3(-0.5f,-0.5f, 0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f,-1.0f, 0.0f), glm::vec2(0.0f, 0.0f)),
		VertexPositionColorNormalUv(glm::vec3(-0.5f,-0.5f,-0.5f), glm::vec3(1.0f, 0.0f, 1.0f), glm::vec3(0.0f,-1.0f, 0.0f), glm::vec2(0.0f, 1.0f)),
		VertexPositionColorNormalUv(glm::vec3(0.5f,-0.5f,-0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f,-1.0f, 0.0f), glm::vec2(1.0f, 1.0f)),
	};

	const std::vector<u8> planeIndices =
	{
		0, 1, 2, 2, 3, 0,
	};

	const std::vector<u8> cubeIndices =
	{
		0,   1,  2,  2,  3,  0,
		4,   5,  6,  6,  7,  4,
		8,   9, 10, 10, 11,  8,

		12, 13, 14, 14, 15, 12,
		16, 17, 18, 18, 19, 16,
		20, 21, 22, 22, 23, 20,
	};

	const std::vector<AttributeFormat> geometryVertexFormat =
	{
		CreateAttributeFormat<glm::vec3>(0, offsetof(VertexPositionColorNormalUv, Position)),
		CreateAttributeFormat<glm::vec3>(1, offsetof(VertexPositionColorNormalUv, Color)),
		CreateAttributeFormat<glm::vec3>(2, offsetof(VertexPositionColorNormalUv, Normal)),
		CreateAttributeFormat<glm::vec2>(3, offsetof(VertexPositionColorNormalUv, Uv))
	};

	const auto asteroidInstances = CreateAsteroidInstances(10000);
	
	_asteroidInstanceBuffer = new InstanceBuffer();
	_asteroidInstanceBuffer->UpdateBuffer(asteroidInstances);

	_planeGeometry = new Geometry(planeVertices, planeIndices, geometryVertexFormat);
	_cubeGeometry = new Geometry(cubeVertices, cubeIndices, geometryVertexFormat);

	_basicShader = new Shader("res/shaders/basic.vs.glsl", "res/shaders/basic.ps.glsl");
	_basicShaderInstanced = new Shader("res/shaders/basic_instanced.vs.glsl", "res/shaders/basic.ps.glsl");

	_camera = new Camera(glm::vec3{0, 0, 5});
	_camera->MouseSensitivity = 0.1f;
	_projection = glm::perspective(glm::pi<f32>() / 4.0f,
	                               static_cast<float>(_windowWidth) / static_cast<float>(_windowHeight), 0.1f, 512.0f);

	oldxpos = _windowWidth / 2.0f;
	oldypos = _windowHeight / 2.0f;
	glfwSetCursorPos(_window, oldxpos, oldypos);
	glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowUserPointer(_window, _camera);
}

void Application::Update(const f32 deltaTime) const
{
	glfwPollEvents();
	HandleInput(deltaTime);

	_physicsScene->Step(deltaTime);
	_camera->Position = _physicsScene->Fetch();

	_basicShader->Use();
	_basicShader->SetValue("u_model", glm::mat4x4(1.0f));
	_basicShader->SetValue("u_view", _camera->GetViewMatrix());
	_basicShader->SetValue("u_projection", _projection);

	_basicShaderInstanced->Use();
	_basicShaderInstanced->SetValue("u_view", _camera->GetViewMatrix());
	_basicShaderInstanced->SetValue("u_projection", _projection);
}

void OnFramebufferResized(GLFWwindow* window, const int width, const int height)
{
	glViewport(0, 0, width, height);
}

void OnMouseMove(GLFWwindow* window, const double xpos, const double ypos)
{
	const auto camera = reinterpret_cast<Camera*>(glfwGetWindowUserPointer(window));
	if (camera != nullptr)
	{
		camera->ProcessMouseMovement(f32(xpos - oldxpos), f32(oldypos - ypos));
		oldxpos = xpos;
		oldypos = ypos;
	}
}
