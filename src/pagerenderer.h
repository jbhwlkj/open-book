/**
 * Copyright (c) 2012 Nokia Corporation.
 */

#ifndef __PAGERENDERER__
#define __PAGERENDERER__

#include <QtCore/QObject>
#include <QtGui/QMatrix4x4>
#include <QtOpenGL/QGLBuffer>
#include <QtOpenGL/QGLWidget>
#include <QtOpenGL/qglshaderprogram.h>


namespace BE3D {

    class PageRenderer : QObject {
        Q_OBJECT

    public:
        PageRenderer( int gridwidth, int gridheight, QObject *parent );
        virtual ~PageRenderer();

        void initializeGL();
        void destroyGL();

        inline int getGridWidth() { return gridWidth; }
        inline int getGridHeight() { return gridHeight; }
        inline QGLShaderProgram& getProgram() { return program; }
        inline GLuint getSamplerLocation() { return samplerLocation; }
        inline GLuint getProjmLocation() { return projmLocation; }
        inline GLuint getDividerMatLocation() { return dividerMatLocation; }
        inline GLuint getShadowPowerLocation() { return shadowPowerLocation; }
        inline QGLBuffer *getIndexBuffer() { return qtibo; }
        inline int getIndexCount() { return indexCount; }

    protected:
        int gridWidth;
        int gridHeight;

        int indexCount;
        QGLBuffer *qtibo;

        QGLShaderProgram program;
        GLuint shadowPowerLocation;
        GLuint dividerMatLocation;
        GLuint samplerLocation;
        GLuint projmLocation;
    };
}



#endif
