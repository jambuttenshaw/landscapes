#include "UserInterface.h"

#include "LandscapesApplication.h"


UIRenderer::UIRenderer(donut::app::DeviceManager* deviceManager, std::shared_ptr<LandscapesApplication> app, UIData& ui)
	: ImGui_Renderer(deviceManager)
	, m_App(std::move(app))
	, m_UI(ui)
{
}

void UIRenderer::buildUI()
{
	ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("Renderer: %s", GetDeviceManager()->GetRendererString());
	double frameTime = GetDeviceManager()->GetAverageFrameTimeSeconds();
	if (frameTime > 0.0)
		ImGui::Text("%.3f ms/frame (%.1f FPS)", frameTime * 1e3, 1.0 / frameTime);

	ImGui::Separator();

	{
		const char* modes[] = {
			"Lit",
			"Normals"
		};
		int currentItem = static_cast<int>(m_UI.ViewMode);
		ImGui::Combo("View Mode", &currentItem, modes, static_cast<int>(ViewModes::COUNT));
		m_UI.ViewMode = static_cast<ViewModes>(currentItem);
	}

	ImGui::Separator();

	ImGui::Checkbox("Wireframe", &m_UI.Wireframe);
	ImGui::Checkbox("Backface Culling", &m_UI.BackFaceCulling);

	ImGui::Separator();

	ImGui::Checkbox("Draw Terrain", &m_UI.DrawTerrain);
	ImGui::Checkbox("Draw Objects", &m_UI.DrawObjects);

	ImGui::SliderInt("Terrain LOD", &m_UI.TerrainLOD, 0, 9);
	ImGui::DragFloat("Terrain Height", &m_UI.TerrainHeight, 1.0f, 0.0f, 1000.0f);

	ImGui::Separator();

	ImGui::Text("Camera Position: %.1f, %.1f, %.1f", m_UI.CameraPosition.x, m_UI.CameraPosition.y, m_UI.CameraPosition.z);

	ImGui::Separator();

	{
		float azimuth, elevation, distance;
		cartesianToSpherical(m_UI.LightDirection, azimuth, elevation, distance);
		ImGui::SliderAngle("Sun Azimuth", &azimuth, -179.0f, 180.0f);
		ImGui::SliderAngle("Sun Elevation", &elevation, -89.0f, 0.0f);
		m_UI.LightDirection = sphericalToCartesian(azimuth, elevation, distance);
	}

	ImGui::End();
}
