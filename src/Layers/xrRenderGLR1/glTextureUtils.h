#pragma once

#if defined(USE_OGL) || defined(USE_OGLR1)

namespace glTextureUtils
{
GLenum ConvertTextureFormat(D3DFORMAT dx9FMT);
}

#endif // USE_OGL
