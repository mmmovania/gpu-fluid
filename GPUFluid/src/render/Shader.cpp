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
#include <stdio.h>

#include "Shader.hpp"
#include "Debug.hpp"
#include "Util.hpp"

Shader::Shader(const char *prefix, const char *preamble, const char *v, const char *g,
        const char *f, int outputs) : _program(-1), _shaderCount(0), _outputCount(0),
        _varyingCount(0), _feedbackMode(FEEDBACK_INTERLEAVED), _uniformCount(0) {

    char fullPreamble[1024], fullV[1024], fullG[1024], fullF[1024];

    sprintf_s(fullPreamble, "%s%s", prefix, preamble);
    sprintf_s(fullV,        "%s%s", prefix, v);
    sprintf_s(fullG,        "%s%s", prefix, g);
    sprintf_s(fullF,        "%s%s", prefix, f);

    ShaderObject *vert, *frag, *geom;

    if (f) {
        frag = addObject();
        frag->addFile(fullPreamble);
        frag->addFile(fullF);
        frag->compile(FRAGMENT_SHADER);
    }

    vert = addObject();
    vert->addFile(fullPreamble);
    vert->addFile(fullV);
    vert->compile(VERTEX_SHADER);

    if (g) {
        geom = addObject();
        geom->addFile(fullPreamble);
        geom->addFile(fullG);
        geom->compile(GEOMETRY_SHADER);
    }

    char out[] = "FragColor0";
    for (int i = 0; i < outputs; i++, out[9]++) /* More than 10 outputs? Pfff who cares */
        addOutput(out);

    link();
}

Shader::Shader(const char *prefix, const char *preamble, const char *c) :
        _program(-1), _shaderCount(0), _outputCount(0),
        _varyingCount(0), _feedbackMode(FEEDBACK_INTERLEAVED), _uniformCount(0) {

    char fullPreamble[1024], fullC[1024];

    sprintf_s(fullPreamble, "%s%s", prefix, preamble);
    sprintf_s(fullC,        "%s%s", prefix, c);

    ShaderObject *compute = addObject();
    compute->addFile(fullPreamble);
    compute->addFile(fullC);
    compute->compile(COMPUTE_SHADER);

    link();
}

ShaderObject *Shader::addObject() {
    return &_shaders[_shaderCount++];
}

void Shader::addOutput(const char *name) {
    _outputs[_outputCount++] = name;
}

void Shader::addFeedbackVarying(const char *name) {
    _varyings[_varyingCount++] = name;
}

void Shader::setFeedbackMode(enum FeedbackMode f) {
    _feedbackMode = f;
}

int Shader::refresh() {
    int linkFlag = 0;

    for (int i = 0; i < _shaderCount; i++)
        if (_shaders[i].refresh())
            linkFlag = 1;

    if (linkFlag)
        link();

    return linkFlag;
}

void Shader::link() {
    _program = glCreateProgram();

    for (int i = 0; i < _shaderCount; i++)
        glAttachShader(_program, _shaders[i].name());

    for (int i = 0; i < _outputCount; i++)
        glBindFragDataLocation(_program, i, _outputs[i]);

    if (_varyingCount) {
        GLenum mode = (_feedbackMode == FEEDBACK_INTERLEAVED ? GL_INTERLEAVED_ATTRIBS : GL_SEPARATE_ATTRIBS);
        glTransformFeedbackVaryings(_program, _varyingCount, _varyings, mode);
    }

    glLinkProgram(_program);

    check();
}

void Shader::bind() {
    glUseProgram(_program);
}

void Shader::dispatch(int sizeX, int sizeY, int sizeZ) {
    glDispatchCompute(sizeX, sizeY, sizeZ);
}

int Shader::uniformIndex(const char *name) {
    int hash = stringHash(name);

    for (int i = 0; i < _uniformCount; i++) {
        if (_uniformHash[i] == hash)
            return i;
    }

    _uniformHash[_uniformCount] = hash;
    _uniformLocation[_uniformCount] = glGetUniformLocation(_program, name);
    _uniformCount++;

    return _uniformCount - 1;
}

GLint Shader::uniform(const char *name) {
    return _uniformLocation[uniformIndex(name)];
}

void Shader::check() {
    GLuint obj = _program;

    int status, length;
    glGetProgramiv(obj, GL_LINK_STATUS, &status);
    glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &length);

    glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &length);
    GLchar *log;
    if (length > 1) {
        log = (GLchar *)malloc(length*sizeof(GLchar));
        glGetProgramInfoLog(obj, length, NULL, log);

        DBG("shader", WARN, "%s\n", log);
    }
}

/* TODO: Kill it with fire and rebuild with a generic version */

void Shader::uniformI(const char *name, int i) {
    int t = uniformIndex(name);
    if (_uniformVals[t].i != i) {
        glUniform1i(_uniformLocation[t], i);
        _uniformVals[t].i = i;
    }
}

void Shader::uniformI(const char *name, int i1, int i2) {
    int t = uniformIndex(name);
    if (_uniformVals[t].i2[0] != i1 || _uniformVals[t].i2[1] != i2) {
        glUniform2i(_uniformLocation[t], i1, i2);
        _uniformVals[t].i2[0] = i1;
        _uniformVals[t].i2[1] = i2;
    }
}

void Shader::uniformI(const char *name, int i1, int i2, int i3) {
    int t = uniformIndex(name);
    int *is = _uniformVals[t].i3;
    if (is[0] != i1 || is[1] != i2 || is[2] != i3) {
        glUniform3i(_uniformLocation[t], i1, i2, i3);
        is[0] = i1;
        is[1] = i2;
        is[2] = i3;
    }
}

void Shader::uniformI(const char *name, int i1, int i2, int i3, int i4) {
    int t = uniformIndex(name);
    int *is = _uniformVals[t].i4;
    if (is[0] != i1 || is[1] != i2 || is[2] != i3 || is[3] != i4) {
        glUniform4i(_uniformLocation[t], i1, i2, i3, i4);
        is[0] = i1;
        is[1] = i2;
        is[2] = i3;
        is[3] = i4;
    }
}

void Shader::uniformF(const char *name, float f) {
    int t = uniformIndex(name);
    if (_uniformVals[t].f != f) {
        glUniform1f(_uniformLocation[t], f);
        _uniformVals[t].f = f;
    }
}
void Shader::uniformF(const char *name, float f1, float f2) {
    int t = uniformIndex(name);
    if (_uniformVals[t].f2[0] != f1 || _uniformVals[t].f2[1] != f2) {
        glUniform2f(_uniformLocation[t], f1, f2);
        _uniformVals[t].f2[0] = f1;
        _uniformVals[t].f2[1] = f2;
    }
}

void Shader::uniformF(const char *name, float f1, float f2, float f3) {
    int t = uniformIndex(name);
    float *fs = _uniformVals[t].f3;
    if (fs[0] != f1 || fs[1] != f2 || fs[2] != f3) {
        glUniform3f(_uniformLocation[t], f1, f2, f3);
        fs[0] = f1;
        fs[1] = f2;
        fs[2] = f3;
    }
}


void Shader::uniformF(const char *name, float f1, float f2, float f3, float f4) {
    int t = uniformIndex(name);
    float *fs = _uniformVals[t].f4;
    if (fs[0] != f1 || fs[1] != f2 || fs[2] != f3 || fs[3] != f4) {
        glUniform4f(_uniformLocation[t], f1, f2, f3, f4);
        fs[0] = f1;
        fs[1] = f2;
        fs[2] = f3;
        fs[3] = f4;
    }
}

void Shader::uniformF(const char *name, const Vec3 &v) {
    int t = uniformIndex(name);
    float *fs = _uniformVals[t].f3;
    if (fs[0] != v.x || fs[1] != v.y || fs[2] != v.z) {
        glUniform3f(_uniformLocation[t], v.x, v.y, v.z);
        fs[0] = v.x;
        fs[1] = v.y;
        fs[2] = v.z;
    }
}


void Shader::uniformF(const char *name, const Vec4 &v) {
    int t = uniformIndex(name);
    float *fs = _uniformVals[t].f4;
    if (fs[0] != v.x || fs[1] != v.y || fs[2] != v.z || fs[3] != v.w) {
        glUniform4f(_uniformLocation[t], v.x, v.y, v.z, v.w);
        fs[0] = v.x;
        fs[1] = v.y;
        fs[2] = v.z;
        fs[3] = v.w;
    }
}

void Shader::uniformMat(const char *name, const Mat4 &m, bool transpose) {
    glUniformMatrix4fv(_uniformLocation[uniformIndex(name)], 1, transpose, m.a);
}
