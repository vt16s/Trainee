#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl.h>

//-----------------------------------------------------------------------------
// Class declarations
//-----------------------------------------------------------------------------

class Renderer
{
public:

    Renderer();
    ~Renderer();
    HRESULT InitD3D(HWND hWnd);
    void DrawFrame();
    void Present();

private:
    void CreateSharedSurf();
    void MakeRTV();
    void CreateShaders();
    void GetFrame();

    HWND m_windowHandle;
    Microsoft::WRL::ComPtr <ID3D11Device>             m_device;
    Microsoft::WRL::ComPtr <IDXGIFactory2>            m_factory;
    Microsoft::WRL::ComPtr <ID3D11DeviceContext>      m_context;
    Microsoft::WRL::ComPtr <IDXGISwapChain1>          m_swapChain;
    Microsoft::WRL::ComPtr <ID3D11RenderTargetView>   m_renderTarget;
    Microsoft::WRL::ComPtr <ID3D11Buffer>             m_vertexDataBuffer;
    Microsoft::WRL::ComPtr <ID3D11InputLayout>        m_inputLayout;
    Microsoft::WRL::ComPtr <ID3D11VertexShader>       m_vertexShader;
    Microsoft::WRL::ComPtr <ID3D11PixelShader>        m_pixelShader;
    Microsoft::WRL::ComPtr <ID3D11ShaderResourceView> m_shaderResourceView;
    Microsoft::WRL::ComPtr <ID3D11SamplerState>       m_samplerState;
    Microsoft::WRL::ComPtr <ID3D11Texture2D>          m_sharedSurf;
    Microsoft::WRL::ComPtr <ID3D11Texture2D>          m_acquiredDesktopImage;
    Microsoft::WRL::ComPtr <IDXGIOutputDuplication>   m_deskDupl;
};