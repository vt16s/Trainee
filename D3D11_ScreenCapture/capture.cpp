#include "capture.h"

HRESULT Capture::CreateDirect3DDevice()
{
    HRESULT hr = S_OK;

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
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };
    UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);

    D3D_FEATURE_LEVEL FeatureLevel;

    // Create device
    for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex)
    {
        hr = D3D11CreateDevice(nullptr, DriverTypes[DriverTypeIndex], nullptr, D3D11_CREATE_DEVICE_VIDEO_SUPPORT, FeatureLevels, NumFeatureLevels,
            D3D11_SDK_VERSION, &device, &FeatureLevel, &context);
        if (SUCCEEDED(hr))
        {
            // Device creation success, no need to loop anymore
            break;
        }
    }
    if (FAILED(hr))
        return hr;

    return S_OK;
}

bool Capture::Prepare(UINT Output)
{
    // Get DXGI device
    CComPtr<IDXGIDevice> lDxgiDevice;
    lDxgiDevice = device;
    if (!lDxgiDevice)
        return 0;

    // Get DXGI adapter
    HRESULT hr = 0;

    CComPtr<IDXGIAdapter> lDxgiAdapter;
    hr = lDxgiDevice->GetParent(
        __uuidof(IDXGIAdapter),
        reinterpret_cast<void**>(&lDxgiAdapter));

    if (FAILED(hr))
        return 0;

    lDxgiDevice = 0;

    // Get output
    CComPtr<IDXGIOutput> lDxgiOutput;
    hr = lDxgiAdapter->EnumOutputs(Output, &lDxgiOutput);
    if (FAILED(hr))
        return 0;

    lDxgiAdapter = 0;

    DXGI_OUTPUT_DESC lOutputDesc;
    hr = lDxgiOutput->GetDesc(&lOutputDesc);

    // QI for Output 1
    CComPtr<IDXGIOutput1> lDxgiOutput1;
    lDxgiOutput1 = lDxgiOutput;
    if (!lDxgiOutput1)
        return 0;

    lDxgiOutput = 0;

    // Create desktop duplication
    hr = lDxgiOutput1->DuplicateOutput(
        device,
        &lDeskDupl);

    if (FAILED(hr))
        return 0;

    lDxgiOutput1 = 0;

    // Create GUI drawing texture
    lDeskDupl->GetDesc(&lOutputDuplDesc);
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width              = lOutputDuplDesc.ModeDesc.Width;
    desc.Height             = lOutputDuplDesc.ModeDesc.Height;
    desc.Format             = lOutputDuplDesc.ModeDesc.Format;
    desc.ArraySize          = 1;
    desc.BindFlags          = D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET;
    desc.MiscFlags          = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.MipLevels          = 1;
    desc.CPUAccessFlags     = 0;
    desc.Usage              = D3D11_USAGE_DEFAULT;

    lGDIImage = 0;
    hr = device->CreateTexture2D(&desc, nullptr, &lGDIImage);
    if (FAILED(hr))
        return 0;

    if (lGDIImage == nullptr)
        return 0;

    // Create CPU access texture
    desc.Width              = lOutputDuplDesc.ModeDesc.Width;
    desc.Height             = lOutputDuplDesc.ModeDesc.Height;
    desc.Format             = lOutputDuplDesc.ModeDesc.Format;
    desc.ArraySize          = 1;
    desc.BindFlags          = 0;
    desc.MiscFlags          = 0;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.MipLevels          = 1;
    desc.CPUAccessFlags     = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    desc.Usage              = D3D11_USAGE_STAGING;

    lDestImage = 0;
    hr = device->CreateTexture2D(&desc, nullptr, &lDestImage);
    if (FAILED(hr))
        return 0;

    if (lDestImage == nullptr)
        return 0;

    return 1;
}

bool Capture::Get(IDXGIResource* lDesktopResource, RECT* rcx)
{
    // QI for ID3D11Texture2D
    CComPtr<ID3D11Texture2D> lAcquiredDesktopImage;
    if (!lDesktopResource)
        return 0;
    auto hr = lDesktopResource->QueryInterface(IID_PPV_ARGS(&lAcquiredDesktopImage));
    if (!lAcquiredDesktopImage)
        return 0;
    lDesktopResource = 0;

    // Copy image into GDI drawing texture
    context->CopyResource(lGDIImage, lAcquiredDesktopImage);

    // Draw cursor image into GDI drawing texture
    CComPtr<IDXGISurface1> lIDXGISurface1;

    lIDXGISurface1 = lGDIImage;

    if (!lIDXGISurface1)
        return 0;

    CURSORINFO lCursorInfo = { 0 };
    lCursorInfo.cbSize = sizeof(lCursorInfo);
    auto lBoolres = GetCursorInfo(&lCursorInfo);
    if (lBoolres == TRUE)
    {
        if (lCursorInfo.flags == CURSOR_SHOWING)
        {
            auto lCursorPosition = lCursorInfo.ptScreenPos;
            HDC  lHDC;
            lIDXGISurface1->GetDC(FALSE, &lHDC);
            DrawIconEx(
                lHDC,
                lCursorPosition.x,
                lCursorPosition.y,
                lCursorInfo.hCursor,
                0,
                0,
                0,
                0,
                DI_NORMAL | DI_DEFAULTSIZE);
            lIDXGISurface1->ReleaseDC(nullptr);
        }
    }

    // Copy image into CPU access texture
    context->CopyResource(lDestImage, lGDIImage);

    // Copy from CPU access texture to bitmap buffer
    D3D11_MAPPED_SUBRESOURCE resource;
    UINT subresource = D3D11CalcSubresource(0, 0, 0);
    hr = context->Map(lDestImage, subresource, D3D11_MAP_READ_WRITE, 0, &resource);
    if (FAILED(hr))
        return 0;

    auto sz = lOutputDuplDesc.ModeDesc.Width * lOutputDuplDesc.ModeDesc.Height * 4;
    auto sz2 = sz;
    buf.resize(sz);
    if (rcx)
    {
        sz2 = (rcx->right - rcx->left) * (rcx->bottom - rcx->top) * 4;
        buf.resize(sz2);
        sz = sz2;
    }

    UINT lBmpRowPitch = lOutputDuplDesc.ModeDesc.Width * 4;
    if (rcx)
        lBmpRowPitch = (rcx->right - rcx->left) * 4;
    UINT lRowPitch = std::min<UINT>(lBmpRowPitch, resource.RowPitch);

    BYTE* sptr = reinterpret_cast<BYTE*>(resource.pData);
    BYTE* dptr = buf.data() + sz - lBmpRowPitch;
    if (rcx)
        sptr += rcx->left * 4;
    for (size_t h = 0; h < lOutputDuplDesc.ModeDesc.Height; ++h)
    {
        if (rcx && h < (size_t)rcx->top)
        {
            sptr += resource.RowPitch;
            continue;
        }
        if (rcx && h >= (size_t)rcx->bottom)
            break;
        memcpy_s(dptr, lBmpRowPitch, sptr, lRowPitch);
        sptr += resource.RowPitch;
        dptr -= lBmpRowPitch;
    }
    context->Unmap(lDestImage, subresource);
    return 1;
}