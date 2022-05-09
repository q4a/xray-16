#pragma once

//#include "Common/Common.hpp"

#pragma warning(disable:4995)
#include "xrEngine/stdafx.h"
#pragma warning(default:4995)
#pragma warning(disable:4714)
#pragma warning( 4 : 4018 )
#pragma warning( 4 : 4244 )
#pragma warning(disable:4237)

#include "xrEngine/vis_common.h"
#include "xrEngine/Render.h"
#include "xrEngine/IGame_Level.h"

#include "xrParticles/psystem.h"

#define GLEW_STATIC
#include <GL/glew.h>
#include "SDL_opengl.h"
#include <GL/glu.h>
#include "SDL_opengl_glext.h"

#include "Layers/xrRenderGLR1/CommonTypes.h"

#include "Layers/xrRenderGLR1/glHW.h"
#include "Layers/xrRender/Debug/dxPixEventWrapper.h"

//#include "Layers/xrRender/BufferUtils.h"

#include "Layers/xrRender/Shader.h"

#include "Layers/xrRender/R_Backend.h"
#include "Layers/xrRender/R_Backend_Runtime.h"

#include "Layers/xrRender/Blender.h"
#include "Layers/xrRender/Blender_CLSID.h"

#define R_GL 0
#define R_GLR1 5
#define R_R1 1
#define R_R2 2
#define R_R3 3
#define R_R4 4
#define RENDER R_GLR1

#include "Common/_d3d_extensions.h"

#include "Layers/xrRender/ResourceManager.h"
#include "Layers/xrRender/xrRender_console.h"

#include "FStaticRender.h"

#define TEX_POINT_ATT "internal" DELIMITER "internal_light_attpoint"
#define TEX_SPOT_ATT "internal" DELIMITER "internal_light_attclip"

/*
IC void jitter(CBlender_Compile& C)
{
    C.r_Sampler("jitter0", JITTER(0), true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
    C.r_Sampler("jitter1", JITTER(1), true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
    C.r_Sampler("jitter2", JITTER(2), true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
    C.r_Sampler("jitter3", JITTER(3), true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
}
*/