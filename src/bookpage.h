/**
 * Copyright (c) 2012 Nokia Corporation.
 */

#ifndef BOOKPAGE_H
#define BOOKPAGE_H

#include "pagerenderer.h"

#include <QtCore/QObject>
#include <QtGui/QMatrix4x4>
#include <QtOpenGL/QGLBuffer>
#include <QtOpenGL/QGLWidget>
#include <QtOpenGL/qglshaderprogram.h>

namespace BE3D {

class BookPage : public QObject {
    Q_OBJECT

public:
    BookPage( float zbase, float xscale, float yscale, PageRenderer *renderer, QObject *parent);
    virtual ~BookPage();

    // Give this page's attributes to another page
    void giveAttributes( BookPage *to );

    // Set the page's main direction (-1 or 1: left or right)
    void setDirection( float set );

    // Update the page's logic "timeStep" seconds forward
    void update( const float timeStep );

    // Render this page with a projection matrix "projm", with chosen
    // front and back textures. If a "topPage" is defines, it's shadow
    // is rendered ontop of this one when using GLES2 - rendering.
    void render(QMatrix4x4 &projm, GLuint frontTexture, GLuint backTexture, BookPage *topPage );

    // Disable "grabbing": If we are pulling (moving) a page,
    // this call releases it.
    void disablePull( const float timeFromGrab ){
        pulling = false;
        incing = true;
        if (timeFromGrab>0 && pageTurn<0.95f && pullDirX<0.0f) {
            pullDirInc[0] = pullDirX/timeFromGrab;
            pullDirInc[1] = pullDirY/timeFromGrab;
            if ( pullDirInc[0]*pullDirInc[0] + pullDirInc[1]*pullDirInc[1] < 1.0f ) {
                pullDirInc[0] = 0;
                pullDirInc[1] = 0;
            }
        } else {
            pullDirInc[0] = 0;
            pullDirInc[1] = 0;
        }
    }

    // Pull a page from specific location to specific direction
    void setPull( float sourceX, float sourceY, float dirX, float dirY, bool fromCenter ) {
        pullFromCenter = fromCenter;
        pullSourceX = sourceX;
        pullSourceY = sourceY;
        pullDirX = dirX;
        pullDirY = dirY;
        pulling = true;
        pullPower = 1.0f;
    }


    inline float getXScale() { return xscale; }
    inline float getYScale() { return yscale; }
    bool isMoving();
    inline float getPageTurn() { return pageTurn; }
    inline float getCurrentDir() { return currentPageDir; }
    inline float getPullDir2DLength() { return pullDir2DLength; }


protected:
    PageRenderer *renderer;

    float currentPageDir;
    bool pullFromCenter;
    bool expulling;
    bool pulling;
    bool incing;
    float pullPower;
    float pullSourceX, pullSourceY;     // pull source coordinates at page-space
    float pullDirX, pullDirY;
    float pullDirNormX, pullDirNormY;
    float pullDirInc[2];

    float pullDir2DLength;
    float exPullDir2DLength;
    float dividerPosX;
    float dividerPosY;
    float pageTurn;

    QVector3D mainVector;
    QVector3D secondVector;

    float xscale, yscale;
    float zbase;

    void recalculateGeometry();
    bool refreshGeometry;
    bool wasMoving;
    GLfloat *vertices;
};
}

#endif
