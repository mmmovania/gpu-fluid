/*
Copyright (c) 2013 Benedikt Bitterli

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

#include <GL/glew.h>
#include <algorithm>

#include "BufferObject.hpp"
#include "Texture.hpp"
#include "Debug.hpp"

static const GLenum GlFormatTable[][4][4] = {
    {{  GL_R8,   GL_RG8,   GL_RGB8,   GL_RGBA8},
     {GL_R16F, GL_RG16F, GL_RGB16F, GL_RGBA16F},
     {      0,        0,         0,          0},
     {GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F}},

    {{ GL_R8I,  GL_RG8I,  GL_RGB8I,  GL_RGBA8I},
     {GL_R16I, GL_RG16I, GL_RGB16I, GL_RGBA16I},
     {      0,        0,         0,          0},
     {GL_R32I, GL_RG32I, GL_RGB32I, GL_RGBA32I}},

    {{ GL_R8UI,  GL_RG8UI,  GL_RGB8UI,  GL_RGBA8UI},
     {GL_R16UI, GL_RG16UI, GL_RGB16UI, GL_RGBA16UI},
     {       0,         0,          0,           0},
     {GL_R32UI, GL_RG32UI, GL_RGB32UI, GL_RGBA32UI}},

    {{                   0, 0, 0, 0},
     {GL_DEPTH_COMPONENT16, 0, 0, 0},
     {GL_DEPTH_COMPONENT24, 0, 0, 0},
     {GL_DEPTH_COMPONENT32, 0, 0, 0}},

    {{                   0, 0, 0, 0},
     {                   0, 0, 0, 0},
     { GL_DEPTH24_STENCIL8, 0, 0, 0},
     {GL_DEPTH32F_STENCIL8, 0, 0, 0}},
};

static const GLenum GlTypeTable[][4] = {
        {GL_UNSIGNED_BYTE,          GL_FLOAT,        0,        GL_FLOAT},
        {         GL_BYTE,          GL_SHORT,        0,          GL_INT},
        {GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT,        0, GL_UNSIGNED_INT},
        {               0,          GL_FLOAT, GL_FLOAT,        GL_FLOAT},
        {               0,                 0, GL_FLOAT,        GL_FLOAT},
};

static const GLenum GlTexTable[] = {
        GL_TEXTURE_BUFFER,
        GL_TEXTURE_1D,
        GL_TEXTURE_CUBE_MAP,
        GL_TEXTURE_2D,
        GL_TEXTURE_3D,
};

static const GLenum GlChanTable[][4] = {
        { GL_RED, GL_RG, GL_RGB, GL_RGBA },
        { GL_RED_INTEGER, GL_RG_INTEGER, GL_RGB_INTEGER, GL_RGBA_INTEGER },
        { GL_RED_INTEGER, GL_RG_INTEGER, GL_RGB_INTEGER, GL_RGBA_INTEGER },
        { GL_DEPTH_COMPONENT, 0, 0, 0 },
        { GL_DEPTH_STENCIL, 0, 0, 0 },
};

unsigned long long int Texture::_memoryUsage = 0;
int Texture::_selectedUnit = 0;
int Texture::_nextTicket = 1;
int Texture::_unitTicket[MaxTextureUnits] = {0};
Texture *Texture::_units[MaxTextureUnits] = {0};

Texture::Texture(TextureType type, int width, int height, int depth, int levels) :
        _type(type), _texelType(TEXEL_FLOAT), _channels(0), _chanBytes(0),
        _glName(0), _glFormat(0), _glChanType(0), _elementType(0),
        _elementSize(0), _levels(levels), _boundUnit(-1) {

    _width = _height = _depth = 1;
    _glType = GlTexTable[_type];

    if (_type > TEXTURE_BUFFER)
        _width = width;
    if (_type > TEXTURE_1D)
        _height = height;
    if (_type > TEXTURE_2D)
        _depth = depth;
}

Texture::~Texture() {
    if (_glName) {
        _memoryUsage -= size();
        glDeleteTextures(1, &_glName);
    }
}

void Texture::selectUnit(int unit) {
    if (unit != _selectedUnit) {
        glActiveTexture(GL_TEXTURE0 + unit);
        _selectedUnit = unit;
    }
}

void Texture::markAsUsed(int unit) {
    _unitTicket[unit] = _nextTicket++;
}

int Texture::selectVictimUnit() {
    int leastTicket = _unitTicket[0];
    int leastUnit = 0;

    for (int i = 1; i < MaxTextureUnits; i++) {
        if (_unitTicket[i] < leastTicket) {
            leastTicket = _unitTicket[i];
            leastUnit = i;
        }
    }

    return leastUnit;
}

void Texture::setFormat(TexelType texel, int channels, int chan_bytes) {
    _texelType = texel;
    _channels = channels;
    _chanBytes = chan_bytes;

    ASSERT(channels == 1 || channels == 2 || channels == 4,
            "Number of channels must be 1, 2 or 4\n");

    _glFormat = GlFormatTable[texel][chan_bytes - 1][channels - 1];
    _glChanType = GlChanTable[texel][channels - 1];
    _elementType = GlTypeTable[texel][chan_bytes - 1];
    _elementSize = chan_bytes * channels;

    ASSERT(_glFormat != 0 && _elementType != 0, "Invalid format combination\n");
}

void Texture::setFilter(bool clamp, bool linear) {
    GLenum coord_mode = (clamp ? GL_CLAMP_TO_EDGE : GL_MIRRORED_REPEAT);
    GLenum inter_mode = (linear ? GL_LINEAR : GL_NEAREST);

    bindAny();

    if (_type > TEXTURE_BUFFER) {
        glTexParameteri(_glType, GL_TEXTURE_WRAP_S, coord_mode);
    }

    if (_type > TEXTURE_1D) {
        glTexParameteri(_glType, GL_TEXTURE_WRAP_T, coord_mode);
    }

    if (_type > TEXTURE_2D || _type == TEXTURE_CUBE) {
        glTexParameteri(_glType, GL_TEXTURE_WRAP_R, coord_mode);
    }

    if (_type != TEXTURE_BUFFER) {
        glTexParameteri(_glType, GL_TEXTURE_MIN_FILTER, inter_mode);
        glTexParameteri(_glType, GL_TEXTURE_MAG_FILTER, inter_mode);
        glTexParameteri(_glType, GL_TEXTURE_MAX_LEVEL, _levels - 1);
    }
}

void Texture::init(GLuint bufferObject) {
    glGenTextures(1, &_glName);

    bindAny();

    switch (_type) {
    case TEXTURE_BUFFER:
        glTexBuffer(GL_TEXTURE_BUFFER, _glFormat, bufferObject);
        break;
    case TEXTURE_1D:
        glTexStorage1D(GL_TEXTURE_1D, _levels, _glFormat, _width);
        break;
    case TEXTURE_CUBE:
        glTexStorage2D(GL_TEXTURE_CUBE_MAP, _levels, _glFormat, _width, _height);
        break;
    case TEXTURE_2D:
        glTexStorage2D(GL_TEXTURE_2D, _levels, _glFormat, _width, _height);
        break;
    case TEXTURE_3D:
        glTexStorage3D(GL_TEXTURE_3D, _levels, _glFormat, _width, _height, _depth);
        break;
    }

    _memoryUsage += size();

    setFilter(true, true);
}

void Texture::copy(void *data, int level) {
    bindAny();

    int w = std::max(_width  >> level, 1);
    int h = std::max(_height >> level, 1);
    int d = std::max(_depth  >> level, 1);

    switch (_type) {
    case TEXTURE_BUFFER:
        FAIL("Texture copy not available for texture buffer - use BufferObject::copyData instead");
        break;
    case TEXTURE_1D:
        glTexSubImage1D(GL_TEXTURE_1D, level, 0, w, _glChanType, _elementType, data);
        break;
    case TEXTURE_CUBE:
        for (int i = 0; i < 6; i++) {
            glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, level, 0, 0,
                w, h, _glChanType, _elementType, data);

            if (data)
                data = (uint8_t *)data + w*h*_elementSize;
        }
        break;
    case TEXTURE_2D:
        glTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, w, h,
                _glChanType, _elementType, data);
        break;
    case TEXTURE_3D:
        glTexSubImage3D(GL_TEXTURE_3D, level, 0, 0, 0, w, h, d,
                _glChanType, _elementType, data);
        break;
    }
}

void Texture::copyPbo(BufferObject &pbo, int level) {
    pbo.bind();

    switch (_type) {
    case TEXTURE_BUFFER:
        FAIL("PBO copy not available for texture buffer - use BufferObject::copyData instead");
        break;
    default:
        copy(NULL, level);
    }

    pbo.unbind();
}

void Texture::bindImage(int unit, bool read, bool write, int level) {
    GLenum mode = read ? (write ? GL_READ_WRITE : GL_READ_ONLY) : GL_WRITE_ONLY;

    glBindImageTexture(unit, _glName, level, GL_TRUE, 0, mode, _glFormat);
}

void Texture::bind(int unit) {
    if (_units[unit])
        _units[unit]->_boundUnit = -1;

    markAsUsed(unit);
    selectUnit(unit);

    if (_boundUnit == unit)
        return;

    _units[unit] = this;
    glBindTexture(_glType, _glName);
    _boundUnit = unit;
}

void Texture::bindAny() {
    if (_boundUnit != -1) {
        markAsUsed(_boundUnit);
        selectUnit(_boundUnit);
        return;
    } else
        bind(selectVictimUnit());
}

unsigned long long int Texture::size() {
    switch (_type) {
    case TEXTURE_BUFFER:
    case TEXTURE_1D:
        return ((unsigned long long int)_width)*_elementSize;
    case TEXTURE_CUBE:
        return ((((unsigned long long int)_width)*_height)*_elementSize)*6;
    case TEXTURE_2D:
        return (((unsigned long long int)_width)*_height)*_elementSize;
    case TEXTURE_3D:
        return ((((unsigned long long int)_width)*_height)*_depth)*_elementSize;
    }
    return 0;
}
