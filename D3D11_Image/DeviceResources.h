#pragma once

#include <windows.h>
#include <d3d11.h>
#include <wrl.h>

//-----------------------------------------------------------------------------
// Class declarations
//-----------------------------------------------------------------------------

class DeviceResources
{
public:
    DeviceResources();
    ~DeviceResources();

    HRESULT CreateDeviceResources(HWND hWnd);

    ID3D11Device* GetDevice() { return m_pd3dDevice.Get(); };
    ID3D11DeviceContext* GetDeviceContext() { return m_pd3dDeviceContext.Get(); };
    ID3D11RenderTargetView* GetRenderTarget() { return m_pRenderTarget.Get(); }

    void Present();

private:

    //-----------------------------------------------------------------------------
    // Direct3D device
    //-----------------------------------------------------------------------------
    Microsoft::WRL::ComPtr<ID3D11Device>        m_pd3dDevice;        // the pointer to the Direct3D device interface
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_pd3dDeviceContext; // the pointer to the Direct3D device context
    Microsoft::WRL::ComPtr<IDXGISwapChain>      m_pDXGISwapChain;    // the pointer to the swap chain interface

    //-----------------------------------------------------------------------------
    // DXGI swap chain device resources
    //-----------------------------------------------------------------------------
    Microsoft::WRL::ComPtr <ID3D11Texture2D>        m_pBackBuffer;
    Microsoft::WRL::ComPtr <ID3D11RenderTargetView> m_pRenderTarget;

    D3D11_VIEWPORT  m_viewport;
};