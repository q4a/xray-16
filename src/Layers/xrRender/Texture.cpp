// Texture.cpp: implementation of the CTexture class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#if !defined(XR_PLATFORM_WINDOWS)
#include <gli/gli.hpp>

/************************************************************
 * helper functions for D3DXLoadSurfaceFromSurface
 */

// wine-8.2/include/d3dx9.h

#define D3DX_DEFAULT         ((UINT)-1)

// wine-8.2/include/d3dx9tex.h

#define D3DX_FILTER_NONE                 0x00000001
#define D3DX_FILTER_POINT                0x00000002
#define D3DX_FILTER_LINEAR               0x00000003
#define D3DX_FILTER_TRIANGLE             0x00000004
#define D3DX_FILTER_BOX                  0x00000005
#define D3DX_FILTER_MIRROR_U             0x00010000
#define D3DX_FILTER_MIRROR_V             0x00020000
#define D3DX_FILTER_MIRROR_W             0x00040000
#define D3DX_FILTER_MIRROR               0x00070000
#define D3DX_FILTER_DITHER               0x00080000

// wine-8.2/include/winuser.h

static inline BOOL SetRect(LPRECT rect, INT left, INT top, INT right, INT bottom)
{
    if (!rect) return FALSE;
    rect->left   = left;
    rect->right  = right;
    rect->top    = top;
    rect->bottom = bottom;
    return TRUE;
}

// wine-8.2/dlls/d3dx9_36/d3dx9_private.h

struct vec4
{
    float x, y, z, w;
};

struct volume
{
    UINT width;
    UINT height;
    UINT depth;
};

/* for internal use */
enum format_type {
    FORMAT_ARGB,   /* unsigned */
    FORMAT_ARGBF16,/* float 16 */
    FORMAT_ARGBF,  /* float */
    FORMAT_DXT,
    FORMAT_INDEX,
    FORMAT_UNKNOWN
};

struct pixel_format_desc {
    D3DFORMAT format;
    BYTE bits[4];
    BYTE shift[4];
    UINT bytes_per_pixel;
    UINT block_width;
    UINT block_height;
    UINT block_byte_count;
    enum format_type type;
    void (*from_rgba)(const struct vec4 *src, struct vec4 *dst);
    void (*to_rgba)(const struct vec4 *src, struct vec4 *dst, const PALETTEENTRY *palette);
};

static inline BOOL is_conversion_from_supported(const struct pixel_format_desc *format)
{
    if (format->type == FORMAT_ARGB || format->type == FORMAT_ARGBF16
        || format->type == FORMAT_ARGBF || format->type == FORMAT_DXT)
        return TRUE;
    return !!format->to_rgba;
}

static inline BOOL is_conversion_to_supported(const struct pixel_format_desc *format)
{
    if (format->type == FORMAT_ARGB || format->type == FORMAT_ARGBF16
        || format->type == FORMAT_ARGBF || format->type == FORMAT_DXT)
        return TRUE;
    return !!format->from_rgba;
}

// wine-8.2/dlls/d3dx9_36/util.c

static void la_from_rgba(const struct vec4 *rgba, struct vec4 *la)
{
    la->x = rgba->x * 0.2125f + rgba->y * 0.7154f + rgba->z * 0.0721f;
    la->w = rgba->w;
}

static void la_to_rgba(const struct vec4 *la, struct vec4 *rgba, const PALETTEENTRY *palette)
{
    rgba->x = la->x;
    rgba->y = la->x;
    rgba->z = la->x;
    rgba->w = la->w;
}

static void index_to_rgba(const struct vec4 *index, struct vec4 *rgba, const PALETTEENTRY *palette)
{
    ULONG idx = (ULONG)(index->x * 255.0f + 0.5f);

    rgba->x = palette[idx].peRed / 255.0f;
    rgba->y = palette[idx].peGreen / 255.0f;
    rgba->z = palette[idx].peBlue / 255.0f;
    rgba->w = palette[idx].peFlags / 255.0f; /* peFlags is the alpha component in DX8 and higher */
}

/************************************************************
 * pixel format table providing info about number of bytes per pixel,
 * number of bits per channel and format type.
 *
 * Call get_format_info to request information about a specific format.
 */
static const struct pixel_format_desc formats[] =
    {
        /* format              bpc               shifts             bpp blocks   type            from_rgba     to_rgba */
        {D3DFMT_R8G8B8,        { 0,  8,  8,  8}, { 0, 16,  8,  0},  3, 1, 1,  3, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_A8R8G8B8,      { 8,  8,  8,  8}, {24, 16,  8,  0},  4, 1, 1,  4, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_X8R8G8B8,      { 0,  8,  8,  8}, { 0, 16,  8,  0},  4, 1, 1,  4, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_A8B8G8R8,      { 8,  8,  8,  8}, {24,  0,  8, 16},  4, 1, 1,  4, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_X8B8G8R8,      { 0,  8,  8,  8}, { 0,  0,  8, 16},  4, 1, 1,  4, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_R5G6B5,        { 0,  5,  6,  5}, { 0, 11,  5,  0},  2, 1, 1,  2, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_X1R5G5B5,      { 0,  5,  5,  5}, { 0, 10,  5,  0},  2, 1, 1,  2, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_A1R5G5B5,      { 1,  5,  5,  5}, {15, 10,  5,  0},  2, 1, 1,  2, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_R3G3B2,        { 0,  3,  3,  2}, { 0,  5,  2,  0},  1, 1, 1,  1, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_A8R3G3B2,      { 8,  3,  3,  2}, { 8,  5,  2,  0},  2, 1, 1,  2, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_A4R4G4B4,      { 4,  4,  4,  4}, {12,  8,  4,  0},  2, 1, 1,  2, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_X4R4G4B4,      { 0,  4,  4,  4}, { 0,  8,  4,  0},  2, 1, 1,  2, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_A2R10G10B10,   { 2, 10, 10, 10}, {30, 20, 10,  0},  4, 1, 1,  4, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_A2B10G10R10,   { 2, 10, 10, 10}, {30,  0, 10, 20},  4, 1, 1,  4, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_A16B16G16R16,  {16, 16, 16, 16}, {48,  0, 16, 32},  8, 1, 1,  8, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_G16R16,        { 0, 16, 16,  0}, { 0,  0, 16,  0},  4, 1, 1,  4, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_A8,            { 8,  0,  0,  0}, { 0,  0,  0,  0},  1, 1, 1,  1, FORMAT_ARGB,    NULL,         NULL      },
        {D3DFMT_A8L8,          { 8,  8,  0,  0}, { 8,  0,  0,  0},  2, 1, 1,  2, FORMAT_ARGB,    la_from_rgba, la_to_rgba},
        {D3DFMT_A4L4,          { 4,  4,  0,  0}, { 4,  0,  0,  0},  1, 1, 1,  1, FORMAT_ARGB,    la_from_rgba, la_to_rgba},
        {D3DFMT_L8,            { 0,  8,  0,  0}, { 0,  0,  0,  0},  1, 1, 1,  1, FORMAT_ARGB,    la_from_rgba, la_to_rgba},
        {D3DFMT_L16,           { 0, 16,  0,  0}, { 0,  0,  0,  0},  2, 1, 1,  2, FORMAT_ARGB,    la_from_rgba, la_to_rgba},
        {D3DFMT_DXT1,          { 0,  0,  0,  0}, { 0,  0,  0,  0},  1, 4, 4,  8, FORMAT_DXT,     NULL,         NULL      },
        {D3DFMT_DXT2,          { 0,  0,  0,  0}, { 0,  0,  0,  0},  1, 4, 4, 16, FORMAT_DXT,     NULL,         NULL      },
        {D3DFMT_DXT3,          { 0,  0,  0,  0}, { 0,  0,  0,  0},  1, 4, 4, 16, FORMAT_DXT,     NULL,         NULL      },
        {D3DFMT_DXT4,          { 0,  0,  0,  0}, { 0,  0,  0,  0},  1, 4, 4, 16, FORMAT_DXT,     NULL,         NULL      },
        {D3DFMT_DXT5,          { 0,  0,  0,  0}, { 0,  0,  0,  0},  1, 4, 4, 16, FORMAT_DXT,     NULL,         NULL      },
        {D3DFMT_R16F,          { 0, 16,  0,  0}, { 0,  0,  0,  0},  2, 1, 1,  2, FORMAT_ARGBF16, NULL,         NULL      },
        {D3DFMT_G16R16F,       { 0, 16, 16,  0}, { 0,  0, 16,  0},  4, 1, 1,  4, FORMAT_ARGBF16, NULL,         NULL      },
        {D3DFMT_A16B16G16R16F, {16, 16, 16, 16}, {48,  0, 16, 32},  8, 1, 1,  8, FORMAT_ARGBF16, NULL,         NULL      },
        {D3DFMT_R32F,          { 0, 32,  0,  0}, { 0,  0,  0,  0},  4, 1, 1,  4, FORMAT_ARGBF,   NULL,         NULL      },
        {D3DFMT_G32R32F,       { 0, 32, 32,  0}, { 0,  0, 32,  0},  8, 1, 1,  8, FORMAT_ARGBF,   NULL,         NULL      },
        {D3DFMT_A32B32G32R32F, {32, 32, 32, 32}, {96,  0, 32, 64}, 16, 1, 1, 16, FORMAT_ARGBF,   NULL,         NULL      },
        {D3DFMT_P8,            { 8,  8,  8,  8}, { 0,  0,  0,  0},  1, 1, 1,  1, FORMAT_INDEX,   NULL,         index_to_rgba},
        /* marks last element */
        {D3DFMT_UNKNOWN,       { 0,  0,  0,  0}, { 0,  0,  0,  0},  0, 1, 1,  0, FORMAT_UNKNOWN, NULL,         NULL      },
        };

/************************************************************
 * get_format_info
 *
 * Returns information about the specified format.
 * If the format is unsupported, it's filled with the D3DFMT_UNKNOWN desc.
 *
 * PARAMS
 *   format [I] format whose description is queried
 *
 */
const struct pixel_format_desc *get_format_info(D3DFORMAT format)
{
    unsigned int i = 0;
    while(formats[i].format != format && formats[i].format != D3DFMT_UNKNOWN) i++;
//    if (formats[i].format == D3DFMT_UNKNOWN)
//        FIXME("Unknown format %#x (as FOURCC %s).\n", format, debugstr_an((const char *)&format, 4));
    return &formats[i];
}

// wine-8.2/dlls/d3dx9_36/surface.c

HRESULT lock_surface(IDirect3DSurface9 *surface, const RECT *surface_rect, D3DLOCKED_RECT *lock,
                     IDirect3DSurface9 **temp_surface, BOOL write)
{
    unsigned int width, height;
    IDirect3DDevice9 *device;
    D3DSURFACE_DESC desc;
    DWORD lock_flag;
    HRESULT hr;

    lock_flag = write ? 0 : D3DLOCK_READONLY;
    *temp_surface = NULL;
    if (FAILED(hr = IDirect3DSurface9_LockRect(surface, lock, surface_rect, lock_flag)))
    {
        IDirect3DSurface9_GetDevice(surface, &device);
        IDirect3DSurface9_GetDesc(surface, &desc);

        if (surface_rect)
        {
            width = surface_rect->right - surface_rect->left;
            height = surface_rect->bottom - surface_rect->top;
        }
        else
        {
            width = desc.Width;
            height = desc.Height;
        }

        hr = write ? IDirect3DDevice9_CreateOffscreenPlainSurface(device, width, height,
                                                                  desc.Format, D3DPOOL_SYSTEMMEM, temp_surface, NULL)
                   : IDirect3DDevice9_CreateRenderTarget(device, width, height,
                                                         desc.Format, D3DMULTISAMPLE_NONE, 0, TRUE, temp_surface, NULL);
        if (FAILED(hr))
        {
//            WARN("Failed to create temporary surface, surface %p, format %#x, "
//                 "usage %#lx, pool %#x, write %#x, width %u, height %u.\n",
//                 surface, desc.Format, desc.Usage, desc.Pool, write, width, height);
            IDirect3DDevice9_Release(device);
            return hr;
        }

        if (write || SUCCEEDED(hr = IDirect3DDevice9_StretchRect(device, surface, surface_rect,
                                                                 *temp_surface, NULL, D3DTEXF_NONE)))
            hr = IDirect3DSurface9_LockRect(*temp_surface, lock, NULL, lock_flag);

        IDirect3DDevice9_Release(device);
        if (FAILED(hr))
        {
//            WARN("Failed to lock surface %p, write %#x, usage %#lx, pool %#x.\n",
//                 surface, write, desc.Usage, desc.Pool);
            IDirect3DSurface9_Release(*temp_surface);
            *temp_surface = NULL;
            return hr;
        }
//        TRACE("Created temporary surface %p.\n", surface);
    }
    return hr;
}

HRESULT unlock_surface(IDirect3DSurface9 *surface, const RECT *surface_rect,
                       IDirect3DSurface9 *temp_surface, BOOL update)
{
    IDirect3DDevice9 *device;
    POINT surface_point;
    HRESULT hr;

    if (!temp_surface)
    {
        hr = IDirect3DSurface9_UnlockRect(surface);
        return hr;
    }

    hr = IDirect3DSurface9_UnlockRect(temp_surface);
    if (update)
    {
        if (surface_rect)
        {
            surface_point.x = surface_rect->left;
            surface_point.y = surface_rect->top;
        }
        else
        {
            surface_point.x = 0;
            surface_point.y = 0;
        }
        IDirect3DSurface9_GetDevice(surface, &device);
//        if (FAILED(hr = IDirect3DDevice9_UpdateSurface(device, temp_surface, NULL, surface, &surface_point)))
//            WARN("Updating surface failed, hr %#lx, surface %p, temp_surface %p.\n",
//                 hr, surface, temp_surface);
        IDirect3DDevice9_Release(device);
    }
    IDirect3DSurface9_Release(temp_surface);
    return hr;
}

/************************************************************
 * helper functions for D3DXLoadSurfaceFromMemory
 */
struct argb_conversion_info
{
    const struct pixel_format_desc *srcformat;
    const struct pixel_format_desc *destformat;
    DWORD srcshift[4], destshift[4];
    DWORD srcmask[4], destmask[4];
    BOOL process_channel[4];
    DWORD channelmask;
};

static void init_argb_conversion_info(const struct pixel_format_desc *srcformat, const struct pixel_format_desc *destformat, struct argb_conversion_info *info)
{
    UINT i;
    ZeroMemory(info->process_channel, 4 * sizeof(BOOL));
    info->channelmask = 0;

    info->srcformat  =  srcformat;
    info->destformat = destformat;

    for(i = 0;i < 4;i++) {
        /* srcshift is used to extract the _relevant_ components */
        info->srcshift[i]  =  srcformat->shift[i] + std::max( srcformat->bits[i] - destformat->bits[i], 0);

        /* destshift is used to move the components to the correct position */
        info->destshift[i] = destformat->shift[i] + std::max(destformat->bits[i] -  srcformat->bits[i], 0);

        info->srcmask[i]  = ((1 <<  srcformat->bits[i]) - 1) <<  srcformat->shift[i];
        info->destmask[i] = ((1 << destformat->bits[i]) - 1) << destformat->shift[i];

        /* channelmask specifies bits which aren't used in the source format but in the destination one */
        if(destformat->bits[i]) {
            if(srcformat->bits[i]) info->process_channel[i] = TRUE;
            else info->channelmask |= info->destmask[i];
        }
    }
}

/************************************************************
 * get_relevant_argb_components
 *
 * Extracts the relevant components from the source color and
 * drops the less significant bits if they aren't used by the destination format.
 */
static void get_relevant_argb_components(const struct argb_conversion_info *info, const BYTE *col, DWORD *out)
{
    unsigned int i, j;
    unsigned int component, mask;

    for (i = 0; i < 4; ++i)
    {
        if (!info->process_channel[i])
            continue;

        component = 0;
        mask = info->srcmask[i];
        for (j = 0; j < 4 && mask; ++j)
        {
            if (info->srcshift[i] < j * 8)
                component |= (col[j] & mask) << (j * 8 - info->srcshift[i]);
            else
                component |= (col[j] & mask) >> (info->srcshift[i] - j * 8);
            mask >>= 8;
        }
        out[i] = component;
    }
}

/************************************************************
 * make_argb_color
 *
 * Recombines the output of get_relevant_argb_components and converts
 * it to the destination format.
 */
static DWORD make_argb_color(const struct argb_conversion_info *info, const DWORD *in)
{
    UINT i;
    DWORD val = 0;

    for(i = 0;i < 4;i++) {
        if(info->process_channel[i]) {
            /* necessary to make sure that e.g. an X4R4G4B4 white maps to an R8G8B8 white instead of 0xf0f0f0 */
            signed int shift;
            for(shift = info->destshift[i]; shift > info->destformat->shift[i]; shift -= info->srcformat->bits[i]) val |= in[i] << shift;
            val |= (in[i] >> (info->destformat->shift[i] - shift)) << info->destformat->shift[i];
        }
    }
    val |= info->channelmask;   /* new channels are set to their maximal value */
    return val;
}

/* It doesn't work for components bigger than 32 bits (or somewhat smaller but unaligned). */
void format_to_vec4(const struct pixel_format_desc *format, const BYTE *src, struct vec4 *dst)
{
    DWORD mask, tmp;
    unsigned int c;

    for (c = 0; c < 4; ++c)
    {
        static const unsigned int component_offsets[4] = {3, 0, 1, 2};
        float *dst_component = (float *)dst + component_offsets[c];

        if (format->bits[c])
        {
            mask = ~0u >> (32 - format->bits[c]);

            memcpy(&tmp, src + format->shift[c] / 8,
                   std::min(sizeof(DWORD), static_cast<ulong>((format->shift[c] % 8 + format->bits[c] + 7) / 8)));

            if (format->type == FORMAT_ARGBF16)
            {
                Msg("ERR Unimplemented for FORMAT_ARGBF166\n");
//                *dst_component = float_16_to_32(tmp);
            }
            else if (format->type == FORMAT_ARGBF)
                *dst_component = *(float *)&tmp;
            else
                *dst_component = (float)((tmp >> format->shift[c] % 8) & mask) / mask;
        }
        else
            *dst_component = 1.0f;
    }
}

/* It doesn't work for components bigger than 32 bits. */
static void format_from_vec4(const struct pixel_format_desc *format, const struct vec4 *src, BYTE *dst)
{
    DWORD v, mask32;
    unsigned int c, i;

    memset(dst, 0, format->bytes_per_pixel);

    for (c = 0; c < 4; ++c)
    {
        static const unsigned int component_offsets[4] = {3, 0, 1, 2};
        const float src_component = *((const float *)src + component_offsets[c]);

        if (!format->bits[c])
            continue;

        mask32 = ~0u >> (32 - format->bits[c]);

        if (format->type == FORMAT_ARGBF16)
        {
            Msg("ERR Unimplemented for FORMAT_ARGBF16\n");
//            v = float_32_to_16(src_component);
        }
        else if (format->type == FORMAT_ARGBF)
            v = *(DWORD *)&src_component;
        else
            v = (DWORD)(src_component * ((1 << format->bits[c]) - 1) + 0.5f);

        for (i = format->shift[c] / 8 * 8; i < format->shift[c] + format->bits[c]; i += 8)
        {
            BYTE mask, byte;

            if (format->shift[c] > i)
            {
                mask = mask32 << (format->shift[c] - i);
                byte = (v << (format->shift[c] - i)) & mask;
            }
            else
            {
                mask = mask32 >> (i - format->shift[c]);
                byte = (v >> (i - format->shift[c])) & mask;
            }
            dst[i / 8] |= byte;
        }
    }
}

/************************************************************
 * copy_pixels
 *
 * Copies the source buffer to the destination buffer.
 * Works for any pixel format.
 * The source and the destination must be block-aligned.
 */
void copy_pixels(const BYTE *src, UINT src_row_pitch, UINT src_slice_pitch,
                 BYTE *dst, UINT dst_row_pitch, UINT dst_slice_pitch, const struct volume *size,
                 const struct pixel_format_desc *format)
{
    UINT row, slice;
    BYTE *dst_addr;
    const BYTE *src_addr;
    UINT row_block_count = (size->width + format->block_width - 1) / format->block_width;
    UINT row_count = (size->height + format->block_height - 1) / format->block_height;

    for (slice = 0; slice < size->depth; slice++)
    {
        src_addr = src + slice * src_slice_pitch;
        dst_addr = dst + slice * dst_slice_pitch;

        for (row = 0; row < row_count; row++)
        {
            memcpy(dst_addr, src_addr, row_block_count * format->block_byte_count);
            src_addr += src_row_pitch;
            dst_addr += dst_row_pitch;
        }
    }
}

/************************************************************
 * convert_argb_pixels
 *
 * Copies the source buffer to the destination buffer, performing
 * any necessary format conversion and color keying.
 * Pixels outsize the source rect are blacked out.
 */
void convert_argb_pixels(const BYTE *src, UINT src_row_pitch, UINT src_slice_pitch, const struct volume *src_size,
                         const struct pixel_format_desc *src_format, BYTE *dst, UINT dst_row_pitch, UINT dst_slice_pitch,
                         const struct volume *dst_size, const struct pixel_format_desc *dst_format, D3DCOLOR color_key,
                         const PALETTEENTRY *palette)
{
    struct argb_conversion_info conv_info, ck_conv_info;
    const struct pixel_format_desc *ck_format = NULL;
    DWORD channels[4];
    UINT min_width, min_height, min_depth;
    UINT x, y, z;

//    TRACE("src %p, src_row_pitch %u, src_slice_pitch %u, src_size %p, src_format %p, dst %p, "
//          "dst_row_pitch %u, dst_slice_pitch %u, dst_size %p, dst_format %p, color_key 0x%08lx, palette %p.\n",
//          src, src_row_pitch, src_slice_pitch, src_size, src_format, dst, dst_row_pitch, dst_slice_pitch, dst_size,
//          dst_format, color_key, palette);

    ZeroMemory(channels, sizeof(channels));
    init_argb_conversion_info(src_format, dst_format, &conv_info);

    min_width = std::min(src_size->width, dst_size->width);
    min_height = std::min(src_size->height, dst_size->height);
    min_depth = std::min(src_size->depth, dst_size->depth);

    if (color_key)
    {
        /* Color keys are always represented in D3DFMT_A8R8G8B8 format. */
        ck_format = get_format_info(D3DFMT_A8R8G8B8);
        init_argb_conversion_info(src_format, ck_format, &ck_conv_info);
    }

    for (z = 0; z < min_depth; z++) {
        const BYTE *src_slice_ptr = src + z * src_slice_pitch;
        BYTE *dst_slice_ptr = dst + z * dst_slice_pitch;

        for (y = 0; y < min_height; y++) {
            const BYTE *src_ptr = src_slice_ptr + y * src_row_pitch;
            BYTE *dst_ptr = dst_slice_ptr + y * dst_row_pitch;

            for (x = 0; x < min_width; x++) {
                if (!src_format->to_rgba && !dst_format->from_rgba
                    && src_format->type == dst_format->type
                    && src_format->bytes_per_pixel <= 4 && dst_format->bytes_per_pixel <= 4)
                {
                    DWORD val;

                    get_relevant_argb_components(&conv_info, src_ptr, channels);
                    val = make_argb_color(&conv_info, channels);

                    if (color_key)
                    {
                        DWORD ck_pixel;

                        get_relevant_argb_components(&ck_conv_info, src_ptr, channels);
                        ck_pixel = make_argb_color(&ck_conv_info, channels);
                        if (ck_pixel == color_key)
                            val &= ~conv_info.destmask[0];
                    }
                    memcpy(dst_ptr, &val, dst_format->bytes_per_pixel);
                }
                else
                {
                    struct vec4 color, tmp;

                    format_to_vec4(src_format, src_ptr, &color);
                    if (src_format->to_rgba)
                        src_format->to_rgba(&color, &tmp, palette);
                    else
                        tmp = color;

                    if (ck_format)
                    {
                        DWORD ck_pixel;

                        format_from_vec4(ck_format, &tmp, (BYTE *)&ck_pixel);
                        if (ck_pixel == color_key)
                            tmp.w = 0.0f;
                    }

                    if (dst_format->from_rgba)
                        dst_format->from_rgba(&tmp, &color);
                    else
                        color = tmp;

                    format_from_vec4(dst_format, &color, dst_ptr);
                }

                src_ptr += src_format->bytes_per_pixel;
                dst_ptr += dst_format->bytes_per_pixel;
            }

            if (src_size->width < dst_size->width) /* black out remaining pixels */
                memset(dst_ptr, 0, dst_format->bytes_per_pixel * (dst_size->width - src_size->width));
        }

        if (src_size->height < dst_size->height) /* black out remaining pixels */
            memset(dst + src_size->height * dst_row_pitch, 0, dst_row_pitch * (dst_size->height - src_size->height));
    }
    if (src_size->depth < dst_size->depth) /* black out remaining pixels */
        memset(dst + src_size->depth * dst_slice_pitch, 0, dst_slice_pitch * (dst_size->depth - src_size->depth));
}

/************************************************************
 * point_filter_argb_pixels
 *
 * Copies the source buffer to the destination buffer, performing
 * any necessary format conversion, color keying and stretching
 * using a point filter.
 */
void point_filter_argb_pixels(const BYTE *src, UINT src_row_pitch, UINT src_slice_pitch, const struct volume *src_size,
                              const struct pixel_format_desc *src_format, BYTE *dst, UINT dst_row_pitch, UINT dst_slice_pitch,
                              const struct volume *dst_size, const struct pixel_format_desc *dst_format, D3DCOLOR color_key,
                              const PALETTEENTRY *palette)
{
    struct argb_conversion_info conv_info, ck_conv_info;
    const struct pixel_format_desc *ck_format = NULL;
    DWORD channels[4];
    UINT x, y, z;

//    TRACE("src %p, src_row_pitch %u, src_slice_pitch %u, src_size %p, src_format %p, dst %p, "
//          "dst_row_pitch %u, dst_slice_pitch %u, dst_size %p, dst_format %p, color_key 0x%08lx, palette %p.\n",
//          src, src_row_pitch, src_slice_pitch, src_size, src_format, dst, dst_row_pitch, dst_slice_pitch, dst_size,
//          dst_format, color_key, palette);

    ZeroMemory(channels, sizeof(channels));
    init_argb_conversion_info(src_format, dst_format, &conv_info);

    if (color_key)
    {
        /* Color keys are always represented in D3DFMT_A8R8G8B8 format. */
        ck_format = get_format_info(D3DFMT_A8R8G8B8);
        init_argb_conversion_info(src_format, ck_format, &ck_conv_info);
    }

    for (z = 0; z < dst_size->depth; z++)
    {
        BYTE *dst_slice_ptr = dst + z * dst_slice_pitch;
        const BYTE *src_slice_ptr = src + src_slice_pitch * (z * src_size->depth / dst_size->depth);

        for (y = 0; y < dst_size->height; y++)
        {
            BYTE *dst_ptr = dst_slice_ptr + y * dst_row_pitch;
            const BYTE *src_row_ptr = src_slice_ptr + src_row_pitch * (y * src_size->height / dst_size->height);

            for (x = 0; x < dst_size->width; x++)
            {
                const BYTE *src_ptr = src_row_ptr + (x * src_size->width / dst_size->width) * src_format->bytes_per_pixel;

                if (!src_format->to_rgba && !dst_format->from_rgba
                    && src_format->type == dst_format->type
                    && src_format->bytes_per_pixel <= 4 && dst_format->bytes_per_pixel <= 4)
                {
                    DWORD val;

                    get_relevant_argb_components(&conv_info, src_ptr, channels);
                    val = make_argb_color(&conv_info, channels);

                    if (color_key)
                    {
                        DWORD ck_pixel;

                        get_relevant_argb_components(&ck_conv_info, src_ptr, channels);
                        ck_pixel = make_argb_color(&ck_conv_info, channels);
                        if (ck_pixel == color_key)
                            val &= ~conv_info.destmask[0];
                    }
                    memcpy(dst_ptr, &val, dst_format->bytes_per_pixel);
                }
                else
                {
                    struct vec4 color, tmp;

                    format_to_vec4(src_format, src_ptr, &color);
                    if (src_format->to_rgba)
                        src_format->to_rgba(&color, &tmp, palette);
                    else
                        tmp = color;

                    if (ck_format)
                    {
                        DWORD ck_pixel;

                        format_from_vec4(ck_format, &tmp, (BYTE *)&ck_pixel);
                        if (ck_pixel == color_key)
                            tmp.w = 0.0f;
                    }

                    if (dst_format->from_rgba)
                        dst_format->from_rgba(&tmp, &color);
                    else
                        color = tmp;

                    format_from_vec4(dst_format, &color, dst_ptr);
                }

                dst_ptr += dst_format->bytes_per_pixel;
            }
        }
    }
}

/************************************************************
 * D3DXLoadSurfaceFromMemory
 *
 * Loads data from a given memory chunk into a surface,
 * applying any of the specified filters.
 *
 * PARAMS
 *   pDestSurface [I] pointer to the surface
 *   pDestPalette [I] palette to use
 *   pDestRect    [I] to be filled area of the surface
 *   pSrcMemory   [I] pointer to the source data
 *   SrcFormat    [I] format of the source pixel data
 *   SrcPitch     [I] number of bytes in a row
 *   pSrcPalette  [I] palette used in the source image
 *   pSrcRect     [I] area of the source data to load
 *   dwFilter     [I] filter to apply on stretching
 *   Colorkey     [I] colorkey
 *
 * RETURNS
 *   Success: D3D_OK, if we successfully load the pixel data into our surface or
 *                    if pSrcMemory is NULL but the other parameters are valid
 *   Failure: D3DERR_INVALIDCALL, if pDestSurface, SrcPitch or pSrcRect is NULL or
 *                                if SrcFormat is an invalid format (other than D3DFMT_UNKNOWN) or
 *                                if DestRect is invalid
 *            D3DXERR_INVALIDDATA, if we fail to lock pDestSurface
 *            E_FAIL, if SrcFormat is D3DFMT_UNKNOWN or the dimensions of pSrcRect are invalid
 *
 * NOTES
 *   pSrcRect specifies the dimensions of the source data;
 *   negative values for pSrcRect are allowed as we're only looking at the width and height anyway.
 *
 */
HRESULT D3DXLoadSurfaceFromMemory(IDirect3DSurface9 *dst_surface,
                                         const PALETTEENTRY *dst_palette, const RECT *dst_rect, const void *src_memory,
                                         D3DFORMAT src_format, UINT src_pitch, const PALETTEENTRY *src_palette, const RECT *src_rect,
                                         DWORD filter, D3DCOLOR color_key)
{
    const struct pixel_format_desc *srcformatdesc, *destformatdesc;
    struct volume src_size, dst_size, dst_size_aligned;
    RECT dst_rect_temp, dst_rect_aligned;
    IDirect3DSurface9 *surface;
    D3DSURFACE_DESC surfdesc;
    D3DLOCKED_RECT lockrect;
    HRESULT hr;

//    TRACE("dst_surface %p, dst_palette %p, dst_rect %s, src_memory %p, src_format %#x, "
//          "src_pitch %u, src_palette %p, src_rect %s, filter %#lx, color_key 0x%08lx.\n",
//          dst_surface, dst_palette, wine_dbgstr_rect(dst_rect), src_memory, src_format,
//          src_pitch, src_palette, wine_dbgstr_rect(src_rect), filter, color_key);

    if (!dst_surface || !src_memory || !src_rect)
    {
//        WARN("Invalid argument specified.\n");
        return D3DERR_INVALIDCALL;
    }
    if (src_format == D3DFMT_UNKNOWN
        || src_rect->left >= src_rect->right
        || src_rect->top >= src_rect->bottom)
    {
//        WARN("Invalid src_format or src_rect.\n");
        return E_FAIL;
    }

    srcformatdesc = get_format_info(src_format);
    if (srcformatdesc->type == FORMAT_UNKNOWN)
    {
//        FIXME("Unsupported format %#x.\n", src_format);
        return E_NOTIMPL;
    }

    src_size.width = src_rect->right - src_rect->left;
    src_size.height = src_rect->bottom - src_rect->top;
    src_size.depth = 1;

    IDirect3DSurface9_GetDesc(dst_surface, &surfdesc);
    destformatdesc = get_format_info(surfdesc.Format);
    if (!dst_rect)
    {
        dst_rect = &dst_rect_temp;
        dst_rect_temp.left = 0;
        dst_rect_temp.top = 0;
        dst_rect_temp.right = surfdesc.Width;
        dst_rect_temp.bottom = surfdesc.Height;
    }
    else
    {
        if (dst_rect->left > dst_rect->right || dst_rect->right > surfdesc.Width
            || dst_rect->top > dst_rect->bottom || dst_rect->bottom > surfdesc.Height
            || dst_rect->left < 0 || dst_rect->top < 0)
        {
//            WARN("Invalid dst_rect specified.\n");
            return D3DERR_INVALIDCALL;
        }
        if (dst_rect->left == dst_rect->right || dst_rect->top == dst_rect->bottom)
        {
//            WARN("Empty dst_rect specified.\n");
            return D3D_OK;
        }
    }

    dst_rect_aligned = *dst_rect;
    if (dst_rect_aligned.left & (destformatdesc->block_width - 1))
        dst_rect_aligned.left = dst_rect_aligned.left & ~(destformatdesc->block_width - 1);
    if (dst_rect_aligned.top & (destformatdesc->block_height - 1))
        dst_rect_aligned.top = dst_rect_aligned.top & ~(destformatdesc->block_height - 1);
    if (dst_rect_aligned.right & (destformatdesc->block_width - 1) && dst_rect_aligned.right != surfdesc.Width)
        dst_rect_aligned.right = std::min((dst_rect_aligned.right + destformatdesc->block_width - 1)
                                         & ~(destformatdesc->block_width - 1), surfdesc.Width);
    if (dst_rect_aligned.bottom & (destformatdesc->block_height - 1) && dst_rect_aligned.bottom != surfdesc.Height)
        dst_rect_aligned.bottom = std::min((dst_rect_aligned.bottom + destformatdesc->block_height - 1)
                                          & ~(destformatdesc->block_height - 1), surfdesc.Height);

    dst_size.width = dst_rect->right - dst_rect->left;
    dst_size.height = dst_rect->bottom - dst_rect->top;
    dst_size.depth = 1;
    dst_size_aligned.width = dst_rect_aligned.right - dst_rect_aligned.left;
    dst_size_aligned.height = dst_rect_aligned.bottom - dst_rect_aligned.top;
    dst_size_aligned.depth = 1;

    if (filter == D3DX_DEFAULT)
        filter = D3DX_FILTER_TRIANGLE | D3DX_FILTER_DITHER;

    if (FAILED(hr = lock_surface(dst_surface, &dst_rect_aligned, &lockrect, &surface, TRUE)))
        return hr;

    src_memory = (BYTE *)src_memory + src_rect->top / srcformatdesc->block_height * src_pitch
                 + src_rect->left / srcformatdesc->block_width * srcformatdesc->block_byte_count;

    if (src_format == surfdesc.Format
        && dst_size.width == src_size.width
        && dst_size.height == src_size.height
        && color_key == 0
        && !(src_rect->left & (srcformatdesc->block_width - 1))
        && !(src_rect->top & (srcformatdesc->block_height - 1))
        && !(dst_rect->left & (destformatdesc->block_width - 1))
        && !(dst_rect->top & (destformatdesc->block_height - 1)))
    {
//        TRACE("Simple copy.\n");
        copy_pixels(static_cast<const BYTE*>(src_memory), src_pitch, 0, static_cast<BYTE*>(lockrect.pBits), lockrect.Pitch, 0,
                    &src_size, srcformatdesc);
    }
    else /* Stretching or format conversion. */
    {
        const struct pixel_format_desc *dst_format;
        BYTE *dst_uncompressed = NULL;
        unsigned int dst_pitch;
        BYTE *dst_mem;

        if (!is_conversion_from_supported(srcformatdesc)
            || !is_conversion_to_supported(destformatdesc))
        {
//            FIXME("Unsupported format conversion %#x -> %#x.\n", src_format, surfdesc.Format);
            unlock_surface(dst_surface, &dst_rect_aligned, surface, FALSE);
            return E_NOTIMPL;
        }

        if (srcformatdesc->type == FORMAT_DXT || destformatdesc->type == FORMAT_DXT)
        {
            Msg("ERR Src/Dst FORMAT_DXT unsupported\n");
            return E_NOTIMPL;
        }
        else
        {
            dst_mem = static_cast<BYTE*>(lockrect.pBits);
            dst_pitch = lockrect.Pitch;
            dst_format = destformatdesc;
        }

        if ((filter & 0xf) == D3DX_FILTER_NONE)
        {
            convert_argb_pixels(static_cast<const BYTE*>(src_memory), src_pitch, 0, &src_size, srcformatdesc,
                                dst_mem, dst_pitch, 0, &dst_size, dst_format, color_key, src_palette);
        }
        else /* if ((filter & 0xf) == D3DX_FILTER_POINT) */
        {
//            if ((filter & 0xf) != D3DX_FILTER_POINT)
//                FIXME("Unhandled filter %#lx.\n", filter);

            /* Always apply a point filter until D3DX_FILTER_LINEAR,
             * D3DX_FILTER_TRIANGLE and D3DX_FILTER_BOX are implemented. */
            point_filter_argb_pixels(static_cast<const BYTE*>(src_memory), src_pitch, 0, &src_size, srcformatdesc,
                                     dst_mem, dst_pitch, 0, &dst_size, dst_format, color_key, src_palette);
        }
    }

    return unlock_surface(dst_surface, &dst_rect_aligned, surface, TRUE);
}

/************************************************************
 * D3DXLoadSurfaceFromSurface
 *
 * Copies the contents from one surface to another, performing any required
 * format conversion, resizing or filtering.
 *
 * PARAMS
 *   pDestSurface [I] pointer to the destination surface
 *   pDestPalette [I] palette to use
 *   pDestRect    [I] to be filled area of the surface
 *   pSrcSurface  [I] pointer to the source surface
 *   pSrcPalette  [I] palette used for the source surface
 *   pSrcRect     [I] area of the source data to load
 *   dwFilter     [I] filter to apply on resizing
 *   Colorkey     [I] any ARGB value or 0 to disable color-keying
 *
 * RETURNS
 *   Success: D3D_OK
 *   Failure: D3DERR_INVALIDCALL, if pDestSurface or pSrcSurface is NULL
 *            D3DXERR_INVALIDDATA, if one of the surfaces is not lockable
 *
 */
HRESULT D3DXLoadSurfaceFromSurface(IDirect3DSurface9 *dst_surface,
                                          const PALETTEENTRY *dst_palette, const RECT *dst_rect, IDirect3DSurface9 *src_surface,
                                          const PALETTEENTRY *src_palette, const RECT *src_rect, DWORD filter, D3DCOLOR color_key)
{
    const struct pixel_format_desc *src_format_desc, *dst_format_desc;
    D3DSURFACE_DESC src_desc, dst_desc;
    struct volume src_size, dst_size;
    IDirect3DSurface9 *temp_surface;
    D3DTEXTUREFILTERTYPE d3d_filter;
    IDirect3DDevice9 *device;
    D3DLOCKED_RECT lock;
    RECT dst_rect_temp;
    HRESULT hr;
    RECT s;

//    TRACE("dst_surface %p, dst_palette %p, dst_rect %s, src_surface %p, "
//          "src_palette %p, src_rect %s, filter %#lx, color_key 0x%08lx.\n",
//          dst_surface, dst_palette, wine_dbgstr_rect(dst_rect), src_surface,
//          src_palette, wine_dbgstr_rect(src_rect), filter, color_key);

    if (!dst_surface || !src_surface)
        return D3DERR_INVALIDCALL;

    IDirect3DSurface9_GetDesc(src_surface, &src_desc);
    src_format_desc = get_format_info(src_desc.Format);
    if (!src_rect)
    {
        SetRect(&s, 0, 0, src_desc.Width, src_desc.Height);
        src_rect = &s;
    }
    else if (src_rect->left == src_rect->right || src_rect->top == src_rect->bottom)
    {
//        WARN("Empty src_rect specified.\n");
        return filter == D3DX_FILTER_NONE ? D3D_OK : E_FAIL;
    }
    else if (src_rect->left > src_rect->right || src_rect->right > src_desc.Width
             || src_rect->left < 0 || src_rect->left > src_desc.Width
             || src_rect->top > src_rect->bottom || src_rect->bottom > src_desc.Height
             || src_rect->top < 0 || src_rect->top > src_desc.Height)
    {
//        WARN("Invalid src_rect specified.\n");
        return D3DERR_INVALIDCALL;
    }

    src_size.width = src_rect->right - src_rect->left;
    src_size.height = src_rect->bottom - src_rect->top;
    src_size.depth = 1;

    IDirect3DSurface9_GetDesc(dst_surface, &dst_desc);
    dst_format_desc = get_format_info(dst_desc.Format);
    if (!dst_rect)
    {
        SetRect(&dst_rect_temp, 0, 0, dst_desc.Width, dst_desc.Height);
        dst_rect = &dst_rect_temp;
    }
    else if (dst_rect->left == dst_rect->right || dst_rect->top == dst_rect->bottom)
    {
//        WARN("Empty dst_rect specified.\n");
        return filter == D3DX_FILTER_NONE ? D3D_OK : E_FAIL;
    }
    else if (dst_rect->left > dst_rect->right || dst_rect->right > dst_desc.Width
             || dst_rect->left < 0 || dst_rect->left > dst_desc.Width
             || dst_rect->top > dst_rect->bottom || dst_rect->bottom > dst_desc.Height
             || dst_rect->top < 0 || dst_rect->top > dst_desc.Height)
    {
//        WARN("Invalid dst_rect specified.\n");
        return D3DERR_INVALIDCALL;
    }

    dst_size.width = dst_rect->right - dst_rect->left;
    dst_size.height = dst_rect->bottom - dst_rect->top;
    dst_size.depth = 1;

    if (!dst_palette && !src_palette && !color_key)
    {
        if (src_desc.Format == dst_desc.Format
            && dst_size.width == src_size.width
            && dst_size.height == src_size.height
            && color_key == 0
            && !(src_rect->left & (src_format_desc->block_width - 1))
            && !(src_rect->top & (src_format_desc->block_height - 1))
            && !(dst_rect->left & (dst_format_desc->block_width - 1))
            && !(dst_rect->top & (dst_format_desc->block_height - 1)))
        {
            d3d_filter = D3DTEXF_NONE;
        }
        else
        {
            switch (filter)
            {
            case D3DX_FILTER_NONE:
                d3d_filter = D3DTEXF_NONE;
                break;

            case D3DX_FILTER_POINT:
                d3d_filter = D3DTEXF_POINT;
                break;

            case D3DX_FILTER_LINEAR:
                d3d_filter = D3DTEXF_LINEAR;
                break;

            default:
                d3d_filter = D3DTEXF_FORCE_DWORD;
                break;
            }
        }

        if (d3d_filter != D3DTEXF_FORCE_DWORD)
        {
            IDirect3DSurface9_GetDevice(src_surface, &device);
            hr = IDirect3DDevice9_StretchRect(device, src_surface, src_rect, dst_surface, dst_rect, d3d_filter);
            IDirect3DDevice9_Release(device);
            if (SUCCEEDED(hr))
                return D3D_OK;
        }
    }

    if (FAILED(lock_surface(src_surface, NULL, &lock, &temp_surface, FALSE)))
        return D3DERR_INVALIDCALL; // D3DXERR_INVALIDDATA;

    hr = D3DXLoadSurfaceFromMemory(dst_surface, dst_palette, dst_rect, lock.pBits,
                                   src_desc.Format, lock.Pitch, src_palette, src_rect, filter, color_key);

    if (FAILED(unlock_surface(src_surface, NULL, temp_surface, FALSE)))
        return D3DERR_INVALIDCALL; // D3DXERR_INVALIDDATA;

    return hr;
}
#endif

constexpr cpcstr NOT_EXISTING_TEXTURE = "ed" DELIMITER "ed_not_existing_texture";

void fix_texture_name(pstr fn)
{
    pstr _ext = strext(fn);
    if (_ext && (!xr_stricmp(_ext, ".tga") || !xr_stricmp(_ext, ".dds") ||
        !xr_stricmp(_ext, ".bmp") || !xr_stricmp(_ext, ".ogm")))
    {
        *_ext = 0;
    }
}

#ifndef _EDITOR
ENGINE_API bool is_enough_address_space_available();
#else
bool is_enough_address_space_available() { return true; }
#endif

int get_texture_load_lod(LPCSTR fn)
{
    CInifile::Sect& sect = pSettings->r_section("reduce_lod_texture_list");
    auto it_ = sect.Data.cbegin();
    auto it_e_ = sect.Data.cend();

    auto it = it_;
    auto it_e = it_e_;

    static bool enough_address_space_available = is_enough_address_space_available();

    for (; it != it_e; ++it)
    {
        if (strstr(fn, it->first.c_str()))
        {
            if (psTextureLOD < 1)
            {
                if (enough_address_space_available || GEnv.Render->GenerationIsR1())
                    return 0;
                else
                    return 1;
            }
            else if (psTextureLOD < 3)
                return 1;
            else
                return 2;
        }
    }

    if (psTextureLOD < 2)
    {
        //if (enough_address_space_available || GEnv.Render->GenerationIsR1())
        return 0;
        //else
        //    return 1;
    }
    else if (psTextureLOD < 4)
        return 1;
    else
        return 2;
}

u32 calc_texture_size(int lod, u32 mip_cnt, size_t orig_size)
{
    if (1 == mip_cnt)
        return orig_size;

    int _lod = lod;
    float res = float(orig_size);

    while (_lod > 0)
    {
        --_lod;
        res -= res / 1.333f;
    }
    return iFloor(res);
}

const float _BUMPHEIGH = 8.f;
//////////////////////////////////////////////////////////////////////
// Utility pack
//////////////////////////////////////////////////////////////////////
IC u32 GetPowerOf2Plus1(u32 v)
{
    u32 cnt = 0;
    while (v)
    {
        v >>= 1;
        cnt++;
    };
    return cnt;
}
IC void Reduce(int& w, int& h, int& l, int& skip)
{
    while ((l > 1) && skip)
    {
        w /= 2;
        h /= 2;
        l -= 1;

        skip--;
    }
    if (w < 1)
        w = 1;
    if (h < 1)
        h = 1;
}

void TW_Save(ID3DTexture2D* T, LPCSTR name, LPCSTR prefix, LPCSTR postfix)
{
    string256 fn;
    strconcat(sizeof(fn), fn, name, "_", prefix, "-", postfix);
    for (int it = 0; it < int(xr_strlen(fn)); it++)
        if (_DELIMITER == fn[it])
            fn[it] = '_';
    string256 fn2;
    strconcat(sizeof(fn2), fn2, "debug" DELIMITER, fn, ".dds");
    Log("* debug texture save: ", fn2);
#if defined(XR_PLATFORM_WINDOWS) // FIX_LINUX textures
    R_CHK(D3DXSaveTextureToFile(fn2, D3DXIFF_DDS, T, nullptr));
#else
    Msg("q4a D3DXSaveTextureToFile");
#endif
}

ID3DTexture2D* TW_LoadTextureFromTexture(
    ID3DTexture2D* t_from, D3DFORMAT& t_dest_fmt, int levels_2_skip, u32& w, u32& h)
{
    // Calculate levels & dimensions
    ID3DTexture2D* t_dest = nullptr;
    D3DSURFACE_DESC t_from_desc0;
    R_CHK(t_from->GetLevelDesc(0, &t_from_desc0));
    int levels_exist = t_from->GetLevelCount();
    int top_width = t_from_desc0.Width;
    int top_height = t_from_desc0.Height;
    Reduce(top_width, top_height, levels_exist, levels_2_skip);

    // Create HW-surface
#if defined(XR_PLATFORM_WINDOWS)
    if (D3DX_DEFAULT == t_dest_fmt)
        t_dest_fmt = t_from_desc0.Format;
    R_CHK(D3DXCreateTexture(HW.pDevice, top_width, top_height, levels_exist, 0, t_dest_fmt,
        (RImplementation.o.no_ram_textures ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED),
    &t_dest));
#else
    R_ASSERT(t_dest_fmt == t_from_desc0.Format);
    R_CHK(HW.pDevice->CreateTexture(top_width, top_height, levels_exist, 0, t_dest_fmt,
        (RImplementation.o.no_ram_textures ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED),
        &t_dest, nullptr));
#endif

    // Copy surfaces & destroy temporary
    ID3DTexture2D* T_src = t_from;
    ID3DTexture2D* T_dst = t_dest;

    int L_src = T_src->GetLevelCount() - 1;
    int L_dst = T_dst->GetLevelCount() - 1;
    for (; L_dst >= 0; L_src--, L_dst--)
    {
        // Get surfaces
        IDirect3DSurface9 *S_src, *S_dst;
        R_CHK(T_src->GetSurfaceLevel(L_src, &S_src));
        R_CHK(T_dst->GetSurfaceLevel(L_dst, &S_dst));

        // Copy
        R_CHK(D3DXLoadSurfaceFromSurface(S_dst, NULL, NULL, S_src, NULL, NULL, D3DX_FILTER_NONE, 0));

        // Release surfaces
        _RELEASE(S_src);
        _RELEASE(S_dst);
    }

    // OK
    w = top_width;
    h = top_height;
    return t_dest;
}

template <class _It>
void TW_Iterate_1OP(ID3DTexture2D* t_dst, ID3DTexture2D* t_src, const _It pred)
{
    u32 mips = t_dst->GetLevelCount();
    R_ASSERT(mips == t_src->GetLevelCount());
    for (u32 i = 0; i < mips; i++)
    {
        D3DLOCKED_RECT Rsrc, Rdst;
        D3DSURFACE_DESC desc, descS;

        t_dst->GetLevelDesc(i, &desc);
        t_src->GetLevelDesc(i, &descS);
        VERIFY(desc.Format == descS.Format);
        VERIFY(desc.Format == D3DFMT_A8R8G8B8);
        t_src->LockRect(i, &Rsrc, nullptr, 0);
        t_dst->LockRect(i, &Rdst, nullptr, 0);
        for (u32 y = 0; y < desc.Height; y++)
        {
            for (u32 x = 0; x < desc.Width; x++)
            {
                u32& pSrc = *(((u32*)((u8*)Rsrc.pBits + (y * Rsrc.Pitch))) + x);
                u32& pDst = *(((u32*)((u8*)Rdst.pBits + (y * Rdst.Pitch))) + x);
                pDst = pred(pDst, pSrc);
            }
        }
        t_dst->UnlockRect(i);
        t_src->UnlockRect(i);
    }
}
template <class _It>
void TW_Iterate_2OP(ID3DTexture2D* t_dst, ID3DTexture2D* t_src0, ID3DTexture2D* t_src1, const _It pred)
{
    u32 mips = t_dst->GetLevelCount();
    R_ASSERT(mips == t_src0->GetLevelCount());
    R_ASSERT(mips == t_src1->GetLevelCount());
    for (u32 i = 0; i < mips; i++)
    {
        D3DLOCKED_RECT Rsrc0, Rsrc1, Rdst;
        D3DSURFACE_DESC desc, descS0, descS1;

        t_dst->GetLevelDesc(i, &desc);
        t_src0->GetLevelDesc(i, &descS0);
        t_src1->GetLevelDesc(i, &descS1);
        VERIFY(desc.Format == descS0.Format);
        VERIFY(desc.Format == descS1.Format);
        VERIFY(desc.Format == D3DFMT_A8R8G8B8);
        t_src0->LockRect(i, &Rsrc0, nullptr, 0);
        t_src1->LockRect(i, &Rsrc1, nullptr, 0);
        t_dst->LockRect(i, &Rdst, nullptr, 0);
        for (u32 y = 0; y < desc.Height; y++)
        {
            for (u32 x = 0; x < desc.Width; x++)
            {
                u32& pSrc0 = *(((u32*)((u8*)Rsrc0.pBits + (y * Rsrc0.Pitch))) + x);
                u32& pSrc1 = *(((u32*)((u8*)Rsrc1.pBits + (y * Rsrc1.Pitch))) + x);
                u32& pDst = *(((u32*)((u8*)Rdst.pBits + (y * Rdst.Pitch))) + x);
                pDst = pred(pDst, pSrc0, pSrc1);
            }
        }
        t_dst->UnlockRect(i);
        t_src0->UnlockRect(i);
        t_src1->UnlockRect(i);
    }
}

IC u32 it_gloss_rev(u32 d, u32 s)
{
    return color_rgba(color_get_A(s), // gloss
        color_get_B(d), color_get_G(d), color_get_R(d));
}
IC u32 it_gloss_rev_base(u32 d, u32 /*s*/)
{
    u32 occ = color_get_A(d) / 3;
    u32 def = 8;
    u32 gloss = (occ * 1 + def * 3) / 4;
    return color_rgba(gloss, // gloss
        color_get_B(d), color_get_G(d), color_get_R(d));
}
IC u32 it_difference(u32 /*d*/, u32 orig, u32 ucomp)
{
    return color_rgba(128 + (int(color_get_R(orig)) - int(color_get_R(ucomp))) * 2, // R-error
        128 + (int(color_get_G(orig)) - int(color_get_G(ucomp))) * 2, // G-error
        128 + (int(color_get_B(orig)) - int(color_get_B(ucomp))) * 2, // B-error
        128 + (int(color_get_A(orig)) - int(color_get_A(ucomp))) * 2); // A-error
}
IC u32 it_height_rev(u32 d, u32 s)
{
    return color_rgba(color_get_A(d), // diff x
        color_get_B(d), // diff y
        color_get_G(d), // diff z
        color_get_R(s)); // height
}
IC u32 it_height_rev_base(u32 d, u32 s)
{
    return color_rgba(color_get_A(d), // diff x
        color_get_B(d), // diff y
        color_get_G(d), // diff z
        (color_get_R(s) + color_get_G(s) + color_get_B(s)) / 3); // height
}

ID3DBaseTexture* CRender::texture_load(LPCSTR fRName, u32& ret_msize)
{
    HRESULT result;
    ID3DTexture2D* pTexture2D = nullptr;
    IDirect3DCubeTexture9* pTextureCUBE = nullptr;
#if !defined(XR_PLATFORM_WINDOWS)
    gli::texture texture;
    gli::texture_cube texCube;
    gli::storage_linear::extent_type dimensions;
    D3DLOCKED_RECT lockRect;
    gli::dx DX;
#endif
    string_path fn;
    u32 dwWidth, dwHeight;
    size_t img_size = 0;
    int img_loaded_lod = 0;
    D3DFORMAT fmt;
    u32 mip_cnt = u32(-1);
    bool dummyTextureExist;

    // validation
    R_ASSERT(fRName);
    R_ASSERT(fRName[0]);

    // make file name
    string_path fname;
    xr_strcpy(fname, fRName); //. andy if (strext(fname)) *strext(fname)=0;
    fix_texture_name(fname);
    IReader* S = nullptr;
    // if (FS.exist(fn,"$game_textures$",fname, ".dds") && strstr(fname,"_bump")) goto _BUMP;
    if (!FS.exist(fn, "$game_textures$", fname, ".dds") && strstr(fname, "_bump"))
        goto _BUMP_from_base;
    if (FS.exist(fn, "$level$", fname, ".dds"))
        goto _DDS;
    if (FS.exist(fn, "$game_saves$", fname, ".dds"))
        goto _DDS;
    if (FS.exist(fn, "$game_textures$", fname, ".dds"))
        goto _DDS;

#ifdef _EDITOR
    ELog.Msg(mtError, "Can't find texture '%s'", fname);
    return 0;
#else

    Msg("! Can't find texture '%s'", fname);
    dummyTextureExist = FS.exist(fn, "$game_textures$", NOT_EXISTING_TEXTURE, ".dds");
    if (!ShadowOfChernobylMode)
        R_ASSERT3(dummyTextureExist, "Dummy texture doesn't exist", NOT_EXISTING_TEXTURE);
    if (!dummyTextureExist)
        return nullptr;
    goto _DDS;

//xrDebug::Fatal(DEBUG_INFO,"Can't find texture '%s'",fname);

#endif

_DDS:
{
    // Load and get header
    S = FS.r_open(fn);
//#ifdef DEBUG
    Msg("* Loaded: %s[%d]", fn, S->length());
//#endif // DEBUG
    img_size = S->length();
    R_ASSERT(S);
#if defined(XR_PLATFORM_WINDOWS)
    D3DXIMAGE_INFO IMG;
    result = D3DXGetImageInfoFromFileInMemory(S->pointer(), S->length(), &IMG);
    if (FAILED(result))
    {
        Msg("! Can't get image info for texture '%s'", fn);
        FS.r_close(S);
        string_path temp;
        R_ASSERT(FS.exist(temp, "$game_textures$", NOT_EXISTING_TEXTURE, ".dds"));
        R_ASSERT(xr_strcmp(temp, fn));
        xr_strcpy(fn, temp);
        goto _DDS;
    }

    if (IMG.ResourceType == D3DRTYPE_CUBETEXTURE)
        goto _DDS_CUBE;
    else
        goto _DDS_2D;
#else
    texture = gli::load((char*)S->pointer(), img_size);
    if(texture.empty())
    {
        Msg("q4a can't load: %s", fn);
        R_ASSERT2(!texture.empty(), fn);
    }

    switch (texture.target())
    {
    case gli::TARGET_2D:
        goto _DDS_2D;
        break;
    /*case gli::TARGET_3D:
    case gli::TARGET_CUBE_ARRAY:*/
    case gli::TARGET_CUBE:
        goto _DDS_CUBE;
        break;
    default:
        Msg("q4a Can't detect texture.target()");
        NODEFAULT;
        break;
    }
#endif

_DDS_CUBE:
{
#if defined(XR_PLATFORM_WINDOWS)
    result = D3DXCreateCubeTextureFromFileInMemoryEx(HW.pDevice, S->pointer(), S->length(), D3DX_DEFAULT,
        IMG.MipLevels, 0, IMG.Format,
       (RImplementation.o.no_ram_textures ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED),
        D3DX_DEFAULT, D3DX_DEFAULT, 0, &IMG, nullptr, &pTextureCUBE);
#else
    texCube = gli::texture_cube(texture);
    dwWidth = texture.extent().x;
    dwHeight = dwWidth;
    fmt = static_cast<D3DFORMAT>(DX.translate(texCube.format()).D3DFormat);
    Msg("!@! '%s'-'5'-'%d'-'%d'-'%d'", fn, fmt, texCube.max_level(), texCube.levels());
    result = HW.pDevice->CreateCubeTexture(dwWidth, texCube.levels(), 0, fmt,
        (RImplementation.o.no_ram_textures ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED), &pTextureCUBE, nullptr);
    if (!FAILED(result))
    {
        D3DLOCKED_RECT rect;
        char* dest;
        ulong maxface = texCube.max_face();
        for (ulong i = 0; i <= maxface; ++i)
        {
            result = pTextureCUBE->LockRect((D3DCUBEMAP_FACES)i, 0, &rect, 0, D3DLOCK_DISCARD);
            if (FAILED(result))
                break;
            dest = static_cast<char*>(rect.pBits);
            memcpy(dest, texCube[i].data(), texCube[i].size());
            result = pTextureCUBE->UnlockRect((D3DCUBEMAP_FACES)i, 0);
            if (FAILED(result))
                break;
        }
    }
#endif
    FS.r_close(S);

    if (FAILED(result))
    {
        Msg("! Can't load texture '%s'", fn);
        string_path temp;
        R_ASSERT(FS.exist(temp, "$game_textures$", NOT_EXISTING_TEXTURE, ".dds"));
        R_ASSERT(xr_strcmp(temp, fn));
        xr_strcpy(fn, temp);
        goto _DDS;
    }

    // OK
#if defined(XR_PLATFORM_WINDOWS)
    dwWidth = IMG.Width;
    dwHeight = IMG.Height;
    fmt = IMG.Format;
#endif
    ret_msize = calc_texture_size(img_loaded_lod, mip_cnt, img_size);
    mip_cnt = pTextureCUBE->GetLevelCount();
    return pTextureCUBE;
}
_DDS_2D:
{
    xr_strlwr(fn);
    // Load   SYS-MEM-surface, bound to device restrictions
    ID3DTexture2D* T_sysmem;
#if defined(XR_PLATFORM_WINDOWS)
    fmt = IMG.Format;
    HRESULT const result =
        D3DXCreateTextureFromFileInMemoryEx(HW.pDevice, S->pointer(), S->length(), D3DX_DEFAULT, D3DX_DEFAULT,
            IMG.MipLevels, 0, IMG.Format, D3DPOOL_SYSTEMMEM, D3DX_DEFAULT, D3DX_DEFAULT, 0, &IMG, nullptr, &T_sysmem);
#else
    dimensions = texture.extent();
    fmt = static_cast<D3DFORMAT>(DX.translate(texture.format()).D3DFormat);
    Msg("!@! '%s'-'3'-'%d'-'%d'-'%d'", fn, fmt, texture.max_level(), texture.levels());
    result = HW.pDevice->CreateTexture(dimensions.x, dimensions.y, texture.levels(), 0, fmt,
            D3DPOOL_SYSTEMMEM, &T_sysmem, nullptr);
    if (!FAILED(result))
    {
        result = T_sysmem->LockRect(0, &lockRect, 0, D3DLOCK_DISCARD);
    }
    if (!FAILED(result))
    {
        char* dest = static_cast<char*>(lockRect.pBits);
        memcpy(dest, texture.data(), texture.size());
        result = T_sysmem->UnlockRect(0);
    }
#endif
    FS.r_close(S);

    if (FAILED(result))
    {
        Msg("! Can't load texture '%s'", fn);
        string_path temp;
        R_ASSERT(FS.exist(temp, "$game_textures$", NOT_EXISTING_TEXTURE, ".dds"));
        xr_strlwr(temp);
        R_ASSERT(xr_strcmp(temp, fn));
        xr_strcpy(fn, temp);
        goto _DDS;
    }

    img_loaded_lod = get_texture_load_lod(fn);
    pTexture2D = TW_LoadTextureFromTexture(T_sysmem, fmt, img_loaded_lod, dwWidth, dwHeight);
    mip_cnt = pTexture2D->GetLevelCount();
    _RELEASE(T_sysmem);

    // OK
    ret_msize = calc_texture_size(img_loaded_lod, mip_cnt, img_size);
    return pTexture2D;
}
}
/*
_BUMP:
{
    // Load SYS-MEM-surface, bound to device restrictions
    D3DXIMAGE_INFO IMG;
    IReader* S = FS.r_open (fn);
    msize = S->length ();
    ID3DTexture2D* T_height_gloss;
    R_CHK(D3DXCreateTextureFromFileInMemoryEx(
        HW.pDevice, S->pointer(), S->length(),
        D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_A8R8G8B8,
        D3DPOOL_SYSTEMMEM, D3DX_DEFAULT,D3DX_DEFAULT,
        0, &IMG, 0, &T_height_gloss));
    FS.r_close(S);
    //TW_Save(T_height_gloss, fname, "debug-0", "original");

    // Create HW-surface, compute normal map
    ID3DTexture2D* T_normal_1 = 0;
    R_CHK(D3DXCreateTexture
        (HW.pDevice, IMG.Width, IMG.Height, D3DX_DEFAULT, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &T_normal_1));
    R_CHK(D3DXComputeNormalMap(T_normal_1, T_height_gloss, 0, 0, D3DX_CHANNEL_RED,_BUMPHEIGH));
    //TW_Save(T_normal_1, fname, "debug-1", "normal");

    // Transfer gloss-map
    TW_Iterate_1OP(T_normal_1, T_height_gloss, it_gloss_rev);
    //TW_Save(T_normal_1, fname, "debug-2", "normal-G");

    // Compress
    fmt = D3DFMT_DXT5;
    ID3DTexture2D* T_normal_1C = TW_LoadTextureFromTexture(T_normal_1, fmt, psTextureLOD, dwWidth, dwHeight);
    //TW_Save(T_normal_1C, fname, "debug-3", "normal-G-C");

#if RENDER==R_R2
    // Decompress (back)
    fmt = D3DFMT_A8R8G8B8;
    ID3DTexture2D* T_normal_1U = TW_LoadTextureFromTexture(T_normal_1C, fmt, 0, dwWidth, dwHeight);
    // TW_Save(T_normal_1U, fname, "debug-4", "normal-G-CU");

    // Calculate difference
    ID3DTexture2D*  T_normal_1D = 0;
    R_CHK(D3DXCreateTexture(HW.pDevice, dwWidth, dwHeight, T_normal_1U->GetLevelCount(), 0, D3DFMT_A8R8G8B8,
        D3DPOOL_SYSTEMMEM, &T_normal_1D));
    TW_Iterate_2OP(T_normal_1D, T_normal_1, T_normal_1U, it_difference);
    //TW_Save(T_normal_1D, fname, "debug-5", "normal-G-diff");

    // Reverse channels back + transfer heightmap
    TW_Iterate_1OP(T_normal_1D, T_height_gloss, it_height_rev);
    //TW_Save(T_normal_1D, fname, "debug-6", "normal-G-diff-H");

    // Compress
    fmt = D3DFMT_DXT5;
    ID3DTexture2D* T_normal_2C = TW_LoadTextureFromTexture(T_normal_1D, fmt, 0, dwWidth, dwHeight);
    //TW_Save(T_normal_2C, fname, "debug-7", "normal-G-diff-H-C");
    _RELEASE(T_normal_1U);
    _RELEASE(T_normal_1D);

    //
    string256 fnameB;
    strconcat(fnameB, "$user$", fname, "X");
    ref_texture t_temp = dxRenderDeviceRender::Instance().Resources->_CreateTexture(fnameB);
    t_temp->surface_set(T_normal_2C);
    RELEASE(T_normal_2C); // texture should keep reference to it by itself
#endif

    // release and return
    // T_normal_1C - normal.gloss, reversed
    // T_normal_2C - 2*error.height, non-reversed
    _RELEASE(T_height_gloss);
    _RELEASE(T_normal_1);
    return T_normal_1C;
}
*/
_BUMP_from_base:
{
    Msg("! auto-generated bump map: %s", fname);
//////////////////
#ifndef _EDITOR
    if (strstr(fname, "_bump#"))
    {
        R_ASSERT2(FS.exist(fn, "$game_textures$", "ed" DELIMITER "ed_dummy_bump#", ".dds"), "ed_dummy_bump#");
        S = FS.r_open(fn);
        R_ASSERT2(S, fn);
        img_size = S->length();
        goto _DDS_2D;
    }
    if (strstr(fname, "_bump"))
    {
        R_ASSERT2(FS.exist(fn, "$game_textures$", "ed" DELIMITER "ed_dummy_bump", ".dds"), "ed_dummy_bump");
        S = FS.r_open(fn);

        R_ASSERT2(S, fn);

        img_size = S->length();
        goto _DDS_2D;
    }
#endif
    //////////////////

    *strstr(fname, "_bump") = 0;
    R_ASSERT2(FS.exist(fn, "$game_textures$", fname, ".dds"), fname);

    // Load   SYS-MEM-surface, bound to device restrictions
#if defined(XR_PLATFORM_WINDOWS) // FIX_LINUX textures
    D3DXIMAGE_INFO IMG;
#else
    Msg("q4a _BUMP_from_base");
#endif
    S = FS.r_open(fn);
    img_size = S->length();
    ID3DTexture2D* T_base;
#if defined(XR_PLATFORM_WINDOWS) // FIX_LINUX textures
    R_CHK2(D3DXCreateTextureFromFileInMemoryEx(HW.pDevice, S->pointer(), S->length(), D3DX_DEFAULT, D3DX_DEFAULT,
        D3DX_DEFAULT, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, D3DX_DEFAULT, D3DX_DEFAULT, 0, &IMG, nullptr, &T_base), fn);
#endif
    FS.r_close(S);

    // Create HW-surface
    ID3DTexture2D* T_normal_1 = nullptr;
#if defined(XR_PLATFORM_WINDOWS) // FIX_LINUX textures
    R_CHK(D3DXCreateTexture(
        HW.pDevice, IMG.Width, IMG.Height, D3DX_DEFAULT, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &T_normal_1));
    R_CHK(D3DXComputeNormalMap(
        T_normal_1, T_base, nullptr, D3DX_NORMALMAP_COMPUTE_OCCLUSION, D3DX_CHANNEL_LUMINANCE, _BUMPHEIGH));

    // Transfer gloss-map
    TW_Iterate_1OP(T_normal_1, T_base, it_gloss_rev_base);
#endif

    // Compress
    fmt = D3DFMT_DXT5;
    img_loaded_lod = get_texture_load_lod(fn);
    ID3DTexture2D* T_normal_1C = TW_LoadTextureFromTexture(T_normal_1, fmt, img_loaded_lod, dwWidth, dwHeight);
    mip_cnt = T_normal_1C->GetLevelCount();

#if RENDER == R_R2
    // Decompress (back)
    fmt = D3DFMT_A8R8G8B8;
    ID3DTexture2D* T_normal_1U = TW_LoadTextureFromTexture(T_normal_1C, fmt, 0, dwWidth, dwHeight);

    // Calculate difference
    ID3DTexture2D* T_normal_1D = 0;
    R_CHK(D3DXCreateTexture(HW.pDevice, dwWidth, dwHeight, T_normal_1U->GetLevelCount(), 0, D3DFMT_A8R8G8B8,
        D3DPOOL_SYSTEMMEM, &T_normal_1D));
    TW_Iterate_2OP(T_normal_1D, T_normal_1, T_normal_1U, it_difference);

    // Reverse channels back + transfer heightmap
    TW_Iterate_1OP(T_normal_1D, T_base, it_height_rev_base);

    // Compress
    fmt = D3DFMT_DXT5;
    ID3DTexture2D* T_normal_2C = TW_LoadTextureFromTexture(T_normal_1D, fmt, 0, dwWidth, dwHeight);
    _RELEASE(T_normal_1U);
    _RELEASE(T_normal_1D);

    //
    string256 fnameB;
    strconcat(sizeof(fnameB), fnameB, "$user$", fname, "_bumpX");
    ref_texture t_temp = Resources->_CreateTexture(fnameB);
    t_temp->surface_set(T_normal_2C);
    _RELEASE(T_normal_2C); // texture should keep reference to it by itself
#endif
    // T_normal_1C - normal.gloss, reversed
    // T_normal_2C - 2*error.height, non-reversed
    _RELEASE(T_base);
    _RELEASE(T_normal_1);
    ret_msize = calc_texture_size(img_loaded_lod, mip_cnt, img_size);
    return T_normal_1C;
}
}
