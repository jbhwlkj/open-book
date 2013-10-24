/**
 * Copyright (c) 2012 Nokia Corporation.
 */

#ifndef __BOOK_TEXTUREMANAGER__
#define __BOOK_TEXTUREMANAGER__

#include <QtCore/QThread>
#include <QtOpenGL/QGLWidget>

#include "provider.h"


namespace BE3D {
    class TextureManager : public QThread
    {
    public:
        TextureManager( Provider *provider, QGLWidget *glwidget );
        ~TextureManager();

        void resetUsageFlags();
        GLuint getPageTexture( int pageIndex );
        void getPageInfo( int pageIndex, QString &url, QString &image1, QString &image2 );


        void releaseUnrequired();

    protected:
        GLuint loadingTexture;
        void releaseTextures();
        void run();
        bool running;
        bool threadFinished;

        Provider *provider;
        QGLWidget *glwidget;

        struct SCapsule {
            GLuint textureHandle;
            QImage *loadedTexture;
            int pageIndex;
            bool usageFlag;
            bool failedToLoad;
            QString m_adUrl;
            QString m_picture1Path;
            QString m_picture2Path;

            SCapsule *next;
        };

        SCapsule *textureList;
    };
}

#endif
