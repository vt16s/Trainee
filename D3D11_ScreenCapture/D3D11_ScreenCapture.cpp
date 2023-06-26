// D3D11_ScreenCapture.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#define WIN32_LEAN_AND_MEAN
#define STRICT

#define REVERSE_IMAGE

#pragma comment(lib, "mfreadwrite")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")
#pragma comment(lib, "d3d11.lib")

#include <Windows.h>
#include <iostream>
#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>
#include <mferror.h>
#include <vector>
#include <atlbase.h>
#include <dxgi1_2.h>
#include "capture.h"

template <class T> void SafeRelease(T** ppT) {

    if (*ppT) {
        (*ppT)->Release();
        *ppT = nullptr;
    }
}

// Format constants
const UINT32 VIDEO_FPS             = 25;
const UINT64 VIDEO_FRAME_DURATION  = 10 * 1000 * 1000 / VIDEO_FPS;
const UINT32 VIDEO_BIT_RATE        = 1000000;
const GUID   VIDEO_ENCODING_FORMAT = MFVideoFormat_WMV3;
const GUID   VIDEO_INPUT_FORMAT    = MFVideoFormat_RGB32;
//const UINT32 VIDEO_FRAME_COUNT = 5 * VIDEO_FPS;

HRESULT InitializeSinkWriter(IMFSinkWriter** ppWriter, DWORD* pStreamIndex, const UINT32 uiWidth, const UINT32 uiHeight) {

    *ppWriter     = nullptr;
    *pStreamIndex = 0;

    IMFSinkWriter* pSinkWriter   = nullptr;
    IMFMediaType*  pMediaTypeOut = nullptr;
    IMFMediaType*  pMediaTypeIn  = nullptr;
    DWORD          streamIndex;

    HRESULT hr = MFCreateSinkWriterFromURL(L"output.wmv", nullptr, nullptr, &pSinkWriter);

    // Set the output media type.
    if (SUCCEEDED(hr)) {
        hr = MFCreateMediaType(&pMediaTypeOut);
    }
    if (SUCCEEDED(hr)) {
        hr = pMediaTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    }
    if (SUCCEEDED(hr)) {
        hr = pMediaTypeOut->SetGUID(MF_MT_SUBTYPE, VIDEO_ENCODING_FORMAT);
    }
    if (SUCCEEDED(hr)) {
        hr = pMediaTypeOut->SetUINT32(MF_MT_AVG_BITRATE, VIDEO_BIT_RATE);
    }
    if (SUCCEEDED(hr)) {
        hr = pMediaTypeOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    }
    if (SUCCEEDED(hr)) {
        hr = MFSetAttributeSize(pMediaTypeOut, MF_MT_FRAME_SIZE, uiWidth, uiHeight);
    }
    if (SUCCEEDED(hr)) {
        hr = MFSetAttributeRatio(pMediaTypeOut, MF_MT_FRAME_RATE, VIDEO_FPS, 1);
    }
    if (SUCCEEDED(hr)) {
        hr = MFSetAttributeRatio(pMediaTypeOut, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    }
    if (SUCCEEDED(hr)) {
        hr = pSinkWriter->AddStream(pMediaTypeOut, &streamIndex);
    }

    // Set the input media type.
    if (SUCCEEDED(hr)) {
        hr = MFCreateMediaType(&pMediaTypeIn);
    }
    if (SUCCEEDED(hr)) {
        hr = pMediaTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    }
    if (SUCCEEDED(hr)) {
        hr = pMediaTypeIn->SetGUID(MF_MT_SUBTYPE, VIDEO_INPUT_FORMAT);
    }
    if (SUCCEEDED(hr)) {
        hr = pMediaTypeIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    }
    if (SUCCEEDED(hr)) {
        hr = MFSetAttributeSize(pMediaTypeIn, MF_MT_FRAME_SIZE, uiWidth, uiHeight);
    }
    if (SUCCEEDED(hr)) {
        hr = MFSetAttributeRatio(pMediaTypeIn, MF_MT_FRAME_RATE, VIDEO_FPS, 1);
    }
    if (SUCCEEDED(hr)) {
        hr = MFSetAttributeRatio(pMediaTypeIn, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    }
    if (SUCCEEDED(hr)) {
        hr = pSinkWriter->SetInputMediaType(streamIndex, pMediaTypeIn, nullptr);
    }

    // Tell the sink writer to start accepting data.
    if (SUCCEEDED(hr)) {
        hr = pSinkWriter->BeginWriting();
    }

    // Return the pointer to the caller.
    if (SUCCEEDED(hr)) {

        *ppWriter = pSinkWriter;
        (*ppWriter)->AddRef();
        *pStreamIndex = streamIndex;
    }

    SafeRelease(&pSinkWriter);
    SafeRelease(&pMediaTypeOut);
    SafeRelease(&pMediaTypeIn);
    return hr;
}

HRESULT WriteFrame(std::vector<BYTE> buf, IMFSinkWriter* pWriter, DWORD streamIndex, const LONGLONG& rtStart, const UINT32 uiWidth, const UINT32 uiHeight)
{
    IMFSample* pSample = nullptr;
    IMFMediaBuffer* pBuffer = nullptr;

    const LONG cbWidth = 4 * uiWidth;
    const DWORD cbBuffer = cbWidth * uiHeight;

    BYTE* pData = nullptr;

    // Create a new memory buffer.
    HRESULT hr = MFCreateMemoryBuffer(cbBuffer, &pBuffer);

    // Lock the buffer and copy the video frame to the buffer.
    if (SUCCEEDED(hr))
    {
        hr = pBuffer->Lock(&pData, nullptr, nullptr);
    }
    if (SUCCEEDED(hr))
    {
#ifdef REVERSE_IMAGE
        for (int i = 0, j = uiHeight - 1; (UINT32)i < uiHeight; i++, j--)
            for (int k = 0; k < cbWidth; k++)
                pData[(i * cbWidth) + k] = buf[(j * cbWidth) + k];
#else
        hr = MFCopyImage(pData, cbWidth, buf.data(), cbWidth, cbWidth, uiHeight);
#endif
    }
    if (pBuffer)
    {
        pBuffer->Unlock();
    }

    // Set the data length of the buffer.
    if (SUCCEEDED(hr))
    {
        hr = pBuffer->SetCurrentLength(cbBuffer);
    }

    // Create a media sample and add the buffer to the sample.
    if (SUCCEEDED(hr))
    {
        hr = MFCreateSample(&pSample);
    }
    if (SUCCEEDED(hr))
    {
        hr = pSample->AddBuffer(pBuffer);
    }

    // Set the time stamp and the duration.
    if (SUCCEEDED(hr))
    {
        hr = pSample->SetSampleTime(rtStart);
    }
    if (SUCCEEDED(hr))
    {
        hr = pSample->SetSampleDuration(VIDEO_FRAME_DURATION);
    }

    // Send the sample to the Sink Writer.
    if (SUCCEEDED(hr))
    {
        hr = pWriter->WriteSample(streamIndex, pSample);
    }

    SafeRelease(&pSample);
    SafeRelease(&pBuffer);
    return hr;
}

int main()
{

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    if (SUCCEEDED(hr)) {

        hr = MFStartup(MF_VERSION);

        if (SUCCEEDED(hr))
        {
            Capture cap;

            UINT32 uiWidth = 0;
            UINT32 uiHeight = 0;

            if (FAILED(cap.CreateDirect3DDevice()))
                return -1;
            if (!cap.Prepare())
                return -2;
            uiWidth = cap.lOutputDuplDesc.ModeDesc.Width;
            uiHeight = cap.lOutputDuplDesc.ModeDesc.Height;

            IMFSinkWriter* pSinkWriter = nullptr;
            DWORD stream;

            hr = InitializeSinkWriter(&pSinkWriter, &stream, uiWidth, uiHeight);

            if (SUCCEEDED(hr))
            {
                std::cout << "Screen recording in progress. Press Esc to stop";

                LONGLONG rtStart = 0;

                for (;;) // for (DWORD i = 0; i < VIDEO_FRAME_COUNT; ++i) 
                {
                    if (GetAsyncKeyState(VK_ESCAPE) & 0x01)
                        break;

                    CComPtr<IDXGIResource> lDesktopResource;
                    DXGI_OUTDUPL_FRAME_INFO lFrameInfo;

                    hr = cap.lDeskDupl->AcquireNextFrame(
                        0,
                        &lFrameInfo,
                        &lDesktopResource);
                    if (hr == DXGI_ERROR_WAIT_TIMEOUT)
                        hr = S_OK;
                    if (hr == DXGI_ERROR_ACCESS_LOST)
                    {
                        cap.lDeskDupl = 0;
                        bool C = false;
                        for (int i = 0; i < 10; i++)
                        {
                            if (cap.Prepare())
                            {
                                C = true;
                                break;
                            }
                            Sleep(250);
                        }
                        if (!C)
                            break;
                        hr = S_OK;
                    }
                    if (FAILED(hr))
                        break;

                    if (lDesktopResource && !cap.Get(lDesktopResource))
                        break;

                    hr = WriteFrame(cap.buf, pSinkWriter, stream, rtStart, uiWidth, uiHeight);

                    if (FAILED(hr)) {
                        break;
                    }
                    cap.lDeskDupl->ReleaseFrame();
                    rtStart += VIDEO_FRAME_DURATION;
                }
            }

            MFShutdown();
        }

        CoUninitialize();
    }
    return 0;
}