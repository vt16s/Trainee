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

#include "MainWindow.h"

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
MainWindow::MainWindow()
{
    m_windowClassName = L"Direct3DWindowClass";
    m_hInstance       = nullptr;
}

//-----------------------------------------------------------------------------
// Create a window for our Direct3D viewport.
//-----------------------------------------------------------------------------
HRESULT MainWindow::CreateDesktopWindow()
{
    // Window resources are dealt with here.

    if (m_hInstance == nullptr)
        m_hInstance = (HINSTANCE)GetModuleHandle(nullptr);

    // Register the windows class
  /*  WNDCLASS wndClass{};
    wndClass.style         = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc   = MainWindow::StaticWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = 0;
    wndClass.hInstance     = m_hInstance;
    wndClass.hIcon         = nullptr;
    wndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndClass.lpszMenuName  = NULL;
    wndClass.lpszClassName = m_windowClassName.c_str();*/

    WNDCLASSEXW wndClass{};
    wndClass.cbSize        = sizeof(WNDCLASSEXW);
    wndClass.style         = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc   = MainWindow::StaticWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = 0;
    wndClass.hInstance     = m_hInstance;
    wndClass.hIcon         = nullptr;
    wndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = nullptr;
    wndClass.lpszMenuName  = nullptr;
    wndClass.lpszClassName = m_windowClassName.c_str();
    wndClass.hIconSm       = nullptr;

    if (!RegisterClassExW(&wndClass))
    {
        DWORD dwError = GetLastError();
        if (dwError != ERROR_CLASS_ALREADY_EXISTS)
            return HRESULT_FROM_WIN32(dwError);
    }

    RECT wr = { 0, 0, m_window_width, m_window_height };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    // Create the window for our viewport.
    m_hWnd = CreateWindowW(
        m_windowClassName.c_str(),
        L"Screenshot",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        wr.right - wr.left,
        wr.bottom - wr.top,
        nullptr,
        nullptr,
        m_hInstance,
        nullptr
    );

    if (m_hWnd == nullptr)
    {
        DWORD dwError = GetLastError();
        return HRESULT_FROM_WIN32(dwError);
    }

    return S_OK;
}

HRESULT MainWindow::Run( std::shared_ptr<Renderer> renderer )
{
    HRESULT hr = S_OK;

    if (!IsWindowVisible(m_hWnd))
        ShowWindow(m_hWnd, SW_SHOW);

    // The render loop is controlled here.
    HRESULT hr_coInit = CoInitialize(nullptr);
    MSG  msg = {};
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
              if (renderer->GetFrame())
              {
                  // Saving to png
                  if(SUCCEEDED(hr_coInit))
                      renderer->SaveToPng();
                  // Draw and present the frame to the screen.
                  renderer->DrawFrame();
              }
        }
    }

    if (SUCCEEDED(hr_coInit))
        CoUninitialize();

    return hr;
}

//-----------------------------------------------------------------------------
// Destructor.
//-----------------------------------------------------------------------------
MainWindow::~MainWindow()
{

}

//-----------------------------------------------------------------------------
// Process windows messages. This looks for window close events, letting us
// exit out of the sample.
//-----------------------------------------------------------------------------
LRESULT CALLBACK MainWindow::StaticWindowProc(
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