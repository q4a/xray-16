#include "stdafx.h"
#include "Layers/xrRender/dxRenderFactory.h"
#include "Layers/xrRender/dxUIRender.h"
#include "Layers/xrRender/dxDebugRender.h"
#include "Layers/xrRender/D3DUtils.h"

constexpr pcstr RENDERER_RGLR1_MODE = "renderer_rglr1";

class RGLRendererModule final : public RendererModule
{
    xr_vector<pcstr> modes;

public:
    bool CheckCanAddMode() const
    {
        // don't duplicate
        if (!modes.empty())
        {
            return false;
        }
        // Check if shaders are available
        if (!FS.exist("$game_shaders$", RImplementation.getShaderPath()))
        {
            Log("~ No shaders found for xrRender_GLR1");
            return false;
        }
        return xrRender_test_hw;
    }

    const xr_vector<pcstr>& ObtainSupportedModes() override
    {
        if (CheckCanAddMode())
        {
            modes.emplace_back(RENDERER_RGLR1_MODE);
        }
        return modes;
    }

    void CheckModeConsistency(pcstr mode) const
    {
        R_ASSERT3(0 == xr_strcmp(mode, RENDERER_RGLR1_MODE),
            "Wrong mode passed to xrRender_GLR1", mode);
    }

    void SetupEnv(pcstr mode) override
    {
        CheckModeConsistency(mode);
        GEnv.Render = &RImplementation;
        GEnv.RenderFactory = &RenderFactoryImpl;
        GEnv.DU = &DUImpl;
        GEnv.UIRender = &UIRenderImpl;
#ifdef DEBUG
        GEnv.DRender = &DebugRenderImpl;
#endif
        xrRender_initconsole();
    }
} static s_rglr1_module;

extern "C"
{
XR_EXPORT RendererModule* GetRendererModule()
{
    return &s_rglr1_module;
}
}
