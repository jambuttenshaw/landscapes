#pragma once

#include <donut/app/imgui_renderer.h>


class LandscapesApplication;

// Data shared between the application layer and the user interface layer
struct UIData
{
	bool Wireframe = true;
	bool BackFaceCulling = true;

	bool DrawTerrain = true;
	bool DrawObjects = false;

	int TerrainLOD = 0;
};


class UIRenderer : public donut::app::ImGui_Renderer
{
public:
	UIRenderer(donut::app::DeviceManager* deviceManager, std::shared_ptr<LandscapesApplication> app, UIData& ui);

protected:
	virtual void buildUI() override;

private:
	std::shared_ptr<LandscapesApplication> m_App;
	UIData& m_UI;
};
