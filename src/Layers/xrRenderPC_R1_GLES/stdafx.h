#pragma once

#include "Common/Common.hpp"

#include "xrEngine/stdafx.h"

#define GLEW_STATIC
#include <GL/glew.h>
//#include "SDL_opengles2.h"
#include <GL/glu.h>
//#include "SDL_opengles2_gl2ext.h"

#include "Layers/xrRenderGL/CommonTypes.h"
#include "Layers/xrRender/Debug/dxPixEventWrapper.h"

#define R_GL 0
#define R_R1 1
#define R_R2 2
#define R_R3 3
#define R_R4 4
#define RENDER R_R1

#include "xrParticles/psystem.h"

#include "Layers/xrRenderGLES/glesHW.h"
#include "Layers/xrRender/Shader.h"
#include "Layers/xrRender/R_Backend.h"
#include "Layers/xrRender/R_Backend_Runtime.h"

#include "Layers/xrRender/ResourceManager.h"

#include "xrEngine/vis_common.h"
#include "xrEngine/Render.h"
#include "Common/_d3d_extensions.h"
#include "xrEngine/IGame_Level.h"
#include "Layers/xrRender/blenders/Blender.h"
#include "Layers/xrRender/blenders/Blender_CLSID.h"
#include "Layers/xrRender/xrRender_console.h"
#include "rgles.h"

//#include "Layers/xrRender/Debug/dxPixEventWrapper.h"
//#include "Layers/xrRender/ResourceManager.h"
//#include "xrEngine/vis_common.h"
//#include "xrEngine/Render.h"
//#include "Common/_d3d_extensions.h"
#ifndef _EDITOR
//#include "xrEngine/IGame_Level.h"
//#include "Layers/xrRender/blenders/Blender.h"
//#include "Layers/xrRender/blenders/Blender_CLSID.h"
//#include "xrParticles/psystem.h"
//#include "Layers/xrRender/xrRender_console.h"
#include "rgles.h"
#endif

#define TEX_POINT_ATT "internal" DELIMITER "internal_light_attpoint"
#define TEX_SPOT_ATT "internal" DELIMITER "internal_light_attclip"
