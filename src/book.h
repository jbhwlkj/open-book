/**
 * Copyright (c) 2012 Nokia Corporation.
 */

#ifndef __BOOK__
#define __BOOK__

#include <QtCore/QObject>
#include <QtGui/QImage>

#include "bookpage.h"
#include "pagerenderer.h"
#include "provider.h"
#include "texturemanager.h"

namespace BE3D {

    class Book : public QObject {
        Q_OBJECT

    public:
        Book(float xscale, float yscale, Provider *provider, QGLWidget *w );
        virtual ~Book();

        void initializeGL();
        void destroyGL();

        // Update the book's logic with "timeStep"- seconds forward. Screen's
        // current aspectratio is required.
        void update( const float timeStep, const float currentAspect );

        // Render the page with projection matrix "projm"
        void render( QMatrix4x4 &projm );

        // Disable "grabbing" of a page if currently enabled.
        void disablePull(bool justCancel = false);

        // Set mouse / touchevent pull to attributes. Book will handle all of
        // the required logic.
        bool setPull( float sourceX, float sourceY, float dirX, float dirY, float aspect );

        void click( float x, float y );

        void zoomIn();
        void zoomOut();
        bool isZoomedIn() { if (viewZTarget>0.5f) return true; else return false; }

    protected:
        float viewX, viewY, viewZ;
        float viewXInc, viewYInc;
        float viewZTarget;

        QGLWidget *glwidget;
        TextureManager *textureManager;
        Provider *provider;
        int currentStartPage;
        //GLuint pageTextures[6];
        PageRenderer *renderer;

        bool pulling;
        float pullDir;
        float pullFromCenter;
        float grabTimer;
        float grabPos[2];
        float exPullDir[2];
        BookPage *pullingPage;
        float xscale, yscale;

            // Pages of the book. There are always just 4 of them.
        BookPage *visiblePages[4];
    };
}

#endif
