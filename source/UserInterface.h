#pragma once

#include <donut/core/math/math.h>
#include <donut/app/imgui_renderer.h>


class LandscapesApplication;

enum class ViewModes : uint8_t
{
	Lit = 0,
	Unlit,
	Normals,
	COUNT
};

// Data shared between the application layer and the user interface layer
struct UIData
{
	ViewModes ViewMode = ViewModes::Unlit;

	bool Wireframe = true;
	bool BackFaceCulling = true;

	bool UpdateTerrain = true;
	bool DrawTerrain = true;

	bool DrawObjects = false;

	int TerrainLOD = 0;
	float TerrainHeight = 1.0f;

	donut::math::float3 CameraPosition;
	donut::math::float3 LightDirection;

	UIData()
	{
		LightDirection = donut::math::sphericalToCartesian(0.0f, -0.7f, 1.0f);
	}
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
