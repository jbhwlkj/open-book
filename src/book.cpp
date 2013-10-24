/**
 * Copyright (c) 2012 Nokia Corporation.
 */

#include "book.h"

#include <QtCore/QDebug>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>
#include <QtOpenGL/QGLWidget>
#include <math.h>

using namespace BE3D;

#define KINETIC_SCROLL_POWER 1.0f
#define KINETIC_SCROLL_SLOWDOWN 2.0f
#define KINETIC_EDGE_RETURN_POWER 6.0f
#define Z_ZOOM_POWER 5.0f


/*!
  \class Book
  \brief The book object containing all of the pages and logic for their
         manipulation.
*/

Book::Book(float xs, float ys, Provider *p, QGLWidget *parent) :
    QObject( parent ),
    viewXInc(0.0f),
    viewYInc(0.0f),
    provider(p),
    xscale(xs),
    yscale(ys),
    glwidget(parent),
    grabTimer(0),
    textureManager(0),
    viewX(0),
    viewY(0),
    viewZ(0),
    viewZTarget(0),
    pullingPage(0),
    pulling(false)
{
    renderer = new PageRenderer( 20,15, parent );
    textureManager = 0;
    exPullDir[0] = 0;
    exPullDir[1] = 0;
    for (int f=0; f<4; f++) {
        visiblePages[f] = new BookPage( -2.0f, xscale, yscale, renderer, parent );
    }

    currentStartPage = -3;
    visiblePages[0]->setDirection(-1);
    visiblePages[1]->setDirection(-1);
}

Book::~Book() {
    if (textureManager) delete textureManager;
    if (renderer) delete renderer;

    for (int f=0; f<4; f++) {
        delete visiblePages[f];
    }
}

void Book::initializeGL() {
    qDebug() << "Book: Initialize GL.";
    renderer->initializeGL();
    textureManager = new TextureManager( provider, glwidget );
}

void Book::destroyGL() {
    qDebug() << "Book: Destroy GL.";
    if (textureManager) {
        delete textureManager;
        textureManager = 0;
    }
    renderer->destroyGL();
}

void Book::disablePull(bool justCancel) {
    if (!pulling) return;
    pulling = false;

    if (pullingPage) {
        exPullDir[0] = 0;
        exPullDir[1] = 0;
        pullingPage->disablePull( grabTimer+0.00001f );
        pullingPage = 0;
    }

    if (isZoomedIn()) {
        if (grabTimer==0.0f) grabTimer = 0.000001f;
        viewXInc = exPullDir[0]*KINETIC_SCROLL_POWER / grabTimer;
        viewYInc = exPullDir[1]*KINETIC_SCROLL_POWER / grabTimer;
        exPullDir[0] = 0;
        exPullDir[1] = 0;
        return;
    }


}

void Book::click( float sourceX, float sourceY ) {
    qDebug() << "Book-Click";

    int pageIndex = -1;
    // Scale the coordinates according books dimensions
    sourceX = (sourceX - 0.45f)*xscale*0.95f + 0.5f;

    if (sourceX < 0.5f) {
        if (currentStartPage<=-3) return;   // No page below
        pageIndex = currentStartPage+2;
        sourceY = (sourceY-0.05f);
        sourceX = (0.5f-sourceX) * 2.0f;
    } else {
        if (currentStartPage>provider->getTotalPageCount()-4) return;   // No page below
        pageIndex = currentStartPage+3;
        sourceX = (sourceX - 0.5f) * 2.0f;
        sourceY = (sourceY-0.05f);
    }

    QString url, i1,i2;
    textureManager->getPageInfo(pageIndex, url, i1,i2 );
    // process the click
    qDebug("clicking page at: %f, %f", sourceX,sourceY );
    if (sourceY<0.5f) {
        // Picture 1 clicked
        if (sourceY<0.2f) {
            // Advertisement cliced
            qDebug() << "adcliced: " << url;
            QDesktopServices::openUrl( QUrl( url ) );
        } else {
            qDebug() << "image2cliced: " << i2;
            QDesktopServices::openUrl( QUrl( QString("file:///") + i2 ));
        }

    }
    else if (sourceY>0.2) {
        // Picture 2 clicked.

        qDebug() << "image1cliced: " << i1;
        QDesktopServices::openUrl( QUrl(  QString("file:///") + i1 ));
    }
}



bool Book::setPull( float sourceX, float sourceY, float dirX, float dirY, float aspect ) {

    if (isZoomedIn()) {
        // slow down the scrolling
        dirX *= 0.5f;
        dirY *= 0.5f;
        viewX -= (exPullDir[0] - dirX);
        viewY -= (exPullDir[1] - dirY);
        exPullDir[0] = dirX;
        exPullDir[1] = dirY;
        if (!pulling) {
            grabTimer = 0.0f;
            viewXInc = 0.0f;
            viewYInc = 0.0f;
        }
        pulling = true;
        return true;
    }


    // Scale the coordinates according books dimensions
    //sourceX = (sourceX - 0.5f)*xscale + 0.5f;
    sourceX = (sourceX - 0.45f)*xscale*0.95f + 0.5f;

    //qDebug() << "Source X : " << sourceX;


    pulling = true;
    // If not yet pulling any page
    if (pullingPage==0) {
        if (sourceX<0 || sourceY<0 || sourceX > 1.0f || sourceY>1.0f) return false;
        // Do not continue while current texturechain is not yet completed


        // If grab did not happend at the beginning, don't let it start at all
        //if (dirX*dirX + dirY*dirY > 0.1f) return;

        //if (visiblePages[1]->isMoving() || visiblePages[2]->isMoving()) return;
        if (sourceX < 0.5f) {
            if (currentStartPage<=-3) return true;
            if (sourceX< 0.5f-xscale/4.0f) pullFromCenter = false; else pullFromCenter = true;
            pullingPage = visiblePages[1];
            pullDir = -1;
            grabPos[1] = (sourceY-0.05f);

            if (pullFromCenter)
                grabPos[0] = (0.5f-sourceX) * 2.0f;
            else
                grabPos[0] = 1.0f;

        } else {
            if (currentStartPage>provider->getTotalPageCount()-4) return true;
            if (sourceX>0.5f + xscale/4.0f) pullFromCenter = false; else pullFromCenter = true;
            pullingPage = visiblePages[2];
            pullDir = 1;
            if (pullFromCenter)
                grabPos[0] = (sourceX - 0.5f) * 2.0f;
            else
                grabPos[0] = 1.0f;

            grabPos[1] = (sourceY-0.05f);

        }

        grabTimer = 0;
        qDebug() << "grabbing at : " << grabPos[0] << "," << grabPos[1];

        // check that if we are pulling from the edge or from the center.
    }

    if (pullingPage) {
        if (dirX * pullingPage->getCurrentDir()  > 0.0f ) {
            dirX = 0;
            dirY = 0;
        }

        dirX *= pullDir;
        if (dirX*dirX + dirY*dirY > 0.005f) {
            pullingPage->setPull(grabPos[0],grabPos[1], dirX, dirY, pullFromCenter );
        }

    }
    return true;
}

void Book::zoomIn() {
    viewZTarget = 1.0f;
}

void Book::zoomOut() {
    viewZTarget = 0.0f;
}


void Book::update( const float frameDelta, const float currentAspect ) {

    grabTimer+=frameDelta;
    textureManager->releaseUnrequired();

    float viewXMax = 0.4f*currentAspect;
    float viewYMax = 0.4f;


    if (isZoomedIn()) {

    } else {

        viewX -= viewX * frameDelta * 4.0f;
        viewY -= viewY * frameDelta * 4.0f;
    }

    viewXInc -= viewXInc * frameDelta*KINETIC_SCROLL_SLOWDOWN;
    viewYInc -= viewYInc * frameDelta*KINETIC_SCROLL_SLOWDOWN;;
    viewX += viewXInc*frameDelta;
    viewY += viewYInc*frameDelta;

    if (viewX>viewXMax) viewX += (viewXMax-viewX)*frameDelta*KINETIC_EDGE_RETURN_POWER;
    if (viewX<-viewXMax) viewX += (-viewXMax-viewX)*frameDelta*KINETIC_EDGE_RETURN_POWER;
    if (viewY>viewYMax) viewY += (viewYMax-viewY)*frameDelta*KINETIC_EDGE_RETURN_POWER;
    if (viewY<-viewYMax) viewY += (-viewYMax-viewY)*frameDelta*KINETIC_EDGE_RETURN_POWER;

    viewZ += (viewZTarget - viewZ) * frameDelta * Z_ZOOM_POWER;

    //if (viewZTarget > viewZTarget)


    // update the books.

    visiblePages[0]->update(frameDelta);
    visiblePages[1]->update(frameDelta);
    visiblePages[2]->update(frameDelta);
    visiblePages[3]->update(frameDelta);

    // only the two middleone needs to be updated


    // Check if the stack needs to be moved

    if (pullingPage==0) {
        // If both are in same way.
        if (visiblePages[1]->getCurrentDir() * visiblePages[2]->getCurrentDir() > 0 ) {
            if (visiblePages[2]->getCurrentDir() < 0) {
                // scroll textures to the right
                currentStartPage+=2;
                visiblePages[2]->giveAttributes( visiblePages[1] );
                visiblePages[2]->setDirection( 1 );
                textureManager->resetUsageFlags();

            } else {
                currentStartPage-=2;
                visiblePages[1]->giveAttributes( visiblePages[2] );
                visiblePages[1]->setDirection( -1 );
                textureManager->resetUsageFlags();
            }

        }
    }
}

void Book::render( QMatrix4x4 &projm ) {


    QMatrix4x4 transMatrix;
    transMatrix.setToIdentity();

    transMatrix.translate(viewX-0.15f, viewY,viewZ);

    QMatrix4x4 mat = projm * transMatrix;


    glDisable( GL_DEPTH_TEST );
    glDepthMask( GL_FALSE );

    // Only render the lower pages when they are needed.



    if (visiblePages[1]->isMoving()==true)
        visiblePages[0]->render( mat, 0, textureManager->getPageTexture(currentStartPage), visiblePages[1] );


    if (visiblePages[2]->isMoving()==true)
        visiblePages[3]->render( mat, textureManager->getPageTexture(currentStartPage+5), 0, visiblePages[2] );


    glEnable( GL_DEPTH_TEST );
    glDepthMask( GL_TRUE );


    visiblePages[1]->render( mat,
                             textureManager->getPageTexture(currentStartPage+1),
                             textureManager->getPageTexture(currentStartPage+2),0 );


    visiblePages[2]->render( mat,
                             textureManager->getPageTexture(currentStartPage+3),
                             textureManager->getPageTexture(currentStartPage+4),0 );

}
