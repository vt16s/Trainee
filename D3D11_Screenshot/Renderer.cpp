//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "Renderer.h"
#include <DirectXMath.h>
#include <assert.h>
#include <float.h>
#include <wincodec.h>
#include "ScreenGrab11.h"

#include "PixelShader.h"
#include "VertexShader.h"

#pragma comment(lib, "d3d11.lib")

#define NUMVERTICES 6

using namespace DirectX;

//
// A vertex with a position and texture coordinate
//
typedef struct _VERTEX
{
    XMFLOAT3 Pos;
    XMFLOAT2 TexCoord;
} VERTEX;

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
Renderer::Renderer()
{
}

//-----------------------------------------------------------------------------
//
// Create device and swap chain.
//
//-----------------------------------------------------------------------------
HRESULT Renderer::InitD3D(HWND hWnd)
{
    HRESULT hr = S_OK;

    // Store window handle
    m_windowHandle = hWnd;

    // Driver types supported
    D3D_DRIVER_TYPE DriverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

    // Feature levels supported
    D3D_FEATURE_LEVEL FeatureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_1
    };
    UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);
    D3D_FEATURE_LEVEL FeatureLevel;

    // Create device
    for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex)
    {
        hr = D3D11CreateDevice(nullptr, DriverTypes[DriverTypeIndex], nullptr, 0, FeatureLevels, NumFeatureLevels,
            D3D11_SDK_VERSION, m_device.GetAddressOf(), &FeatureLevel, m_context.GetAddressOf());
        if (SUCCEEDED(hr))
        {
            // Device creation succeeded, no need to loop anymore
            break;
        }
    }
    assert(SUCCEEDED(hr));

    // Get DXGI factory
    Microsoft::WRL::ComPtr <IDXGIDevice> DxgiDevice;
    hr = m_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(DxgiDevice.GetAddressOf()));
    assert(SUCCEEDED(hr));

    Microsoft::WRL::ComPtr <IDXGIAdapter> DxgiAdapter;
    hr = DxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(DxgiAdapter.GetAddressOf()));
    assert(SUCCEEDED(hr));

    hr = DxgiAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(m_factory.GetAddressOf()));
    assert(SUCCEEDED(hr));

    Microsoft::WRL::ComPtr <IDXGIOutput> DxgiOutput;

    // Figure out right dimensions for full size desktop texture and # of outputs to duplicate

    hr = DxgiAdapter->EnumOutputs(0, &DxgiOutput);
    assert(SUCCEEDED(hr));

    DXGI_OUTPUT_DESC DesktopDesc;
    DxgiOutput->GetDesc(&DesktopDesc);

    // QI for Output 1
    Microsoft::WRL::ComPtr <IDXGIOutput1> DxgiOutput1;
    hr = DxgiOutput->QueryInterface(__uuidof(DxgiOutput1), reinterpret_cast<void**>(DxgiOutput1.GetAddressOf()));
    assert(SUCCEEDED(hr));

    // Create desktop duplication
    hr = DxgiOutput1->DuplicateOutput(m_device.Get(), m_deskDupl.GetAddressOf());
    assert(SUCCEEDED(hr));

    // Get window size
    RECT WindowRect;
    GetClientRect(m_windowHandle, &WindowRect);
    UINT Width = WindowRect.right - WindowRect.left;
    UINT Height = WindowRect.bottom - WindowRect.top;

    // Create swapchain for window
    DXGI_SWAP_CHAIN_DESC1 SwapChainDesc;
    RtlZeroMemory(&SwapChainDesc, sizeof(SwapChainDesc));

    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    SwapChainDesc.BufferCount = 2;
    SwapChainDesc.Width = Width;
    SwapChainDesc.Height = Height;
    SwapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.SampleDesc.Count = 1;
    SwapChainDesc.SampleDesc.Quality = 0;
    hr = m_factory->CreateSwapChainForHwnd(m_device.Get(), hWnd, &SwapChainDesc, nullptr, nullptr, m_swapChain.GetAddressOf());

    assert(SUCCEEDED(hr));

    // Disable the ALT-ENTER shortcut for entering full-screen mode
    hr = m_factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
    assert(SUCCEEDED(hr));

    // Create shared texture
    CreateSharedSurf();

    // Create render target view
    MakeRTV();
    assert(SUCCEEDED(hr));

    // Set viewport
    D3D11_VIEWPORT VP;
    VP.Width = static_cast<FLOAT>(Width);
    VP.Height = static_cast<FLOAT>(Height);
    VP.MinDepth = 0.0f;
    VP.MaxDepth = 1.0f;
    VP.TopLeftX = 0;
    VP.TopLeftY = 0;
    m_context->RSSetViewports(1, &VP);


    // Create the sample state
    D3D11_SAMPLER_DESC SampDesc;
    RtlZeroMemory(&SampDesc, sizeof(SampDesc));
    SampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    SampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    SampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    SampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    SampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    SampDesc.MinLOD = 0;
    SampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = m_device->CreateSamplerState(&SampDesc, m_samplerState.GetAddressOf());
    assert(SUCCEEDED(hr));
    m_context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

    // Create shaders and input layout
    CreateShaders();

    return hr;
}

//-----------------------------------------------------------------------------
// Creating shared surface
//-----------------------------------------------------------------------------
void Renderer::CreateSharedSurf()
{
    HRESULT hr;

    DXGI_OUTDUPL_DESC lOutputDuplDesc;
    m_deskDupl->GetDesc(&lOutputDuplDesc);

    D3D11_TEXTURE2D_DESC desc;

    desc.Width = lOutputDuplDesc.ModeDesc.Width;
    desc.Height = lOutputDuplDesc.ModeDesc.Height;
    desc.Format = lOutputDuplDesc.ModeDesc.Format;
    desc.ArraySize = 1;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    desc.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.MipLevels = 1;
    desc.CPUAccessFlags = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    hr = m_device->CreateTexture2D(&desc, NULL, m_sharedSurf.GetAddressOf());
    assert(SUCCEEDED(hr));
}

//-----------------------------------------------------------------------------
// Creating render target view
//-----------------------------------------------------------------------------
void Renderer::MakeRTV()
{
    HRESULT hr;

    // Get backbuffer
    Microsoft::WRL::ComPtr <ID3D11Texture2D> backBuffer;
    hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));
    assert(SUCCEEDED(hr));

    // Create a render target view
    hr = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTarget.GetAddressOf());
    assert(SUCCEEDED(hr));

    // Set new render target
    m_context->OMSetRenderTargets(1, m_renderTarget.GetAddressOf(), nullptr);
}

//-----------------------------------------------------------------------------
// Creating shaders and input layout.
//-----------------------------------------------------------------------------
void Renderer::CreateShaders()
{
    HRESULT hr;

    UINT Size = ARRAYSIZE(g_VS);
    hr = m_device->CreateVertexShader(g_VS, Size, nullptr, m_vertexShader.GetAddressOf());
    assert(SUCCEEDED(hr));
    m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);

    D3D11_INPUT_ELEMENT_DESC Layout[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    UINT NumElements = ARRAYSIZE(Layout);
    hr = m_device->CreateInputLayout(Layout, NumElements, g_VS, Size, m_inputLayout.GetAddressOf());
    assert(SUCCEEDED(hr));
    m_context->IASetInputLayout(m_inputLayout.Get());

    Size = ARRAYSIZE(g_PS);
    hr = m_device->CreatePixelShader(g_PS, Size, nullptr, m_pixelShader.GetAddressOf());
    assert(SUCCEEDED(hr));
    m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
}

//-----------------------------------------------------------------------------
// Getting screen capture
//-----------------------------------------------------------------------------
bool Renderer::GetFrame()
{
    HRESULT hr;

    for (int i = 0; i < 10; ++i) 
    {
        DXGI_OUTDUPL_FRAME_INFO FrameInfo{};
        hr = m_deskDupl->AcquireNextFrame(500, &FrameInfo, m_deskResource.GetAddressOf());
        if (SUCCEEDED(hr) && (FrameInfo.LastPresentTime.QuadPart == 0)) 
        {
            // If AcquireNextFrame() returns S_OK and
            // fi.LastPresentTime.QuadPart == 0, it means
            // AcquireNextFrame() didn't acquire next frame yet.
            // We must wait next frame sync timing to retrieve
            // actual frame data.
            //
            // Since method is successfully completed,
            // we need to release the resource and frame explicitly.
            m_deskResource->Release();
            m_deskDupl->ReleaseFrame();
            Sleep(1);
            continue;
        }
        else 
            break;
    }
    if (FAILED(hr))
        return false;

    // QI for ID3D11Texture2D
    hr = m_deskResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(m_acquiredDesktopImage.GetAddressOf()));
    if (FAILED(hr))
        return false;
  
    m_context->CopyResource(m_sharedSurf.Get(), m_acquiredDesktopImage.Get());

    hr = SaveWICTextureToFile(m_context.Get(), m_sharedSurf.Get(), GUID_ContainerFormatPng, L"SCREENSHOT.PNG");
    assert(SUCCEEDED(hr));

    return true;
}

//-----------------------------------------------------------------------------
// Drawing screen capture
//-----------------------------------------------------------------------------
void Renderer::DrawFrame()
{
    HRESULT hr;

   // GetFrame();

    // Vertices for drawing whole texture
    VERTEX Vertices[NUMVERTICES] =
    {
        {XMFLOAT3(-1.0f, -1.0f, 0), XMFLOAT2(0.0f, 1.0f)},
        {XMFLOAT3(-1.0f, 1.0f, 0), XMFLOAT2(0.0f, 0.0f)},
        {XMFLOAT3(1.0f, -1.0f, 0), XMFLOAT2(1.0f, 1.0f)},
        {XMFLOAT3(1.0f, -1.0f, 0), XMFLOAT2(1.0f, 1.0f)},
        {XMFLOAT3(-1.0f, 1.0f, 0), XMFLOAT2(0.0f, 0.0f)},
        {XMFLOAT3(1.0f, 1.0f, 0), XMFLOAT2(1.0f, 0.0f)},
    };

    D3D11_TEXTURE2D_DESC FrameDesc;
    m_sharedSurf->GetDesc(&FrameDesc);

    D3D11_SHADER_RESOURCE_VIEW_DESC ShaderDesc;
    ShaderDesc.Format = FrameDesc.Format;
    ShaderDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    ShaderDesc.Texture2D.MostDetailedMip = FrameDesc.MipLevels - 1;
    ShaderDesc.Texture2D.MipLevels = FrameDesc.MipLevels;

    // Create new shader resource view
    hr = m_device->CreateShaderResourceView(m_sharedSurf.Get(), &ShaderDesc, m_shaderResourceView.GetAddressOf());
    assert(SUCCEEDED(hr));

    // Set resources
    UINT Stride = sizeof(VERTEX);
    UINT Offset = 0;
    FLOAT blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };

    m_context->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
    m_context->PSSetShaderResources(0, 1, m_shaderResourceView.GetAddressOf());
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D11_BUFFER_DESC BufferDesc;
    RtlZeroMemory(&BufferDesc, sizeof(BufferDesc));
    BufferDesc.Usage = D3D11_USAGE_DEFAULT;
    BufferDesc.ByteWidth = sizeof(VERTEX) * NUMVERTICES;
    BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    BufferDesc.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
    RtlZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = Vertices;

    Microsoft::WRL::ComPtr <ID3D11Buffer> VertexBuffer;

    // Create vertex buffer
    hr = m_device->CreateBuffer(&BufferDesc, &InitData, VertexBuffer.GetAddressOf());
    assert(SUCCEEDED(hr));

    m_context->IASetVertexBuffers(0, 1, VertexBuffer.GetAddressOf(), &Stride, &Offset);

    // Draw textured quad onto render target
    m_context->Draw(NUMVERTICES, 0);
    // Present the frame to the screen.
    m_swapChain->Present(1, 0);
}

//-----------------------------------------------------------------------------
// Clean up cube resources when the Direct3D device is lost or destroyed.
//-----------------------------------------------------------------------------
Renderer::~Renderer()
{
    // ComPtr will clean up references for us. But be careful to release
    // references to anything you don't need whenever you call Flush or Trim.
    // As always, clean up your system (CPU) memory resources before exit.
}