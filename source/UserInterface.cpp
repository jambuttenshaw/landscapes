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
			"Unlit",
			"Normals"
		};
		int currentItem = static_cast<int>(m_UI.ViewMode);
		ImGui::Combo("View Mode", &currentItem, modes, static_cast<int>(ViewModes::COUNT));
		m_UI.ViewMode = static_cast<ViewModes>(currentItem);
	}

	ImGui::Separator();

	ImGui::Checkbox("Wireframe", &m_UI.Wireframe);

	ImGui::Separator();

	ImGui::Checkbox("Draw Terrain", &m_UI.DrawTerrain);
	ImGui::Checkbox("Update Terrain", &m_UI.UpdateTerrain);

	ImGui::SliderInt("Subdivision Level", &m_UI.TerrainSubdivisionLevel, 1, 5);
	ImGui::SliderFloat("Primitive Pixel Length", &m_UI.TerrainPrimitivePixelLength, 1.0f, 15.0f);

	ImGui::Separator();

	ImGui::Text("Camera Position: %.1f, %.1f, %.1f", m_UI.CameraPosition.x, m_UI.CameraPosition.y, m_UI.CameraPosition.z);

	ImGui::Separator();

	{
		float azimuth, elevation, distance;
		cartesianToSpherical(m_UI.LightDirection, azimuth, elevation, distance);
		ImGui::SliderAngle("Sun Azimuth", &azimuth, -179.0f, 180.0f, "%.1f deg");
		ImGui::SliderAngle("Sun Elevation", &elevation, -89.0f, 0.0f, "%.1f deg");
		m_UI.LightDirection = sphericalToCartesian(azimuth, elevation, distance);
	}

	ImGui::Separator();

	ImGui::DragFloat3("Plane Origin", &m_UI.DebugPlaneOrigin.x, 0.1f);
	{
		float azimuth, elevation, distance;
		cartesianToSpherical(m_UI.DebugPlaneNormal, azimuth, elevation, distance);
		ImGui::SliderAngle("Plane Azimuth", &azimuth, -179.0f, 180.0f, "%.1f deg");
		ImGui::SliderAngle("Plane Elevation", &elevation, -89.9f, 89.9f, "%.1f deg");
		m_UI.DebugPlaneNormal = sphericalToCartesian(azimuth, elevation, distance);

		ImGui::Text("%f, %f, %f", m_UI.DebugPlaneNormal.x, m_UI.DebugPlaneNormal.y, m_UI.DebugPlaneNormal.z);
	}

	ImGui::End();
}
