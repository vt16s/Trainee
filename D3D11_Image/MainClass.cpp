//-----------------------------------------------------------------------------
// File: MainClass.cpp
//
// Creates and owns a desktop window resource.
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <string>
#include <memory>

#include "MainClass.h"

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
MainClass::MainClass()
{
    m_windowClassName = L"Direct3DWindowClass";
    m_hInstance = NULL;
}

//-----------------------------------------------------------------------------
// Create a window for our Direct3D viewport.
//-----------------------------------------------------------------------------
HRESULT MainClass::CreateDesktopWindow()
{
    // Window resources are dealt with here.

    if (m_hInstance == NULL)
        m_hInstance = (HINSTANCE)GetModuleHandle(NULL);

    HICON hIcon = NULL;
    WCHAR szExePath[MAX_PATH];
    GetModuleFileName(NULL, szExePath, MAX_PATH);

    // If the icon is NULL, then use the first one found in the exe
    if (hIcon == NULL)
        hIcon = ExtractIcon(m_hInstance, szExePath, 0);

    // Register the windows class
    WNDCLASS wndClass = {};
    wndClass.style         = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc   = MainClass::StaticWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = 0;
    wndClass.hInstance     = m_hInstance;
    wndClass.hIcon         = hIcon;
    wndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndClass.lpszMenuName  = NULL;
    wndClass.lpszClassName = m_windowClassName.c_str();

    if (!RegisterClass(&wndClass))
    {
        DWORD dwError = GetLastError();
        if (dwError != ERROR_CLASS_ALREADY_EXISTS)
            return HRESULT_FROM_WIN32(dwError);
    }

    // Create the window for our viewport.
    m_hWnd = CreateWindow(
        m_windowClassName.c_str(),
        L"Image",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        m_window_width,
        m_window_height,
        NULL,
        NULL,
        m_hInstance,
        NULL
    );

    if (m_hWnd == NULL)
    {
        DWORD dwError = GetLastError();
        return HRESULT_FROM_WIN32(dwError);
    }

    return S_OK;
}

HRESULT MainClass::Run(
    std::shared_ptr<DeviceResources> deviceResources,
    std::shared_ptr<Renderer> renderer
)
{
    HRESULT hr = S_OK;

    if (!IsWindowVisible(m_hWnd))
        ShowWindow(m_hWnd, SW_SHOW);

    // The render loop is controlled here.
    MSG  msg = {};
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // Render frames during idle time (when no messages are waiting).
            renderer->Render();

            // Present the frame to the screen.
            deviceResources->Present();
        }
    }

    return hr;
}

//-----------------------------------------------------------------------------
// Destructor.
//-----------------------------------------------------------------------------
MainClass::~MainClass()
{

}

//-----------------------------------------------------------------------------
// Process windows messages. This looks for window close events, letting us
// exit out of the sample.
//-----------------------------------------------------------------------------
LRESULT CALLBACK MainClass::StaticWindowProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}