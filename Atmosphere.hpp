#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/noise.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <EditableGame.hpp>
#include <Primitives.hpp>
#include <Graphics/VulkanDevice.hpp>

class Atmosphere : public fe::EditableGame {
public:

	bool showDebugUI = false;
	bool freeCamera = true;
	float freeCamSpeed = 15.0f;
	float farPlane = 10000.0f;
	float orbitAngle = 0.0f;

	// Terrain params
	float globeRadius = 5.0f;
	int sectorCount = 128;
	int stackCount = 64;
	float terrainAmplitude = 0.6f;
	float terrainFrequency = 0.8f;
	int terrainOctaves = 6;
	float persistence = 0.45f;
	float lacunarity = 2.1f;
	unsigned int terrainSeed = 42;

	float waterLevel = 0.15f;
	std::shared_ptr<fe::Object<>> globeObject;

	// Flight simulator
	float flightVelocity = 30.0f;
	float planeAltitude = 0.8f;
	float planeSize = 1.0f;
	std::shared_ptr<fe::Object<>> planeObject;

	Atmosphere(int width = 1000, int height = 1000, bool vr = false) : fe::EditableGame(fe::XRGameOptions(width, height, vr)) {

		SetClearColor(0.05f, 0.05f, 0.15f);

		if (!useVulkan)
			LoadShaders("resources/shaders/VertexShader.glsl", "resources/shaders/FragmentShader.glsl");

		LoadModels();

		GetPhysicsEngine()->EnableGravity();
	}

	void OnPreSwap() override {}

	fe::Mesh<> GenerateTerrainSphere() {
		const float PI = 3.14159265359f;
		std::vector<fe::Vertex> vertices;
		std::vector<uint32_t> indices;

		for (int stack = 0; stack <= stackCount; stack++) {
			float phi = PI * stack / stackCount;
			float v = (float)stack / stackCount;
			for (int sector = 0; sector <= sectorCount; sector++) {
				float theta = 2.0f * PI * sector / sectorCount;
				float u = (float)sector / sectorCount;

				// Spherical coordinates
				float nx = sin(phi) * cos(theta);
				float ny = cos(phi);
				float nz = sin(phi) * sin(theta);

				// Multi-octave noise for terrain height
				float noiseVal = 0.0f;
				float amplitude = 1.0f;
				float frequency = terrainFrequency;
				float maxAmplitude = 0.0f;

				for (int o = 0; o < terrainOctaves; o++) {
					noiseVal += glm::perlin(glm::vec2(nx * frequency + (float)terrainSeed,
					                                   nz * frequency + (float)terrainSeed)) * amplitude;
					maxAmplitude += amplitude;
					amplitude *= persistence;
					frequency *= lacunarity;
				}
				noiseVal /= maxAmplitude;

				// Normalize to 0..1
				float h = noiseVal * 0.5f + 0.5f;

				// Displace along normal
				float r = globeRadius + h * terrainAmplitude;

				float x = r * nx;
				float y = r * ny;
				float z = r * nz;

				// Normal will be recalculated below
				vertices.push_back(fe::Vertex(x, y, z, nx, ny, nz, u, v));
			}
		}

		// Build indices
		for (int stack = 0; stack < stackCount; stack++) {
			for (int sector = 0; sector < sectorCount; sector++) {
				int current = stack * (sectorCount + 1) + sector;
				int next = current + sectorCount + 1;
				indices.push_back(current);
				indices.push_back(current + 1);
				indices.push_back(next);
				indices.push_back(current + 1);
				indices.push_back(next + 1);
				indices.push_back(next);
			}
		}

		// Recalculate normals using finite differences
		for (int stack = 0; stack <= stackCount; stack++) {
			for (int sector = 0; sector <= sectorCount; sector++) {
				int idx = stack * (sectorCount + 1) + sector;
				int idxRight = stack * (sectorCount + 1) + ((sector + 1) % (sectorCount + 1));
				int idxLeft  = stack * (sectorCount + 1) + ((sector - 1 + sectorCount + 1) % (sectorCount + 1));
				int idxDown = std::max(0, stack - 1) * (sectorCount + 1) + sector;
				int idxUp   = std::min(stackCount, stack + 1) * (sectorCount + 1) + sector;

				glm::vec3 tangent = vertices[idxRight].position - vertices[idxLeft].position;
				glm::vec3 bitangent = vertices[idxUp].position - vertices[idxDown].position;
				glm::vec3 normal = glm::normalize(glm::cross(tangent, bitangent));

				// Ensure normal points outward
				if (glm::dot(normal, vertices[idx].position) < 0.0f)
					normal = -normal;

				vertices[idx].normal = normal;
			}
		}

		return fe::Mesh<>(vertices, indices);
	}

	void LoadModels() {

		// Globe with procedural terrain
		auto globeMesh = GenerateTerrainSphere();
		globeObject = std::make_shared<fe::Object<>>(globeMesh);
		globeObject->name = "Globe";
		globeObject->state.position = glm::vec3(0.0f);
		globeObject->color = glm::vec3(0.3f, 0.6f, 0.3f);
		this->scene->AddObject(globeObject);

		// Flight plane (simple rectangle)
		auto planeMesh = fe::Primitives::GeneratePlane(planeSize, planeSize);
		planeObject = std::make_shared<fe::Object<>>(planeMesh);
		planeObject->name = "Plane";
		planeObject->state.position = glm::vec3(0.0f, globeRadius + planeAltitude, 0.0f);
		planeObject->color = glm::vec3(0.0f, 0.0f, 0.0f);
		this->scene->AddObject(planeObject);

		// Player
		this->player = std::make_shared<fe::Character>();
		this->scene->AddObject(player);
		this->player->state.position = glm::vec3(0.0f, 0.0f, globeRadius * 3.0f);
		this->player->gravityEnabled = false;
	}

	void SyncCameraToPlayer() {
		if (!player || freeCamera) return;
		camera->SetPos(player->state.position + glm::vec3(0.0f, 1.6f, 0.0f));
	}

	void ProcessInput() {
		SDL_Event event;
		fe::SDLWindow *window = (fe::SDLWindow*)this->window.get();
		while (window->PollSDLEvent(&event)) {
			ImGui_ImplSDL3_ProcessEvent(&event);
			auto io = ImGui::GetIO();
			switch (event.type) {
				case SDL_EVENT_QUIT:
					window->PrepareClose();
					break;
				case SDL_EVENT_MOUSE_BUTTON_DOWN:
					if (event.button.button == SDL_BUTTON_LEFT && !io.WantCaptureMouse) {
						window->StartMouseCapture();
					}
					break;
			case SDL_EVENT_WINDOW_RESIZED:
			case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
				break;
				case SDL_EVENT_MOUSE_MOTION:
				{
					if (!window->IsCapturingMouse()) break;
					float sensitivity = 0.1f;
					camera->yaw   += event.motion.xrel * sensitivity;
					camera->pitch -= event.motion.yrel * sensitivity;
					camera->UpdateDirection();
					camera->pitch = std::clamp(camera->pitch, -89.0f, 89.0f);
					break;
				}
				case SDL_EVENT_KEY_DOWN:
					if (event.key.key == SDLK_F11) {
						window->ToggleFullscreen();
					}
					else if (event.key.key == SDLK_F3) {
						showDebugUI = !showDebugUI;
					}
					else if (event.key.key == SDLK_F2) {
						freeCamera = !freeCamera;
						window->StartMouseCapture();
					}
					break;
			}
		}

		if (window->IsKeyDown(SDL_SCANCODE_ESCAPE)) window->StopMouseCapture();
		if (ImGui::GetIO().WantCaptureMouse) window->StopMouseCapture();
	}

	void Run() {
		auto window = this->GetWindow<fe::SDLWindow>();
		window->Show();
		window->DisableVSync();

		camera->farDist = farPlane;
		camera->SetAspect(camera->aspect);
		camera->SetPos(glm::vec3(0.0f, globeRadius + planeAltitude + 1.5f, -4.0f));
		camera->pitch = -20.0f;
		camera->yaw = 90.0f;
		camera->UpdateDirection();

		while (!window->ShouldClose()) {

			ProcessInput();

			double dt = fpsCounter.deltaTime;

			if (freeCamera) {
				float spd = freeCamSpeed * dt;
				glm::vec3 cp = camera->GetPos();
				glm::vec3 right = glm::normalize(glm::cross(camera->front, camera->up));
				if (window->IsKeyDown(SDL_SCANCODE_W)) cp += camera->front * spd;
				if (window->IsKeyDown(SDL_SCANCODE_S)) cp -= camera->front * spd;
				if (window->IsKeyDown(SDL_SCANCODE_A)) cp -= right * spd;
				if (window->IsKeyDown(SDL_SCANCODE_D)) cp += right * spd;
				if (window->IsKeyDown(SDL_SCANCODE_SPACE)) cp += camera->up * spd;
				if (window->IsKeyDown(SDL_SCANCODE_LSHIFT)) cp -= camera->up * spd;
				camera->SetPos(cp);
			}

			// Rotate globe to simulate forward flight
			globeObject->state.rotation.x -= flightVelocity * dt;

			// Keep plane above the globe
			if (planeObject) {
				planeObject->state.position = glm::vec3(0.0f, globeRadius + planeAltitude, 0.0f);
			}

			Update();
			Redraw();
		}

		Destroy();
	}

	void InitUI() override {}

	void DrawUI() override {
		if (!showDebugUI) return;
		BeginFrame();

		DrawDebugUI();

		ImGui::Begin("Terrain");
		ImGui::SliderFloat("Amplitude", &terrainAmplitude, 0.0f, 3.0f);
		ImGui::SliderFloat("Frequency", &terrainFrequency, 0.1f, 5.0f);
		ImGui::SliderInt("Octaves", &terrainOctaves, 1, 10);
		ImGui::SliderFloat("Persistence", &persistence, 0.0f, 1.0f);
		ImGui::SliderFloat("Lacunarity", &lacunarity, 1.0f, 4.0f);
		ImGui::SliderFloat("Radius", &globeRadius, 1.0f, 20.0f);
		ImGui::SliderFloat("Water Level", &waterLevel, 0.0f, 1.0f);
		if (ImGui::Button("Regenerate")) {
			terrainSeed = static_cast<unsigned int>(rand());
			if (globeObject) {
				globeObject->meshes.clear();
				globeObject->meshes.push_back(GenerateTerrainSphere());
			}
		}
		ImGui::End();

		ImGui::Begin("Flight");
		ImGui::SliderFloat("Velocity", &flightVelocity, 0.0f, 200.0f);
		ImGui::SliderFloat("Altitude", &planeAltitude, 0.1f, 5.0f);
		if (ImGui::Button("Reset Rotation")) {
			globeObject->state.rotation.x = 0.0f;
		}
		ImGui::End();

		EndFrame();
	}
};
