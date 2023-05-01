//-----------------------------------------------------------------------------
// File: Cube.cpp
//
// Desktop app that renders a spinning, colorful cube.
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <string>
#include <memory>

#include "DeviceResources.h"
#include "Renderer.h"
#include "MainClass.h"

//-----------------------------------------------------------------------------
// Main function: Creates window, calls initialization functions, and hosts
// the render loop.
//-----------------------------------------------------------------------------
INT WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    HRESULT hr = S_OK;

    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    // Begin initialization.

    // Instantiate the window manager class.
    std::shared_ptr<MainClass> winMain = std::shared_ptr<MainClass>(new MainClass());
    // Create a window.
    hr = winMain->CreateDesktopWindow();

    if (SUCCEEDED(hr))
    {
        // Instantiate the device manager class.
        std::shared_ptr<DeviceResources> deviceResources = std::shared_ptr<DeviceResources>(new DeviceResources());
        // Create device resources.
        deviceResources->CreateDeviceResources();

        // Instantiate the renderer.
        std::shared_ptr<Renderer> renderer = std::shared_ptr<Renderer>(new Renderer(deviceResources));
        renderer->CreateDeviceDependentResources();

        // We have a window, so initialize window size-dependent resources.
        deviceResources->CreateWindowResources(winMain->GetWindowHandle());
        renderer->CreateWindowSizeDependentResources();

        // Run the program.
        hr = winMain->Run(deviceResources, renderer);
    }

    // Cleanup is handled in destructors.
    return hr;
}