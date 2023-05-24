//-----------------------------------------------------------------------------
// File: Screenshot.cpp
//
// Desktop app that renders screen.
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MainWindow.h"
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
    std::shared_ptr<MainWindow> winMain = std::shared_ptr<MainWindow>(new MainWindow());

    // Create a window.
    hr = winMain->CreateDesktopWindow();

    if (SUCCEEDED(hr))
    {
        CoInitialize(nullptr);
        // Instantiate the renderer.
        std::shared_ptr<Renderer> renderer = std::shared_ptr<Renderer>(new Renderer());
        hr = renderer->InitD3D(winMain->GetWindowHandle());

        if (FAILED(hr))
            return hr;

      //  if (renderer->GetFrame())
      //  {
            // Saving to png
      //      renderer->SaveToPng();
            // Run the program.
            hr = winMain->Run(renderer);
      //  }
    }

    // Cleanup is handled in destructors.
    return hr;
}