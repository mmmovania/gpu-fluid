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

#include "BufferObject.hpp"
#define NULL 0

static GLenum bufferTypes[] = {
    GL_ARRAY_BUFFER,
    GL_ELEMENT_ARRAY_BUFFER,
    GL_PIXEL_PACK_BUFFER,
    GL_PIXEL_UNPACK_BUFFER,
    GL_SHADER_STORAGE_BUFFER,
    GL_UNIFORM_BUFFER,
};

BufferObject::BufferObject(BufferType type) : _type(type), _size(-1), _data(0) {
    _glType = bufferTypes[type];

    glGenBuffers(1, &_glName);
}

BufferObject::BufferObject(BufferType type, GLsizei size) : _type(type), _data(0) {
    _glType = bufferTypes[type];

    glGenBuffers(1, &_glName);
    init(size);
}

void BufferObject::init(GLsizei size) {
    _size = size;
    bind();
    glBufferData(_glType, _size, NULL, GL_STATIC_DRAW);
    unbind();
}

BufferObject::~BufferObject() {
    glDeleteBuffers(1, &_glName);
}

void BufferObject::map(int flags) {
    if (flags & (MAP_INVALIDATE | MAP_INVALIDATE_RANGE))
        invalidate();

    GLuint flag = (flags & MAP_READ) ? ((flags & MAP_WRITE) ? GL_READ_WRITE : GL_READ_ONLY) : GL_WRITE_ONLY;

    _data = glMapBuffer(_glType, flag);
}

void BufferObject::mapRange(GLintptr offset, GLsizeiptr length, int flags) {
    const GLenum flagBits[] = {
        GL_MAP_READ_BIT,
        GL_MAP_WRITE_BIT,
        GL_MAP_INVALIDATE_RANGE_BIT,
        GL_MAP_INVALIDATE_BUFFER_BIT,
        GL_MAP_FLUSH_EXPLICIT_BIT,
        GL_MAP_UNSYNCHRONIZED_BIT,
    };

    GLbitfield glFlags = 0;
    for (int i = 0; i < 6; i++)
        if (flags & (1 << i))
            glFlags |= flagBits[i];

    glMapBufferRange(_glType, offset, length, glFlags);
}

void BufferObject::unmap() {
    _data = NULL;
    glUnmapBuffer(_glType);
}

void BufferObject::copyData(void *data, GLsizei size, GLenum usage) {
    glBufferData(_glType, size, data, usage);
}

void BufferObject::bind() {
    glBindBuffer(_glType, _glName);
}

void BufferObject::unbind() {
    glBindBuffer(_glType, 0);
}

void BufferObject::invalidate() {
    glInvalidateBufferData(_glName);
}

void BufferObject::invalidateRange(GLintptr offset, GLsizeiptr length)  {
    glInvalidateBufferSubData(_glName, offset, length);
}

void BufferObject::bindIndexed(int index) {
    glBindBufferBase(_glType, index, _glName);
}

void BufferObject::bindIndexedRange(int index, GLintptr offset, GLsizeiptr size) {
    glBindBufferRange(_glType, index, _glName, offset, size);
}

void BufferObject::unbindIndexed(int index) {
    glBindBufferBase(_glType, index, 0);
}
