#include "stdafx.h"
#include "rgles.h"

#include "Layers/xrRender/ShaderResourceTraits.h"
#include "xrCore/FileCRC32.h"

void show_errors(cpcstr filename, GLuint* program, GLuint* shader)
{
    GLint length;
    GLchar *errors = nullptr, *sources = nullptr;

    if (program)
    {
        CHK_GL(glGetProgramiv(*program, GL_INFO_LOG_LENGTH, &length));
        errors = xr_alloc<GLchar>(length);
        CHK_GL(glGetProgramInfoLog(*program, length, nullptr, errors));
    }
    else if (shader)
    {
        CHK_GL(glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &length));
        errors = xr_alloc<GLchar>(length);
        CHK_GL(glGetShaderInfoLog(*shader, length, nullptr, errors));

        CHK_GL(glGetShaderiv(*shader, GL_SHADER_SOURCE_LENGTH, &length));
        sources = xr_alloc<GLchar>(length);
        CHK_GL(glGetShaderSource(*shader, length, nullptr, sources));
    }

    Log("! shader compilation failed:", filename);
    if (errors)
        Log("! error: ", errors);

    if (sources)
    {
        Log("Shader source:");
        Log(sources);
        Log("Shader source end.");
    }
    xr_free(errors);
    xr_free(sources);
}

template <typename T>
static GLuint create_shader(pcstr* buffer, size_t const buffer_size, cpcstr filename,
    T*& result, const GLenum* format)
{
    auto [shader, program] = ShaderTypeTraits<T>::CreateHWShader(buffer, buffer_size, result->sh, format, filename);

    const bool success = shader == 0 && program != 0 && program != GLuint(-1);

    // Parse constant, texture, sampler binding
    if (success)
    {
        result->sh = program;
        // Let constant table parse it's data
        result->constants.parse(&program, ShaderTypeTraits<T>::GetShaderDest());
    }
    else
    {
        if (shader != 0 && shader != GLuint(-1))
        {
            show_errors(filename, nullptr, &shader);
            glDeleteShader(shader);
        }
        else
        {
            show_errors(filename, &program, nullptr);
            glDeleteProgram(program);
        }
    }

    return success ? program : 0;
}

static GLuint create_shader(cpcstr pTarget, pcstr* buffer, size_t const buffer_size,
    cpcstr filename, void*& result, const GLenum* format)
{
    switch (pTarget[0])
    {
    case 'p':
        return create_shader(buffer, buffer_size, filename, (SPS*&)result, format);
    case 'v':
        return create_shader(buffer, buffer_size, filename, (SVS*&)result, format);
    default:
        NODEFAULT;
        return 0;
    }
}

class shader_name_holder
{
    size_t pos{};
    string_path name;

public:
    void append(cpcstr string)
    {
        const size_t size = xr_strlen(string);
        for (size_t i = 0; i < size; ++i)
        {
            name[pos] = string[i];
            ++pos;
        }
    }

    void append(u32 value)
    {
        name[pos] = '0' + char(value); // NOLINT
        ++pos;
    }

    void finish()
    {
        name[pos] = '\0';
    }

    pcstr c_str() const { return name; }
};

class shader_options_holder
{
    size_t pos{};
    string512 m_options[128];

public:
    void add(cpcstr name, cpcstr value)
    {
        // It's important to have postfix increment!
        xr_sprintf(m_options[pos++], "#define %s\t%s\n", name, value);
    }

    void finish()
    {
        m_options[pos][0] = '\0';
    }

    [[nodiscard]] size_t size() const { return pos; }
    string512& operator[](size_t idx) { return m_options[idx]; }
};

class shader_sources_manager
{
    pcstr* m_sources{};
    size_t m_sources_lines{};
    xr_vector<pstr> m_source, m_includes;
    string512 m_name_comment;

public:
    explicit shader_sources_manager(cpcstr name)
    {
        xr_sprintf(m_name_comment, "// %s\n", name);
    }

    ~shader_sources_manager()
    {
        // Free string resources
        xr_free(m_sources);
        for (pstr include : m_includes)
            xr_free(include);
        m_source.clear();
        m_includes.clear();
    }

    [[nodiscard]] auto get() const { return m_sources; }
    [[nodiscard]] auto length() const { return m_sources_lines; }

    [[nodiscard]] static constexpr bool optimized()
    {
#ifdef DEBUG
        return false;
#else
        return true;
#endif
    }

    void compile(IReader* file, shader_options_holder& options)
    {
        load_includes(file);
        apply_options(options);
    }

private:
    // TODO: OGL: make ignore commented includes
    void load_includes(IReader* file)
    {
        cpcstr sourceData = static_cast<cpcstr>(file->pointer());
        const size_t dataLength = file->length();

        // Copy source file data into a null-terminated buffer
        cpstr data = xr_alloc<char>(dataLength + 2);
        CopyMemory(data, sourceData, dataLength);
        data[dataLength] = '\n';
        data[dataLength + 1] = '\0';
        m_includes.push_back(data);
        m_source.push_back(data);

        string_path path;
        pstr str = data;
        while (strstr(str, "#include") != nullptr)
        {
            // Get filename of include directive
            str = strstr(str, "#include"); // Find the include directive
            char* fn = strchr(str, '"') + 1; // Get filename, skip quotation
            *str = '\0'; // Terminate previous source
            str = strchr(fn, '"'); // Get end of filename path
            *str = '\0'; // Terminate filename path

            // Create path to included shader
            strconcat(sizeof(path), path, GEnv.Render->getShaderPath(), fn);
            FS.update_path(path, _game_shaders_, path);
            while (cpstr sep = strchr(path, '/'))
                *sep = '\\';

            // Open and read file, recursively load includes
            IReader* R = FS.r_open(path);
            R_ASSERT2(R, path);
            load_includes(R);
            FS.r_close(R);

            // Add next source, skip quotation
            ++str;
            m_source.push_back(str);
        }
    }

    void apply_options(shader_options_holder& options)
    {
        // Compile sources list
        const size_t head_lines = 2; // "#version" line + name_comment line
        m_sources_lines = m_source.size() + options.size() + head_lines;
        m_sources = xr_alloc<pcstr>(m_sources_lines);
#ifdef DEBUG
        m_sources[0] = "#version 410\n#pragma optimize (off)\n";
#else
        m_sources[0] = "#version 410\n";
#endif
        m_sources[1] = m_name_comment;

        // Make define lines
        for (size_t i = 0; i < options.size(); ++i)
        {
            m_sources[head_lines + i] = options[i];
        }
        CopyMemory(m_sources + head_lines + options.size(), m_source.data(), m_source.size() * sizeof(pstr));
    }
};

HRESULT CRender::shader_compile(LPCSTR name, IReader* fs, LPCSTR pFunctionName,
    LPCSTR pTarget, DWORD Flags, void*& result)
{
    shader_options_holder options;
    shader_name_holder sh_name;

    // Don't move these variables to lower scope!
    string32 c_smapsize;
    string32 c_gloss;
    string32 c_sun_shafts;
    string32 c_ssao;
    string32 c_sun_quality;
    string32 c_water_reflection;

    // TODO: OGL: Implement these parameters.
    UNUSED(pFunctionName);
    UNUSED(Flags);

    // options:
    const auto appendShaderOption = [&](u32 option, cpcstr macro, cpcstr value)
    {
        if (option)
            options.add(macro, value);

        sh_name.append(option);
    };

    // Branching
    appendShaderOption(HW.Caps.raster_major >= 3, "USE_BRANCHING", "1");

    // Vertex texture fetch
    appendShaderOption(HW.Caps.geometry.bVTF, "USE_VTF", "1");

    // Force skinw
    appendShaderOption(o.forceskinw, "SKIN_COLOR", "1");

    // skinning
    // SKIN_NONE
    appendShaderOption(m_skinning < 0, "SKIN_NONE", "1");

    // SKIN_0
    appendShaderOption(0 == m_skinning, "SKIN_0", "1");

    // SKIN_1
    appendShaderOption(1 == m_skinning, "SKIN_1", "1");

    // SKIN_2
    appendShaderOption(2 == m_skinning, "SKIN_2", "1");

    // SKIN_3
    appendShaderOption(3 == m_skinning, "SKIN_3", "1");

    // SKIN_4
    appendShaderOption(4 == m_skinning, "SKIN_4", "1");

    // FXAA
    // SkyLoader: temporary added
    appendShaderOption(ps_r2_fxaa, "USE_FXAA", "1");
    // end

    // Shadow of Chernobyl compatibility
    appendShaderOption(ShadowOfChernobylMode, "USE_SHOC_RESOURCES", "1");

    // Don't mix optimized and unoptimized shaders
    sh_name.append(static_cast<u32>(shader_sources_manager::optimized()));

    // finish
    options.finish();
    sh_name.finish();

    char extension[3];
    strncpy_s(extension, pTarget, 2);

    u32 fileCrc = 0;
    string_path filename, full_path;
    strconcat(sizeof(filename), filename, "gl" DELIMITER, name, ".", extension, DELIMITER, sh_name.c_str());
    if (HW.ShaderBinarySupported)
    {
        string_path file;
        strconcat(sizeof(file), file, "shaders_cache" DELIMITER, filename);
        FS.update_path(full_path, "$app_data_root$", file);

        string_path shadersFolder;
        FS.update_path(shadersFolder, "$game_shaders$", GEnv.Render->getShaderPath());

        getFileCrc32(fs, shadersFolder, fileCrc);
        fs->seek(0);
    }

    GLuint program = 0;
    if (HW.ShaderBinarySupported && FS.exist(full_path))
    {
        IReader* file = FS.r_open(full_path);
        if (file->length() > 8)
        {
            xr_string renderer, glVer, shadingVer;
            file->r_string(renderer);
            file->r_string(glVer);
            file->r_string(shadingVer);

            if (0 == xr_strcmp(renderer.c_str(), HW.AdapterName) &&
                0 == xr_strcmp(glVer.c_str(), HW.OpenGLVersionString) &&
                0 == xr_strcmp(shadingVer.c_str(), HW.ShadingVersion))
            {
                const GLenum binaryFormat = file->r_u32();

                const u32 savedFileCrc = file->r_u32();
                if (savedFileCrc == fileCrc)
                {
                    const u32 savedBytecodeCrc = file->r_u32();
                    const u32 bytecodeCrc = crc32(file->pointer(), file->elapsed());
                    if (bytecodeCrc == savedBytecodeCrc)
                        program = create_shader(pTarget, (pcstr*)file->pointer(), file->elapsed(), filename, result, &binaryFormat);
                }
            }
        }
        file->close();
    }

    // Failed to use cached shader, then:
    if (!program)
    {
        // Compile sources list
        shader_sources_manager sources(name);
        sources.compile(fs, options);

        // Compile the shader from sources
        program = create_shader(pTarget, sources.get(), sources.length(), filename, result, nullptr);

        if (HW.ShaderBinarySupported && program)
        {
            GLvoid* binary{};
            GLint binaryLength{};
            GLenum binaryFormat{};
            glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
            if (binaryLength)
                binary = static_cast<GLvoid*>(xr_malloc(binaryLength));

            if (binary)
            {
                glGetProgramBinary(program, binaryLength, nullptr, &binaryFormat, binary);
                IWriter* file = FS.w_open(full_path);

                file->w_string(HW.AdapterName);
                file->w_string(HW.OpenGLVersionString);
                file->w_string(HW.ShadingVersion);

                file->w_u32(binaryFormat);
                file->w_u32(fileCrc);

                const u32 bytecodeCrc = crc32(binary, binaryLength);
                file->w_u32(bytecodeCrc);

                file->w(binary, binaryLength);
                FS.w_close(file);
                xr_free(binary);
            }
        }
    }

    if (program)
        return S_OK;

    return E_FAIL;
}
