//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <assert.h>
#include <d3dcompiler.h>

#include "Renderer.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#pragma comment (lib, "d3dcompiler.lib")

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
Renderer::Renderer(std::shared_ptr<DeviceResources> deviceResources)
    :
    m_deviceResources(deviceResources)
{
}

//-----------------------------------------------------------------------------
// Create device-dependent resources for rendering.
//-----------------------------------------------------------------------------
void Renderer::CreateDeviceDependentResources()
{
    HRESULT hr;

    auto device  = m_deviceResources->GetDevice();

    // Create shaders

    ID3D10Blob* VSBlob;
    hr = D3DCompileFromFile(L"shaders.hlsl", 0, 0, "vs_main",
        "vs_4_0", 0, 0, &VSBlob, 0);
    assert(SUCCEEDED(hr));

    hr = device->CreateVertexShader(VSBlob->GetBufferPointer(),
        VSBlob->GetBufferSize(),
        0,
        m_vertexShader.GetAddressOf());
    assert(SUCCEEDED(hr));

    ID3D10Blob* PSBlob;
    hr = D3DCompileFromFile(L"shaders.hlsl", 0, 0, "ps_main",
        "ps_4_0", 0, 0, &PSBlob, 0);
    assert(SUCCEEDED(hr));

    hr = device->CreatePixelShader(PSBlob->GetBufferPointer(),
        PSBlob->GetBufferSize(),
        0,
        m_pixelShader.GetAddressOf());
    assert(SUCCEEDED(hr));
    PSBlob->Release();

    // Input layout

    D3D11_INPUT_ELEMENT_DESC InputElementDesc[] = {
        {
            "POSITION", 0,
            DXGI_FORMAT_R32G32_FLOAT,
            0, 0,
            D3D11_INPUT_PER_VERTEX_DATA, 0
        },
        {
            "UV", 0,
            DXGI_FORMAT_R32G32_FLOAT,
            0, D3D11_APPEND_ALIGNED_ELEMENT,
            D3D11_INPUT_PER_VERTEX_DATA, 0
        }
    };

    hr = device->CreateInputLayout(InputElementDesc,
        ARRAYSIZE(InputElementDesc),
        VSBlob->GetBufferPointer(),
        VSBlob->GetBufferSize(),
        m_inputLayout.GetAddressOf()
    );
    assert(SUCCEEDED(hr));
    VSBlob->Release();
    
    // Vertex data, x,y position & uv coords

    float vertexData[] = {
        -0.5f, -0.5f, 0.0f, 1.0f,
        -0.5f, 0.5f, 0.0f, 0.0f,
        0.5f, 0.5f, 1.0f, 0.0f,
        -0.5f, -0.5f, 0.0f, 1.0f,
        0.5f, 0.5f, 1.0f, 0.0f,
        0.5f, -0.5f, 1.0f, 1.0f
    };

    // Vertex buffer

    D3D11_BUFFER_DESC vertexDataDesc = {
        sizeof(vertexData),
        D3D11_USAGE_DEFAULT,
        D3D11_BIND_VERTEX_BUFFER,
        0, 0, 0
    };
    D3D11_SUBRESOURCE_DATA vertexDataInitial = { vertexData };

    hr = device->CreateBuffer(&vertexDataDesc,
        &vertexDataInitial,
        m_vertexDataBuffer.GetAddressOf()
    );
    assert(SUCCEEDED(hr));

    m_stride = 4 * sizeof(float);
    m_offset = 0;
    m_numVertices = sizeof(vertexData) / m_stride;

    // Load image

    int ImageWidth;
    int ImageHeight;
    int ImageChannels;
    int ImageDesiredChannels = 4;

    unsigned char* ImageData = stbi_load("dx11.png",
        &ImageWidth,
        &ImageHeight,
        &ImageChannels, ImageDesiredChannels);
    assert(ImageData);

    int ImagePitch = ImageWidth * 4;

    // Texture

    D3D11_TEXTURE2D_DESC ImageTextureDesc = {};

    ImageTextureDesc.Width = ImageWidth;
    ImageTextureDesc.Height = ImageHeight;
    ImageTextureDesc.MipLevels = 1;
    ImageTextureDesc.ArraySize = 1;
    ImageTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    ImageTextureDesc.SampleDesc.Count = 1;
    ImageTextureDesc.SampleDesc.Quality = 0;
    ImageTextureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    ImageTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA ImageSubresourceData = {};

    ImageSubresourceData.pSysMem = ImageData;
    ImageSubresourceData.SysMemPitch = ImagePitch;

    ID3D11Texture2D* ImageTexture;

    hr = device->CreateTexture2D(&ImageTextureDesc,
        &ImageSubresourceData,
        &ImageTexture
    );

    assert(SUCCEEDED(hr));

    free(ImageData);

    // Shader resource view

    hr = device->CreateShaderResourceView(ImageTexture,
        nullptr,
        m_textureView.GetAddressOf()
    );
    assert(SUCCEEDED(hr));
    ImageTexture->Release();

}

//-----------------------------------------------------------------------------
// Render the image.
//-----------------------------------------------------------------------------
void Renderer::Render()
{
    // Use the Direct3D device context to draw.
    ID3D11DeviceContext* context = m_deviceResources->GetDeviceContext();

    ID3D11RenderTargetView* renderTarget = m_deviceResources->GetRenderTarget();

    // Clear the back buffer to a deep blue
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    context->ClearRenderTargetView(renderTarget, clearColor);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(m_inputLayout.Get());
    context->VSSetShader(m_vertexShader.Get(), 0, 0);
    context->PSSetShader(m_pixelShader.Get(), 0, 0);
    context->PSSetShaderResources(0, 1, m_textureView.GetAddressOf());
    context->IASetVertexBuffers(0, 1, m_vertexDataBuffer.GetAddressOf(), &m_stride, &m_offset);
    context->Draw(m_numVertices, 0);
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