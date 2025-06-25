#include "user_interface.h"

#include "landscapes_application.h"


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

	ImGui::Checkbox("Wireframe", &m_UI.Wireframe);
	ImGui::Checkbox("Backface Culling", &m_UI.BackFaceCulling);

	ImGui::Separator();

	ImGui::Checkbox("Draw Terrain", &m_UI.DrawTerrain);
	ImGui::Checkbox("Draw Objects", &m_UI.DrawObjects);

	ImGui::End();
}
