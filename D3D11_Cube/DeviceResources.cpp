//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <string>
#include <memory>
#include <dxgi1_3.h>

#include "DeviceResources.h"


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
DeviceResources::DeviceResources()
{

};

//-----------------------------------------------------------------------------
//
// Creates the device.
//
//-----------------------------------------------------------------------------
HRESULT DeviceResources::CreateDeviceResources()
{
    HRESULT hr = S_OK;

    D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_9_1,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_11_1
    };

    // This flag adds support for surfaces with a color-channel ordering different
    // from the API default. It is required for compatibility with Direct2D.
    UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(DEBUG) || defined(_DEBUG)
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // Create the Direct3D 11 API device object and a corresponding context.
    Microsoft::WRL::ComPtr<ID3D11Device>        device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;

#pragma comment(lib, "d3d11.lib")

    hr = D3D11CreateDevice(
        nullptr,                    // Specify nullptr to use the default adapter.
        D3D_DRIVER_TYPE_HARDWARE,   // Create a device using the hardware graphics driver.
        0,                          // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
        deviceFlags,                // Set debug and Direct2D compatibility flags.
        levels,                     // List of feature levels this app can support.
        ARRAYSIZE(levels),          // Size of the list above.
        D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Windows Store apps.
        &device,                    // Returns the Direct3D device created.
        &m_featureLevel,            // Returns feature level of device created.
        &context                    // Returns the device immediate context.
    );

    if (FAILED(hr))
    {
        // Handle device interface creation failure if it occurs.
        // For example, reduce the feature level requirement, or fail over 
        // to WARP rendering.
    }

    // Store pointers to the Direct3D 11.1 API device and immediate context.
    device.As(&m_pd3dDevice);
    context.As(&m_pd3dDeviceContext);

    return hr;
}

//-----------------------------------------------------------------------------
// Creates the swap chain.
//-----------------------------------------------------------------------------
HRESULT DeviceResources::CreateWindowResources(HWND hWnd)
{
    HRESULT hr = S_OK;


    DXGI_SWAP_CHAIN_DESC desc;
    ZeroMemory(&desc, sizeof(DXGI_SWAP_CHAIN_DESC));
    desc.Windowed = TRUE; // Sets the initial state of full-screen mode.
    desc.BufferCount = 2;
    desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SampleDesc.Count = 1;      //multisampling setting
    desc.SampleDesc.Quality = 0;    //vendor-specific flag
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.OutputWindow = hWnd;

    // Create the DXGI device object to use in other factories, such as Direct2D.
    Microsoft::WRL::ComPtr<IDXGIDevice3> dxgiDevice;
    m_pd3dDevice.As(&dxgiDevice);

    // Create swap chain.
    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    Microsoft::WRL::ComPtr<IDXGIFactory> factory;

    hr = dxgiDevice->GetAdapter(&adapter);

    if (SUCCEEDED(hr))
    {
        adapter->GetParent(IID_PPV_ARGS(&factory));

        hr = factory->CreateSwapChain(
            m_pd3dDevice.Get(),
            &desc,
            &m_pDXGISwapChain
        );
    }

    // Configure the back buffer, stencil buffer, and viewport.
    hr = ConfigureBackBuffer();

    return hr;
}

HRESULT DeviceResources::ConfigureBackBuffer()
{
    HRESULT hr = S_OK;

    hr = m_pDXGISwapChain->GetBuffer(
        0,
        __uuidof(ID3D11Texture2D),
        (void**)&m_pBackBuffer);

    hr = m_pd3dDevice->CreateRenderTargetView(
        m_pBackBuffer.Get(),
        nullptr,
        m_pRenderTarget.GetAddressOf()
    );

    m_pBackBuffer->GetDesc(&m_bbDesc);

    // Create a depth-stencil view for use with 3D rendering if needed.
    CD3D11_TEXTURE2D_DESC depthStencilDesc(
        DXGI_FORMAT_D24_UNORM_S8_UINT,
        static_cast<UINT> (m_bbDesc.Width),
        static_cast<UINT> (m_bbDesc.Height),
        1, // This depth stencil view has only one texture.
        1, // Use a single mipmap level.
        D3D11_BIND_DEPTH_STENCIL
    );

    m_pd3dDevice->CreateTexture2D(
        &depthStencilDesc,
        nullptr,
        &m_pDepthStencil
    );

    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);

    m_pd3dDevice->CreateDepthStencilView(
        m_pDepthStencil.Get(),
        &depthStencilViewDesc,
        &m_pDepthStencilView
    );


    ZeroMemory(&m_viewport, sizeof(D3D11_VIEWPORT));
    m_viewport.Height = (float)m_bbDesc.Height;
    m_viewport.Width = (float)m_bbDesc.Width;
    m_viewport.MinDepth = 0;
    m_viewport.MaxDepth = 1;

    m_pd3dDeviceContext->RSSetViewports(
        1,
        &m_viewport
    );

    return hr;
}

HRESULT DeviceResources::ReleaseBackBuffer()
{
    HRESULT hr = S_OK;

    // Release the render target view based on the back buffer:
    m_pRenderTarget.Reset();

    // Release the back buffer itself:
    m_pBackBuffer.Reset();

    // The depth stencil will need to be resized, so release it (and view):
    m_pDepthStencilView.Reset();
    m_pDepthStencil.Reset();

    // After releasing references to these resources, we need to call Flush() to 
    // ensure that Direct3D also releases any references it might still have to
    // the same resources - such as pipeline bindings.
    m_pd3dDeviceContext->Flush();

    return hr;
}

//-----------------------------------------------------------------------------
// Returns the aspect ratio of the back buffer.
//-----------------------------------------------------------------------------
float DeviceResources::GetAspectRatio()
{
    return static_cast<float>(m_bbDesc.Width) / static_cast<float>(m_bbDesc.Height);
}

//-----------------------------------------------------------------------------
// Present frame:
// Show the frame on the primary surface.
//-----------------------------------------------------------------------------
void DeviceResources::Present()
{
    m_pDXGISwapChain->Present(1, 0);
}


//-----------------------------------------------------------------------------
// Destructor.
//-----------------------------------------------------------------------------
DeviceResources::~DeviceResources()
{

}