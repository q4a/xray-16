#pragma once

#include "xrCore/ModuleLookup.hpp"

#include "Layers/xrRender/HWCaps.h"
#include "Layers/xrRender/stats_manager.h"

#include <SDL.h>
#include <SDL_syswm.h>

#if !defined(XR_PLATFORM_WINDOWS) // USE_LINUX D3DXDeclaratorFromFVF

// wine/dlls/d3dx9_36/mesh.c

static const UINT d3dx_decltype_size[] =
    {
        /* D3DDECLTYPE_FLOAT1    */ sizeof(FLOAT),
        /* D3DDECLTYPE_FLOAT2    */ sizeof(float)*2,  //sizeof(D3DXVECTOR2)
        /* D3DDECLTYPE_FLOAT3    */ sizeof(D3DVECTOR),//sizeof(D3DXVECTOR3)
        /* D3DDECLTYPE_FLOAT4    */ sizeof(float)*4,  //sizeof(D3DXVECTOR4),
        /* D3DDECLTYPE_D3DCOLOR  */ sizeof(D3DCOLOR),
        /* D3DDECLTYPE_UBYTE4    */ 4 * sizeof(BYTE),
        /* D3DDECLTYPE_SHORT2    */ 2 * sizeof(SHORT),
        /* D3DDECLTYPE_SHORT4    */ 4 * sizeof(SHORT),
        /* D3DDECLTYPE_UBYTE4N   */ 4 * sizeof(BYTE),
        /* D3DDECLTYPE_SHORT2N   */ 2 * sizeof(SHORT),
        /* D3DDECLTYPE_SHORT4N   */ 4 * sizeof(SHORT),
        /* D3DDECLTYPE_USHORT2N  */ 2 * sizeof(USHORT),
        /* D3DDECLTYPE_USHORT4N  */ 4 * sizeof(USHORT),
        /* D3DDECLTYPE_UDEC3     */ 4, /* 3 * 10 bits + 2 padding */
        /* D3DDECLTYPE_DEC3N     */ 4,
        /* D3DDECLTYPE_FLOAT16_2 */ 2 * sizeof(WORD),//2 * sizeof(D3DXFLOAT16)
        /* D3DDECLTYPE_FLOAT16_4 */ 4 * sizeof(WORD),//4 * sizeof(D3DXFLOAT16)
};

#endif

class CHW
    : public pureAppActivate,
      public pureAppDeactivate
{
public:
    CHW();
    ~CHW();

    void CreateD3D();
    void DestroyD3D();

    void CreateDevice(SDL_Window* sdlWnd);
    void DestroyDevice();

    void Reset();

    void SetPrimaryAttributes(u32& windowFlags);

    BOOL support(D3DFORMAT fmt, u32 type, u32 usage) const;
    static bool GivenGPUIsIntelGMA(u32 id_vendor, u32 id_device);

    std::pair<u32, u32> GetSurfaceSize() const;
    DeviceState GetDeviceState() const;

public:
    void BeginScene();
    void EndScene();
    void Present();

public:
    void OnAppActivate() override;
    void OnAppDeactivate() override;

public:
    void BeginPixEvent(LPCWSTR wszName) const;
    void EndPixEvent() const;

private:
    u32 selectPresentInterval() const;
    u32 selectGPU() const;
    D3DFORMAT selectDepthStencil(D3DFORMAT) const;
    bool ThisInstanceIsGlobal() const;

public:
    CHWCaps Caps;

    u32 BackBufferCount{};
    u32 CurrentBackBuffer{};

    ID3DDevice* pDevice = nullptr; // render device

    D3D_DRIVER_TYPE m_DriverType;

#ifdef DEBUG
    IDirect3DStateBlock9* dwDebugSB = nullptr;
#endif

    IDirect3D9* pD3D = nullptr; // D3D

    u32 DevAdapter;

    decltype(&D3DPERF_BeginEvent) d3dperf_BeginEvent = nullptr;
    decltype(&D3DPERF_EndEvent) d3dperf_EndEvent = nullptr;

#if !defined(_MAYA_EXPORT)
    stats_manager m_stats_manager;
#endif

private:
    D3DPRESENT_PARAMETERS DevPP;
    XRay::Module hD3D = nullptr;
};

extern ECORE_API CHW HW;
