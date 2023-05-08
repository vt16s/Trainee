#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <memory>
#include "DeviceResources.h"

//-----------------------------------------------------------------------------
// Class declarations
//-----------------------------------------------------------------------------

class Renderer
{
public:

    Renderer(std::shared_ptr<DeviceResources> deviceResources);
    ~Renderer();
    void CreateDeviceDependentResources();
    void Render();

private:

    std::shared_ptr<DeviceResources> m_deviceResources;
    Microsoft::WRL::ComPtr <ID3D11Buffer> m_vertexDataBuffer;
    Microsoft::WRL::ComPtr <ID3D11InputLayout> m_inputLayout;
    Microsoft::WRL::ComPtr <ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr <ID3D11PixelShader> m_pixelShader;
    Microsoft::WRL::ComPtr <ID3D11ShaderResourceView> m_textureView;
    UINT m_stride;       // stride for the vertex-buffer
    UINT m_offset;       // offset for the vertex-buffer
    UINT m_numVertices;  // number of vertices to draw
};