/*
 * Copyright (C) 2017 Martin Lambers <marlam@marlam.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <cstdio>
#include <algorithm>

#include <QGuiApplication>
#include <QCommandLineParser>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QSet>

int getI(QOpenGLExtraFunctions* gl, GLenum p)
{
    GLint v = 0;
    gl->glGetIntegerv(p, &v);
    return v;
}

const char* getS(QOpenGLExtraFunctions* gl, GLenum p)
{
    return reinterpret_cast<const char*>(gl->glGetString(p));
}

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    /* Process command line */
    QCommandLineParser parser;
    parser.setApplicationDescription("glinf -- print information about OpenGL or OpenGLES contexts");
    parser.addHelpOption();
    parser.addOptions({
            { { "t", "type" }, "Select context type: 'opengl' or 'opengles'.", "type" },
            { { "p", "profile" }, "Select context profile: 'core' or 'compat'.", "profile" },
            { { "v", "version" }, "Select context version: MAJOR.MINOR.", "version" },
            { { "e", "extensions" }, "List supported extensions." }
    });
    parser.process(app);
    QSurfaceFormat format;
    if (parser.isSet("type")) {
        if (parser.value("type").compare("opengl", Qt::CaseInsensitive) == 0) {
            format.setRenderableType(QSurfaceFormat::OpenGL);
        } else if (parser.value("type").compare("opengles", Qt::CaseInsensitive) == 0) {
            format.setRenderableType(QSurfaceFormat::OpenGLES);
        } else {
            fprintf(stderr, "invalid type\n");
            return 1;
        }
    }
    format.setProfile(QSurfaceFormat::CoreProfile);
    if (parser.isSet("profile")) {
        if (parser.value("profile").compare("core", Qt::CaseInsensitive) == 0) {
            format.setProfile(QSurfaceFormat::CoreProfile);
        } else if (parser.value("profile").compare("compat", Qt::CaseInsensitive) == 0
                || parser.value("profile").compare("compatibility", Qt::CaseInsensitive) == 0) {
            format.setProfile(QSurfaceFormat::CompatibilityProfile);
        } else {
            fprintf(stderr, "invalid profile\n");
            return 1;
        }
    }
    int tryMajorMax = 4;
    int tryMinorMax = 9;
    int tryMajorMin = 3;
    int tryMinorMin = 2;
    if (parser.isSet("version")) {
        QStringList majorminor = parser.value("version").split('.');
        unsigned int major, minor;
        bool majorOk, minorOk;
        if (majorminor.length() == 2) {
            major = majorminor[0].toUInt(&majorOk);
            minor = majorminor[1].toUInt(&minorOk);
        }
        if (majorminor.length() != 2 || !majorOk || !minorOk) {
            fprintf(stderr, "invalid version\n");
            return 1;
        }
        tryMajorMax = tryMajorMin = major;
        tryMinorMax = tryMinorMin = minor;
    }

    /* Initialize OpenGL context */
    QOffscreenSurface* surface = nullptr;
    QOpenGLContext* context = nullptr;
    int tryMajor = tryMajorMax;
    int tryMinor = tryMinorMax;
    bool ok = false;
    for (;;) {
        format.setVersion(tryMajor, tryMinor);
        QSurfaceFormat::setDefaultFormat(format);
        surface = new QOffscreenSurface;
        surface->create();
        context = new QOpenGLContext;
        ok = context->create();
        if (ok) {
            if (context->format().majorVersion() < tryMajor
                    || (context->format().majorVersion() == tryMajor
                        && context->format().minorVersion() < tryMinor)) {
                ok = false;
            }
        }
        if (!ok) {
            bool tryAgain = false;
            if (tryMajor > tryMajorMin && tryMinor == 0) {
                tryMajor--;
                tryMinor = 9;
                tryAgain = true;
            } else if (tryMajor > tryMajorMin || tryMinor > tryMinorMin) {
                tryMinor--;
                tryAgain = true;
            }
            if (tryAgain) {
                delete context;
                context = nullptr;
                delete surface;
                surface = nullptr;
                continue;
            }
        }
        break;
    }
    if (!ok) {
        fprintf(stderr, "cannot create context\n");
        return 1;
    }
    if (!context->makeCurrent(surface)) {
        fprintf(stderr, "cannot make context current\n");
        return 1;
    }
    QOpenGLExtraFunctions* gl = context->extraFunctions();

    /* Print general info */
    QString contextString = context->isOpenGLES() ? "OpenGLES" : "OpenGL";
    contextString += QString(" version ")
        + QString::number(context->format().majorVersion()) + '.'
        + QString::number(context->format().minorVersion());
    if (!context->isOpenGLES()
            && (context->format().majorVersion() > 3
                || (context->format().majorVersion() == 3 && context->format().minorVersion() >= 2))) {
        contextString += QString(" ")
            + (context->format().profile() == QSurfaceFormat::CompatibilityProfile ? "compatibility" : "core")
            + " profile";
    }
    printf("Context:    %s\n", qPrintable(contextString));
    printf("Version:    %s\n", getS(gl, GL_VERSION));
    printf("SL Version: %s\n", getS(gl, GL_SHADING_LANGUAGE_VERSION));
    printf("Vendor:     %s\n", getS(gl, GL_VENDOR));
    printf("Renderer:   %s\n", getS(gl, GL_RENDERER));

    /* Print extensions */
    if (parser.isSet("extensions")) {
        QSet<QByteArray> extensions = context->extensions();
        QList<QByteArray> sortedExtensions;
        foreach (const QByteArray& ext, extensions)
            if (ext.size() > 0)
                sortedExtensions.append(ext);
        std::sort(sortedExtensions.begin(), sortedExtensions.end());
        printf("Extensions:\n");
        foreach (const QByteArray& ext, sortedExtensions)
            printf("    %s\n", ext.constData());
    }

    /* Print implementation-defined limitations */
    printf("Resource limitations:\n");
    printf("  Texture limits:\n");
    printf("    1D / 2D size: %5d  GL_MAX_TEXTURE_SIZE\n", getI(gl, GL_MAX_TEXTURE_SIZE));
    printf("    3D size:      %5d  GL_MAX_3D_TEXTURE_SIZE\n", getI(gl, GL_MAX_3D_TEXTURE_SIZE));
    printf("    Cubemap size: %5d  GL_MAX_CUBE_MAP_TEXTURE_SIZE\n", getI(gl, GL_MAX_CUBE_MAP_TEXTURE_SIZE));
    printf("    Arr. layers:  %5d  GL_MAX_ARRAY_TEXTURE_LAYERS\n", getI(gl, GL_MAX_ARRAY_TEXTURE_LAYERS));
    printf("  Framebuffer object limits:\n");
    printf("    Width:        %5d  GL_MAX_FRAMEBUFFER_WIDTH\n", getI(gl, GL_MAX_FRAMEBUFFER_WIDTH));
    printf("    Height:       %5d  GL_MAX_FRAMEBUFFER_HEIGHT\n", getI(gl, GL_MAX_FRAMEBUFFER_HEIGHT));
    printf("    Color Attach.:%5d  GL_MAX_COLOR_ATTACHMENTS\n", getI(gl, GL_MAX_COLOR_ATTACHMENTS));
    printf("    Draw buffers: %5d  GL_MAX_DRAW_BUFFERS\n", getI(gl, GL_MAX_DRAW_BUFFERS));
    printf("  Maximum number of uniform components in shader stage:\n");
    printf("    Vertex:       %5d  GL_MAX_VERTEX_UNIFORM_COMPONENTS\n", getI(gl, GL_MAX_VERTEX_UNIFORM_COMPONENTS));
    printf("    Tess. Ctrl.:  %5d  GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS\n", getI(gl, GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS));
    printf("    Tess. Eval.:  %5d  GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS\n", getI(gl, GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS));
    printf("    Geometry:     %5d  GL_MAX_GEOMETRY_UNIFORM_COMPONENTS\n", getI(gl, GL_MAX_GEOMETRY_UNIFORM_COMPONENTS));
    printf("    Fragment:     %5d  GL_MAX_FRAGMENT_UNIFORM_COMPONENTS\n", getI(gl, GL_MAX_FRAGMENT_UNIFORM_COMPONENTS));
    printf("    Compute:      %5d  GL_MAX_COMPUTE_UNIFORM_COMPONENTS\n", getI(gl, GL_MAX_COMPUTE_UNIFORM_COMPONENTS));
    printf("  Maximum number of input components in shader stage:\n");
    printf("    Vertex:       %5d  4*GL_MAX_VERTEX_ATTRIBS\n", 4 * getI(gl, GL_MAX_VERTEX_ATTRIBS));
    printf("    Tess. Ctrl.:  %5d  GL_MAX_TESS_CONTROL_INPUT_COMPONENTS\n", getI(gl, GL_MAX_TESS_CONTROL_INPUT_COMPONENTS));
    printf("    Tess. Eval.:  %5d  GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS\n", getI(gl, GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS));
    printf("    Geometry:     %5d  GL_MAX_GEOMETRY_INPUT_COMPONENTS\n", getI(gl, GL_MAX_GEOMETRY_INPUT_COMPONENTS));
    printf("    Fragment:     %5d  GL_MAX_FRAGMENT_INPUT_COMPONENTS\n", getI(gl, GL_MAX_FRAGMENT_INPUT_COMPONENTS));
    //printf("    Vert.->Frag.: %5d  GL_MAX_VARYING_COMPONENTS\n", getI(gl, GL_MAX_VARYING_COMPONENTS));
    printf("  Maximum number of output components in shader stage:\n");
    printf("    Vertex:       %5d  GL_MAX_VERTEX_OUTPUT_COMPONENTS\n", getI(gl, GL_MAX_VERTEX_OUTPUT_COMPONENTS));
    //printf("    Vert.->Frag.: %5d  GL_MAX_VARYING_COMPONENTS\n", getI(gl, GL_MAX_VARYING_COMPONENTS));
    printf("    Tess. Ctrl.:  %5d  GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS\n", getI(gl, GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS));
    printf("    Tess. Eval.:  %5d  GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS\n", getI(gl, GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS));
    printf("    Geometry:     %5d  GL_MAX_GEOMETRY_OUTPUT_COMPONENTS\n", getI(gl, GL_MAX_GEOMETRY_OUTPUT_COMPONENTS));
    printf("    Fragment:     %5d  4*GL_MAX_DRAW_BUFFERS\n", 4 * getI(gl, GL_MAX_DRAW_BUFFERS));
    printf("  Maximum number of samplers in shader stage:\n");
    printf("    Vertex:       %5d  GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS\n", getI(gl, GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS));
    printf("    Tess. Ctrl.:  %5d  GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS\n", getI(gl, GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS));
    printf("    Tess. Eval.:  %5d  GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS\n", getI(gl, GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS));
    printf("    Geometry:     %5d  GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS\n", getI(gl, GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS));
    printf("    Fragment:     %5d  GL_MAX_TEXTURE_IMAGE_UNITS\n", getI(gl, GL_MAX_TEXTURE_IMAGE_UNITS));
    printf("    Compute:      %5d  GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS\n", getI(gl, GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS));
    printf("    Combined:     %5d  GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS\n", getI(gl, GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS));

    return 0;
}
