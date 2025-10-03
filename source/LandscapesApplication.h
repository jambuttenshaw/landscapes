#pragma once

#include <donut/app/ApplicationBase.h>
#include <donut/app/Camera.h>
#include <donut/engine/View.h>
#include <donut/engine/ShaderFactory.h>

#include <donut/engine/BindingCache.h>
#include <donut/render/GBuffer.h>

#include <donut/render/DeferredLightingPass.h>
#include <donut/render/GBufferFillPass.h>

#include "UserInterface.h"
#include "LandscapesScene.h"

#include "engine/ViewEx.h"
#include "render/passes/DebugPasses.h"
#include "render/Passes/GBufferVisualizationPass.h"
#include "render/Passes/TerrainPass.h"
#include "terrain/TerrainTessellation.h"


extern const char* g_WindowTitle;

class LandscapesApplication : public donut::app::ApplicationBase
{
public:
	using ApplicationBase::ApplicationBase;

	LandscapesApplication(donut::app::DeviceManager* deviceManager, UIData& ui);

	virtual void SceneUnloading() override;
	virtual bool LoadScene(std::shared_ptr<donut::vfs::IFileSystem> fs, const std::filesystem::path& sceneFileName) override;
	virtual void SceneLoaded() override;

	bool KeyboardUpdate(int key, int scancode, int action, int mods) override;
	bool MousePosUpdate(double xpos, double ypos) override;
	bool MouseButtonUpdate(int button, int action, int mods) override;

	void Animate(float fElapsedTimeSeconds) override;
	void BackBufferResizing() override;

	void RenderScene(nvrhi::IFramebuffer* framebuffer) override;


	inline std::shared_ptr<donut::engine::ShaderFactory> GetShaderFactory() const { return m_ShaderFactory; }

private:

	void CreateDeferredShadingOutput(nvrhi::IDevice* device, dm::uint2 size, dm::uint sampleCount);
	void CreateGBufferPasses();

private:
	UIData& m_UI;

	std::shared_ptr<donut::vfs::NativeFileSystem> m_NativeFS;

	std::shared_ptr<donut::engine::ShaderFactory> m_ShaderFactory;
	std::unique_ptr<donut::engine::BindingCache> m_BindingCache;

	nvrhi::CommandListHandle m_CommandList;

	donut::app::FirstPersonCamera m_Camera;
	PlanarViewEx m_View;

	std::shared_ptr<donut::render::GBufferRenderTargets> m_GBuffer;
	nvrhi::TextureHandle m_ShadedColour;

	std::unique_ptr<TerrainTessellator> m_TerrainTessellator;

	std::unique_ptr<donut::render::GBufferFillPass> m_GBufferPass;
	std::unique_ptr<TerrainGBufferFillPass> m_TerrainGBufferPass;

	std::unique_ptr<donut::render::DeferredLightingPass> m_DeferredLightingPass;
	std::unique_ptr<GBufferVisualizationPass> m_GBufferVisualizationPass;

	std::unique_ptr<DebugPlanePass> m_DebugPlanePass;

	std::unique_ptr<LandscapesScene> m_Scene;

	std::shared_ptr<donut::engine::DirectionalLight> m_SunLight;
};
