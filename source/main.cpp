#include <donut/app/ApplicationBase.h>
#include <donut/engine/ShaderFactory.h>
#include <donut/app/DeviceManager.h>
#include <donut/core/log.h>
#include <nvrhi/utils.h>

#include "basic_triangle.h"

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
        BasicTriangle example(deviceManager);
        if (example.Init())
        {
            deviceManager->AddRenderPassToBack(&example);
            deviceManager->RunMessageLoop();
            deviceManager->RemoveRenderPass(&example);
        }
    }
    
    deviceManager->Shutdown();

    delete deviceManager;

    return 0;
}
