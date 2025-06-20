#pragma once

#include <donut/app/ApplicationBase.h>
#include <donut/app/Camera.h>
#include <donut/engine/View.h>
#include <donut/engine/ShaderFactory.h>

#include <donut/engine/BindingCache.h>
#include <donut/render/GBuffer.h>

#include <donut/render/DeferredLightingPass.h>
#include <donut/render/GBufferFillPass.h>

#include "landscapes_scene.h"


extern const char* g_WindowTitle;

class LandscapesApplication : public donut::app::ApplicationBase
{
public:
	using ApplicationBase::ApplicationBase;

	bool Init();

	bool LoadScene(std::shared_ptr<donut::vfs::IFileSystem> fs, const std::filesystem::path& sceneFileName) override;

	bool KeyboardUpdate(int key, int scancode, int action, int mods) override;
	bool MousePosUpdate(double xpos, double ypos) override;
	bool MouseButtonUpdate(int button, int action, int mods) override;

	void Animate(float fElapsedTimeSeconds) override;
	void BackBufferResizing() override;

	void Render(nvrhi::IFramebuffer* framebuffer) override;

private:

	nvrhi::TextureHandle CreateDeferredShadingOutput(nvrhi::IDevice* device, dm::uint2 size, dm::uint sampleCount);

private:
	std::shared_ptr<donut::engine::ShaderFactory> m_ShaderFactory;
	std::unique_ptr<donut::engine::BindingCache> m_BindingCache;

	nvrhi::CommandListHandle m_CommandList;

	donut::app::FirstPersonCamera m_Camera;
	donut::engine::PlanarView m_View;

	std::shared_ptr<donut::render::GBufferRenderTargets> m_GBuffer;
	nvrhi::TextureHandle m_ShadedColour;

	std::unique_ptr<donut::render::GBufferFillPass> m_GBufferPass;
	std::unique_ptr<donut::render::DeferredLightingPass> m_DeferredLightingPass;

	LandscapesScene m_Scene;
};
