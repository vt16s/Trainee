#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "DeviceResources.h"
#include "Renderer.h"

//-----------------------------------------------------------------------------
// Class declarations
//-----------------------------------------------------------------------------

class MainClass
{
public:
    MainClass();
    ~MainClass();

    HRESULT CreateDesktopWindow();

    HWND GetWindowHandle() { return m_hWnd; };

    static LRESULT CALLBACK StaticWindowProc(
        HWND hWnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
    );

    HRESULT Run(
        std::shared_ptr<DeviceResources> deviceResources,
        std::shared_ptr<Renderer> renderer
    );

    static constexpr int m_window_width  = 640;
    static constexpr int m_window_height = 480;

private:
    HWND      m_hWnd;
};

// These are STATIC because this sample only creates one window.
// If your app can have multiple windows, MAKE SURE this is dealt with 
// differently.
static HINSTANCE m_hInstance;
static std::wstring m_windowClassName;