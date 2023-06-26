#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <vector>
#include <atlbase.h>

class Capture
{
public:
    std::vector<BYTE> buf;   // bitmap buffer
    CComPtr<IDXGIOutputDuplication> lDeskDupl;
    DXGI_OUTDUPL_DESC lOutputDuplDesc = {};

    HRESULT CreateDirect3DDevice();   // Instantiating a DirectX 11 device
    bool Prepare(UINT Output = 0);    // Creating the Desktop Duplication
    bool Get(IDXGIResource* lDesktopResource, RECT* rcx = 0);  // Creating the bitmap of the desctop

private:
    CComPtr<ID3D11Device> device;
    CComPtr<ID3D11DeviceContext> context;
    CComPtr<ID3D11Texture2D> lGDIImage;
    CComPtr<ID3D11Texture2D> lDestImage;

};

