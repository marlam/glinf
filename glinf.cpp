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
    GLint v;
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
        format.setVersion(major, minor);
    }

    /* Initialize OpenGL context */
    QSurfaceFormat::setDefaultFormat(format);
    QOffscreenSurface surface;
    surface.create();
    QOpenGLContext context;
    if (!context.create()) {
        fprintf(stderr, "cannot create context\n");
        return 1;
    }
    if (!context.makeCurrent(&surface)) {
        fprintf(stderr, "cannot make context current\n");
        return 1;
    }
    QOpenGLExtraFunctions* gl = context.extraFunctions();

    /* Print info */
    QString contextString = context.isOpenGLES() ? "OpenGLES" : "OpenGL";
    if (!context.isOpenGLES())
        contextString += QString(", ") + (context.format().profile() == QSurfaceFormat::CompatibilityProfile ? "compatibility" : "core") + " profile";
    contextString += QString(", version ") + QString::number(context.format().majorVersion()) + '.'
        + QString::number(context.format().minorVersion());
    printf("Context:    %s\n", qPrintable(contextString));
    printf("Version:    %s\n", getS(gl, GL_VERSION));
    printf("SL Version: %s\n", getS(gl, GL_SHADING_LANGUAGE_VERSION));
    printf("Vendor:     %s\n", getS(gl, GL_VENDOR));
    printf("Renderer:   %s\n", getS(gl, GL_RENDERER));
    printf("Limitations:\n");
    printf("    GL_MAX_ARRAY_TEXTURE_LAYERS: %d\n", getI(gl, GL_MAX_ARRAY_TEXTURE_LAYERS));
    printf("    GL_MAX_COLOR_ATTACHMENTS: %d\n", getI(gl, GL_MAX_COLOR_ATTACHMENTS));
    printf("    GL_MAX_COMBINED_ATOMIC_COUNTERS: %d\n", getI(gl, GL_MAX_COMBINED_ATOMIC_COUNTERS));
    printf("    GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS: %d\n", getI(gl, GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS));
    printf("    GL_MAX_CUBE_MAP_TEXTURE_SIZE: %d\n", getI(gl, GL_MAX_CUBE_MAP_TEXTURE_SIZE));
    printf("    GL_MAX_DRAW_BUFFERS: %d\n", getI(gl, GL_MAX_DRAW_BUFFERS));
    printf("    GL_MAX_ELEMENTS_INDICES: %d\n", getI(gl, GL_MAX_ELEMENTS_INDICES));
    printf("    GL_MAX_ELEMENTS_VERTICES: %d\n", getI(gl, GL_MAX_ELEMENTS_VERTICES));
    printf("    GL_MAX_FRAMEBUFFER_HEIGHT: %d\n", getI(gl, GL_MAX_FRAMEBUFFER_HEIGHT));
    printf("    GL_MAX_FRAMEBUFFER_WIDTH: %d\n", getI(gl, GL_MAX_FRAMEBUFFER_WIDTH));
    printf("    GL_MAX_TEXTURE_SIZE: %d\n", getI(gl, GL_MAX_TEXTURE_SIZE));
    if (parser.isSet("extensions")) {
        QSet<QByteArray> extensions = context.extensions();
        QList<QByteArray> sortedExtensions;
        foreach (const QByteArray& ext, extensions)
            if (ext.size() > 0)
                sortedExtensions.append(ext);
        std::sort(sortedExtensions.begin(), sortedExtensions.end());
        printf("Extensions:\n");
        foreach (const QByteArray& ext, sortedExtensions)
            printf("    %s\n", ext.constData());
    }

    return 0;
}
