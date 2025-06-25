#include <donut/app/ApplicationBase.h>
#include <donut/engine/ShaderFactory.h>
#include <donut/app/DeviceManager.h>
#include <donut/core/log.h>

#include "landscapes_application.h"
#include "user_interface.h"

using namespace donut;


#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int __argc, const char** __argv)
#endif
{
    nvrhi::GraphicsAPI api = app::GetGraphicsAPIFromCommandLine(__argc, __argv);
    app::DeviceManager* deviceManager = app::DeviceManager::Create(api);

    app::DeviceCreationParameters deviceParams;
#ifdef _DEBUG
    deviceParams.enableDebugRuntime = true; 
    deviceParams.enableNvrhiValidationLayer = true;
#endif

    if (!deviceManager->CreateWindowDeviceAndSwapChain(deviceParams, g_WindowTitle))
    {
        log::fatal("Cannot initialize a graphics device with the requested parameters");
        return 1;
    }
    
    {
        UIData m_UI;

        auto application = std::make_shared<LandscapesApplication>(deviceManager, m_UI);
        auto uiRenderer = std::make_shared<UIRenderer>(deviceManager, application, m_UI);

        if (application->Init() && uiRenderer->Init(application->GetShaderFactory()))
        {
            deviceManager->AddRenderPassToBack(application.get());
            deviceManager->AddRenderPassToBack(uiRenderer.get());
            deviceManager->RunMessageLoop();
        }
    }
    
    deviceManager->Shutdown();

    delete deviceManager;

    return 0;
}
