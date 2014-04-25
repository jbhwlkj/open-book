/**
 * Copyright (c) 2012-2014 Microsoft Mobile.
 */

#include <QtCore/QDebug>
#include "galleryprovider.h"
#include "texturemanager.h"

using namespace BE3D;

TextureManager::TextureManager( Provider *p, QGLWidget *widget ) : QThread( widget ) {
    provider = p;
    glwidget = widget;
    threadFinished = false;
    running = true;
    textureList = 0;
    loadingTexture = glwidget->bindTexture( QImage(":/images/loading.png"), GL_TEXTURE_2D, GL_RGBA, QGLContext::LinearFilteringBindOption | QGLContext::MipmapBindOption);
    start();
}


TextureManager::~TextureManager() {
    glwidget->deleteTexture( loadingTexture );
    qDebug() << "TextureManager: Shutting down (1).";
    running = false;
    qDebug() << "TextureManager: Shutting down (2).";
    while (threadFinished==false) { msleep(100); }
    qDebug() << "TextureManager: Shutting down (3).";
    releaseTextures();
    qDebug() << "TextureManager: Shutting down ok.";
}

void TextureManager::resetUsageFlags() {
    SCapsule *l = textureList;
    while (l) {
        l->usageFlag = false;
        l = l->next;
    }
}

void TextureManager::releaseTextures() {
    SCapsule *l = textureList;
    int count = 0;
    while (l) {
        SCapsule *n = l->next;
        if (l->loadedTexture) delete l->loadedTexture;
        if (l->textureHandle) glwidget->deleteTexture( l->textureHandle );
        delete l;
        l = n;
        count++;
    }

    qDebug() << "TextureManager: Released " << count << " capsules.";
}

void TextureManager::getPageInfo( int pageIndex, QString &url, QString &image1, QString &image2 )
{
    url = "";
    image1="";
    image2="";

    if (pageIndex<0 || pageIndex>=provider->getTotalPageCount()) return;
    // seek capsules
    SCapsule *l = textureList;
    while (l) {
        if (l->pageIndex == pageIndex) {
            break;
        }
        l = l->next;
    }

        // capsule found,..
    if (l) {
        url = l->m_adUrl;
        image1 = l->m_picture1Path;
        image2 = l->m_picture2Path;
    }
}

GLuint TextureManager::getPageTexture( int pageIndex ) {
    if (pageIndex<0 || pageIndex>=provider->getTotalPageCount()) return 0;
    // seek capsules
    SCapsule *l = textureList;
    while (l) {
        if (l->pageIndex == pageIndex) {
            break;
        }
        l = l->next;
    }

        // capsule found,..
    if (l) {
        l->usageFlag = true;

        if (l->loadedTexture!=0 && l->textureHandle==0) {
            qDebug() << "TextureManager: Creating texture for page " << l->pageIndex;
            l->textureHandle = glwidget->bindTexture( *l->loadedTexture, GL_TEXTURE_2D, GL_RGBA, QGLContext::LinearFilteringBindOption );
        }

        if (l->textureHandle)
            return l->textureHandle;
        else return loadingTexture;
    }

    // Capsule was not found,.. create a new one and add it into the list.
    SCapsule *ncap = new SCapsule;
    ncap->failedToLoad = false;
    ncap->loadedTexture = 0;
    ncap->textureHandle = 0;
    ncap->usageFlag = true;
    ncap->pageIndex = pageIndex;
    ncap->next = 0;

    if (textureList == 0)
        textureList = ncap;
    else {
        l = textureList;
        while (l && l->next) l = l->next;
        l->next = ncap;
    }

    return 0;
}


void TextureManager::releaseUnrequired() {
    SCapsule *l = textureList;
    while (l) {
        if (l->usageFlag == false && (l->loadedTexture || l->textureHandle)) {
            qDebug() << "TextureManager: Releasing page " << l->pageIndex;
            if (l->loadedTexture) {
                delete l->loadedTexture;
                l->loadedTexture = 0;
            }
            if (l->textureHandle) {
                glwidget->deleteTexture( l->textureHandle );
                l->textureHandle = 0;
            }
        }

        l = l->next;
    }
}


void TextureManager::run() {
    /*
    QGLContext *tempCont = new QGLContext( glwidget->context()->format() );
    qDebug() << "create context: " << tempCont->create( glwidget->context() );
    tempCont->makeCurrent();
    */

    threadFinished = false;
    while (running) {
        msleep(100);
        SCapsule *l = textureList;
        while (l) {
            if (l->textureHandle==0 && l->loadedTexture==0 && l->failedToLoad==false && l->usageFlag) {
                qDebug() << "TextureManager: Loading page " << l->pageIndex;
                l->loadedTexture = provider->getPage( l->pageIndex );
                GalleryProvider *galprov = (GalleryProvider*)provider;
                l->m_adUrl = galprov->getLatestUrl();
                l->m_picture1Path = galprov->getLatestPicture(0);
                l->m_picture2Path = galprov->getLatestPicture(1);
                //l->textureHandle = tempCont->bindTexture( *l->loadedTexture, GL_TEXTURE_2D, GL_RGBA, QGLContext::LinearFilteringBindOption | QGLContext::MipmapBindOption);
                if (l->loadedTexture==0) l->failedToLoad = true;
            }
            l = l->next;
        }
    }
    //delete tempCont;
    qDebug() << "TextureManager: Thread finished.";
    threadFinished = true;
}
