//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <string>
#include <memory>

#include "DeviceResources.h"
#include "MainClass.h"

#pragma comment(lib, "d3d11.lib")

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
DeviceResources::DeviceResources()
{

};

//-----------------------------------------------------------------------------
//
// Create device and swap chain.
//
//-----------------------------------------------------------------------------
HRESULT DeviceResources::CreateDeviceResources(HWND hWnd)
{
    HRESULT hr = S_OK;

    // Create a struct to hold information about the swap chain
    DXGI_SWAP_CHAIN_DESC desc;
    ZeroMemory(&desc, sizeof(DXGI_SWAP_CHAIN_DESC));
    desc.BufferCount        = 1;                               // one back buffer
    desc.BufferDesc.Format  = DXGI_FORMAT_R8G8B8A8_UNORM;      // use 32-bit color
    desc.BufferDesc.Width   = MainClass::m_window_width;       // set the back buffer width
    desc.BufferDesc.Height  = MainClass::m_window_height;      // set the back buffer height
    desc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT; // how swap chain is to be used
    desc.OutputWindow       = hWnd;                            // the window to be used
    desc.SampleDesc.Count   = 1;                               // how many multisamples
    desc.SampleDesc.Quality = 0;                               // multisample quality level
    desc.Windowed           = TRUE;                            // windowed/full-screen mode

    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;

    hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &desc,
        swapChain.GetAddressOf(),
        device.GetAddressOf(),
        nullptr,
        context.GetAddressOf()
    );

    device.As(&m_pd3dDevice);
    context.As(&m_pd3dDeviceContext);
    swapChain.As(&m_pDXGISwapChain);

    // Configure the back buffer.
    hr = m_pDXGISwapChain->GetBuffer(
        0,
        __uuidof(ID3D11Texture2D),
        (void**)&m_pBackBuffer);

    hr = m_pd3dDevice->CreateRenderTargetView(
        m_pBackBuffer.Get(),
        nullptr,
        m_pRenderTarget.GetAddressOf()
    );

    // Set the render target as the back buffer
    m_pd3dDeviceContext->OMSetRenderTargets(1, m_pRenderTarget.GetAddressOf(), nullptr);

    // Set the viewport
    ZeroMemory(&m_viewport, sizeof(D3D11_VIEWPORT));
    m_viewport.TopLeftX = 0;
    m_viewport.TopLeftY = 0;
    m_viewport.Width    = MainClass::m_window_width;
    m_viewport.Height   = MainClass::m_window_height;

    m_pd3dDeviceContext->RSSetViewports(1, &m_viewport);

    return hr;
}

//-----------------------------------------------------------------------------
// Present frame:
// Show the frame on the primary surface.
//-----------------------------------------------------------------------------
void DeviceResources::Present()
{
    m_pDXGISwapChain->Present(0, 0);
}

//-----------------------------------------------------------------------------
// Destructor.
//-----------------------------------------------------------------------------
DeviceResources::~DeviceResources()
{

}