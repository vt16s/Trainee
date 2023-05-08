//-----------------------------------------------------------------------------
// File: Image.cpp
//
// Desktop app that renders an image.
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <string>
#include <memory>

#include "MainClass.h"
#include "DeviceResources.h"
#include "Renderer.h"

//-----------------------------------------------------------------------------
// Main function: Creates window, calls initialization functions, and hosts
// the render loop.
//-----------------------------------------------------------------------------
INT WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    HRESULT hr = S_OK;

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
        deviceResources->CreateDeviceResources(winMain->GetWindowHandle());

        // Instantiate the renderer.
        std::shared_ptr<Renderer> renderer = std::shared_ptr<Renderer>(new Renderer(deviceResources));
        renderer->CreateDeviceDependentResources();

        // Run the program.
        hr = winMain->Run(deviceResources, renderer);
    }

    // Cleanup is handled in destructors.
    return hr;
}