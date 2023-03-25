#include "stdafx.h"

enum
{
    LOCKFLAGS_FLUSH  = D3DLOCK_DISCARD,
    LOCKFLAGS_APPEND = D3DLOCK_NOOVERWRITE,
};

#if !defined(XR_PLATFORM_WINDOWS) // USE_LINUX D3DXGetFVFVertexSize

// wine/dlls/d3dx9_36/mesh.c

/*************************************************************************
 * D3DXGetFVFVertexSize
 */
static UINT Get_TexCoord_Size_From_FVF(DWORD FVF, int tex_num)
{
    return (((((FVF) >> (16 + (2 * (tex_num)))) + 1) & 0x03) + 1);
}

UINT D3DXGetFVFVertexSize(DWORD FVF)
{
    DWORD size = 0;
    UINT i;
    UINT numTextures = (FVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;

    if (FVF & D3DFVF_NORMAL) size += sizeof(D3DVECTOR);//D3DXVECTOR3
    if (FVF & D3DFVF_DIFFUSE) size += sizeof(DWORD);
    if (FVF & D3DFVF_SPECULAR) size += sizeof(DWORD);
    if (FVF & D3DFVF_PSIZE) size += sizeof(DWORD);

    switch (FVF & D3DFVF_POSITION_MASK)
    {
    case D3DFVF_XYZ:    size += sizeof(D3DVECTOR); break;//D3DXVECTOR3
    case D3DFVF_XYZRHW: size += 4 * sizeof(FLOAT); break;
    case D3DFVF_XYZB1:  size += 4 * sizeof(FLOAT); break;
    case D3DFVF_XYZB2:  size += 5 * sizeof(FLOAT); break;
    case D3DFVF_XYZB3:  size += 6 * sizeof(FLOAT); break;
    case D3DFVF_XYZB4:  size += 7 * sizeof(FLOAT); break;
    case D3DFVF_XYZB5:  size += 8 * sizeof(FLOAT); break;
    case D3DFVF_XYZW:   size += 4 * sizeof(FLOAT); break;
    }

    for (i = 0; i < numTextures; i++)
    {
        size += Get_TexCoord_Size_From_FVF(FVF, i) * sizeof(FLOAT);
    }

    return size;
}

/*************************************************************************
 * D3DXGetDeclVertexSize
 */
UINT D3DXGetDeclVertexSize(const D3DVERTEXELEMENT9 *decl, DWORD stream_idx)
{
    const D3DVERTEXELEMENT9 *element;
    UINT size = 0;

//    TRACE("decl %p, stream_idx %lu.\n", decl, stream_idx);

    if (!decl) return 0;

    for (element = decl; element->Stream != 0xff; ++element)
    {
        UINT type_size;

        if (element->Stream != stream_idx) continue;

        if (element->Type >= SDL_arraysize(d3dx_decltype_size))
        {
            Msg("Unhandled element type %#x, size will be incorrect.\n", element->Type);
            continue;
        }

        type_size = d3dx_decltype_size[element->Type];
        if (element->Offset + type_size > size) size = element->Offset + type_size;
    }

    return size;
}

/*************************************************************************
 * D3DXGetDeclLength
 */
UINT D3DXGetDeclLength(const D3DVERTEXELEMENT9 *decl)
{
    const D3DVERTEXELEMENT9 *element;

//    TRACE("decl %p\n", decl);

    /* null decl results in exception on Windows XP */

    for (element = decl; element->Stream != 0xff; ++element);

    return element - decl;
}
#endif

u32 GetFVFVertexSize(u32 FVF)
{
    return D3DXGetFVFVertexSize(FVF);
}

u32 GetDeclVertexSize(const VertexElement* decl, u32 Stream)
{
    return D3DXGetDeclVertexSize(decl, Stream);
}

u32 GetDeclLength(const VertexElement* decl)
{
    return D3DXGetDeclLength(decl);
}

//-----------------------------------------------------------------------------
VertexStagingBuffer::~VertexStagingBuffer()
{
    Destroy();
}

void VertexStagingBuffer::Create(size_t size, bool allowReadBack /*= false*/)
{
    m_Size = size;
    m_AllowReadBack = allowReadBack;

    u32 dwUsage = allowReadBack ? 0 : D3DUSAGE_WRITEONLY;
    if (HW.Caps.geometry.bSoftware)
        dwUsage |= D3DUSAGE_SOFTWAREPROCESSING;
    R_CHK(HW.pDevice->CreateVertexBuffer(size, dwUsage, 0, D3DPOOL_MANAGED, &m_DeviceBuffer, nullptr));
    VERIFY(m_DeviceBuffer);

    HW.m_stats_manager.increment_stats_vb(m_DeviceBuffer);
    AddRef();
}

bool VertexStagingBuffer::IsValid() const
{
    return !!m_DeviceBuffer;
}

void* VertexStagingBuffer::Map(
    size_t offset /*= 0 */,
    size_t size /*= 0 */,
    bool read /*= false*/)
{
    VERIFY(IsValid());
    VERIFY2(!read || m_AllowReadBack, "Can't read from write only buffer");
    VERIFY(size <= m_Size);

    u32 mapMode = read ? D3DLOCK_READONLY : 0;
    R_CHK(m_DeviceBuffer->Lock(offset, size, const_cast<void**>(&m_HostBuffer), mapMode));
    return m_HostBuffer;
}

void VertexStagingBuffer::Unmap(bool /*doFlush = false*/)
{
    VERIFY(IsValid());
    R_CHK(m_DeviceBuffer->Unlock());
}

void VertexStagingBuffer::DiscardHostBuffer()
{
    /* Do nothing */
}

VertexBufferHandle VertexStagingBuffer::GetBufferHandle() const
{
    return m_DeviceBuffer;
}

void VertexStagingBuffer::Destroy()
{
    HW.m_stats_manager.decrement_stats_vb(m_DeviceBuffer);
    _RELEASE(m_DeviceBuffer);
    m_DeviceBuffer = nullptr;
}

size_t VertexStagingBuffer::GetSystemMemoryUsage() const
{
    if (IsValid())
    {
        D3DVERTEXBUFFER_DESC desc;
        m_DeviceBuffer->GetDesc(&desc);

        if (desc.Pool == D3DPOOL_MANAGED || desc.Pool == D3DPOOL_SCRATCH)
            return desc.Size;
    }

    return 0;
}

size_t VertexStagingBuffer::GetVideoMemoryUsage() const
{
    if (IsValid())
    {
        D3DVERTEXBUFFER_DESC desc;
        m_DeviceBuffer->GetDesc(&desc);

        if (desc.Pool == D3DPOOL_DEFAULT || desc.Pool == D3DPOOL_MANAGED)
            return desc.Size;
    }

    return 0;
}

//-----------------------------------------------------------------------------
IndexStagingBuffer::~IndexStagingBuffer()
{
    Destroy();
}

void IndexStagingBuffer::Create(size_t size, bool allowReadBack /*= false*/, bool managed /*= true*/)
{
    m_Size = size;
    m_AllowReadBack = allowReadBack;

    u32 dwUsage = m_AllowReadBack ? 0 : D3DUSAGE_WRITEONLY;
    if (HW.Caps.geometry.bSoftware)
        dwUsage |= D3DUSAGE_SOFTWAREPROCESSING;
    R_CHK(HW.pDevice->CreateIndexBuffer(size, dwUsage, D3DFMT_INDEX16, managed ? D3DPOOL_MANAGED : D3DPOOL_DEFAULT, &m_DeviceBuffer, NULL));

    HW.m_stats_manager.increment_stats_ib(m_DeviceBuffer);
    AddRef();
}

bool IndexStagingBuffer::IsValid() const
{
    return !!m_DeviceBuffer;
}

void* IndexStagingBuffer::Map(
    size_t offset /*= 0 */,
    size_t size /*= 0 */,
    bool read /*= false*/)
{
    VERIFY(IsValid());
    VERIFY2(!read || m_AllowReadBack, "Can't read from write only buffer");
    VERIFY(size <= m_Size);

    u32 mapMode = read ? D3DLOCK_READONLY : 0;
    R_CHK(m_DeviceBuffer->Lock(offset, size, const_cast<void**>(&m_HostBuffer), mapMode));
    return m_HostBuffer;
}

void IndexStagingBuffer::Unmap(bool /*doFlush = false*/)
{
    VERIFY(IsValid());
    R_CHK(m_DeviceBuffer->Unlock());
}

void IndexStagingBuffer::DiscardHostBuffer()
{
    /* Do nothing */
}

IndexBufferHandle IndexStagingBuffer::GetBufferHandle() const
{
    return m_DeviceBuffer;
}

void IndexStagingBuffer::Destroy()
{
    HW.m_stats_manager.decrement_stats_ib(m_DeviceBuffer);
    _RELEASE(m_DeviceBuffer);
    m_DeviceBuffer = nullptr;
}

size_t IndexStagingBuffer::GetSystemMemoryUsage() const
{
    if (IsValid())
    {
        D3DINDEXBUFFER_DESC desc;
        m_DeviceBuffer->GetDesc(&desc);

        if (desc.Pool == D3DPOOL_MANAGED || desc.Pool == D3DPOOL_SCRATCH)
            return desc.Size;
    }

    return 0;
}

size_t IndexStagingBuffer::GetVideoMemoryUsage() const
{
    if (IsValid())
    {
        D3DINDEXBUFFER_DESC desc;
        m_DeviceBuffer->GetDesc(&desc);

        if (desc.Pool == D3DPOOL_DEFAULT || desc.Pool == D3DPOOL_MANAGED)
            return desc.Size;
    }

    return 0;
}

//-----------------------------------------------------------------------------
VertexStreamBuffer::~VertexStreamBuffer()
{
    Destroy();
}

void VertexStreamBuffer::Create(size_t size)
{
    R_CHK(HW.pDevice->CreateVertexBuffer(
        size,
        D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
        0,
        D3DPOOL_DEFAULT,
        &m_DeviceBuffer,
        NULL));
    VERIFY(m_DeviceBuffer);
    AddRef();
    HW.m_stats_manager.increment_stats_vb(m_DeviceBuffer);
}

void VertexStreamBuffer::Destroy()
{
    if (m_DeviceBuffer == nullptr)
        return;

    HW.m_stats_manager.decrement_stats_vb(m_DeviceBuffer);
    _RELEASE(m_DeviceBuffer);
}

void* VertexStreamBuffer::Map(size_t offset, size_t size, bool flush /*= false*/)
{
    VERIFY(m_DeviceBuffer);

    void *pData = nullptr;
    const auto flags = flush ? LOCKFLAGS_FLUSH : LOCKFLAGS_APPEND;
    R_CHK(m_DeviceBuffer->Lock(offset, size, &pData, flags));
    return pData;
}

void VertexStreamBuffer::Unmap()
{
    VERIFY(m_DeviceBuffer);
    m_DeviceBuffer->Unlock();
}

bool VertexStreamBuffer::IsValid() const
{
    return !!m_DeviceBuffer;
}

//-----------------------------------------------------------------------------
IndexStreamBuffer::~IndexStreamBuffer()
{
    Destroy();
}

void IndexStreamBuffer::Create(size_t size)
{
    R_CHK(HW.pDevice->CreateIndexBuffer(
        size,
        D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
        D3DFMT_INDEX16,
        D3DPOOL_DEFAULT,
        &m_DeviceBuffer,
        NULL));
    VERIFY(m_DeviceBuffer);
    AddRef();
    HW.m_stats_manager.increment_stats_ib(m_DeviceBuffer);
}

void IndexStreamBuffer::Destroy()
{
    if (m_DeviceBuffer == nullptr)
        return;

    HW.m_stats_manager.decrement_stats_ib(m_DeviceBuffer);
    _RELEASE(m_DeviceBuffer);
}

void* IndexStreamBuffer::Map(size_t offset, size_t size, bool flush /*= false*/)
{
    VERIFY(m_DeviceBuffer);

    void *pData = nullptr;
    const auto flags = flush ? LOCKFLAGS_FLUSH : LOCKFLAGS_APPEND;
    m_DeviceBuffer->Lock(offset, size, &pData, flags);
    return pData;
}

void IndexStreamBuffer::Unmap()
{
    VERIFY(m_DeviceBuffer);
    m_DeviceBuffer->Unlock();
}

bool IndexStreamBuffer::IsValid() const
{
    return !!m_DeviceBuffer;
}
