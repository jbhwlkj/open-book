/**
 * Copyright (c) 2012 Nokia Corporation.
 */

#include "bookpage.h"

#include <QtCore/QDebug>
#include <QtOpenGL/QGLShader>
#include <math.h>

using namespace BE3D;

/*!
  \class BookPage
  \brief Page object of a book. Manages a single page including all of its
         events and turning.
*/

BookPage::BookPage( float zb, float xs, float ys, PageRenderer *r, QObject *parent ) : QObject(parent) {
    renderer = r;
    xscale = xs;
    yscale = ys;
    zbase = zb;
    exPullDir2DLength = 0;
    pullDir2DLength = 0;
    currentPageDir = 1.0f;
    mainVector = QVector3D( currentPageDir,0,0 );
    pullPower = 0.0f;
    pulling = false;
    incing = false;
    wasMoving = false;
    expulling = false;
    vertices = new GLfloat[ r->getGridWidth()*r->getGridHeight() * 8];
    refreshGeometry = true;
}

BookPage::~BookPage() {
    if (vertices) delete [] vertices;
}


void BookPage::setDirection( float set ) {
    currentPageDir = set;
    mainVector = QVector3D( currentPageDir,0,0 );
    secondVector = mainVector;
    pageTurn = 0;
    pullDirX = 0;
    pullDirY = 0;
    refreshGeometry = true;
}

#define MOVING_EPSILON 0.0003f

bool BookPage::isMoving() {
    if ( fabsf(mainVector.x()-currentPageDir) < MOVING_EPSILON &&
         fabsf(secondVector.x()-currentPageDir) < MOVING_EPSILON &&
         pulling==false &&
         incing==false) return false;
    return true;
}

void BookPage::giveAttributes( BookPage *to ) {

    to->pullDir2DLength = pullDir2DLength;
    to->exPullDir2DLength = pullDir2DLength;
    to->mainVector = mainVector;
    to->secondVector = secondVector;

    to->currentPageDir = currentPageDir;
    to->pageTurn = pageTurn;
    to->dividerPosX = dividerPosX;
    to->dividerPosY = dividerPosY;

    to->pullDirInc[0] = pullDirInc[0];
    to->pullDirInc[1] = pullDirInc[1];
    to->pullSourceX = pullSourceX;
    to->pullSourceY = pullSourceY;
    to->pullDirNormX = pullDirNormX;
    to->pullDirNormY = pullDirNormY;
    to->pullDirX = pullDirX;
    to->pullDirY = pullDirY;

    to->refreshGeometry = true;
    to->pullFromCenter = pullFromCenter;
    refreshGeometry = true;

}

void BookPage::update( const float timeStep ) {
    bool moving = isMoving();

    if (pulling==false) {
        if (!incing && expulling==true) {
            if (pageTurn>0.2) currentPageDir *= -1.0f;
        }

        if (!incing) {
            secondVector += QVector3D( currentPageDir, 0, 0)* timeStep*12.0f;
            secondVector.normalize();
            mainVector += secondVector*timeStep*4.0f;
            mainVector.normalize();

        } else {
            pullDirX += pullDirInc[0] * timeStep;
            pullDirY += pullDirInc[1] * timeStep;
            pullDirInc[0] -= pullDirInc[0] * timeStep*2.0f;
            pullDirInc[1] -= pullDirInc[1] * timeStep*2.0f;
            pullDirInc[0] -= pullDirX * timeStep*5.0f;
            pullDirInc[1] -= pullDirY * timeStep*5.0f;



            if ((pullDir2DLength-exPullDir2DLength)<0.007f || pageTurn>0.95f) {
                // check turn here.
                if ((pullFromCenter==false && pageTurn>0.1f) || (pullFromCenter==true && pageTurn>0.8f)) {
                    currentPageDir *= -1.0f;
                    incing = false;
                }

                if (pullDir2DLength<0.02f) {
                    incing = false;
                    pulling = false;
                    pullDirX = 0;
                    pullDirY = 0;
                    secondVector = QVector3D( currentPageDir, 0, 0);
                    mainVector = secondVector;
                }
            }
        }
    }

    if (pulling || incing) {

        if (pullFromCenter == false && pullDirX<0) {

            dividerPosX = pullSourceX + pullDirX;
            dividerPosY = pullSourceY + pullDirY;

            QVector3D todiv = QVector3D( dividerPosX, dividerPosY - 0.5f, 0);
            QVector3D tosource = QVector3D( pullSourceX, pullSourceY - 0.5f, 0);
            if (todiv.length()>tosource.length()) {
                //todiv.
                todiv.normalize();
                todiv *= tosource.length();
                dividerPosX = todiv.x();
                dividerPosY = todiv.y();
            }



            pullDirX = dividerPosX - pullSourceX;
            pullDirY = dividerPosY - pullSourceY;


            //pullDirX

            //float pulllength = sqrtf( pullDirX * pullDirX + pullDirY * pullDirY );

            // dividerpos limitations -> dividerdir can be different from pulldir's normal



            float pullnorm[2];
            pullnorm[0] = -pullDirY;
            pullnorm[1] = pullDirX;


            float divpos1[2];
            float divpos2[2];
            pageTurn = 1.0f;

            float steps = dividerPosY / -pullnorm[1];
            divpos1[0] = dividerPosX + steps * pullnorm[0];
            divpos1[1] = dividerPosY + steps * pullnorm[1];
            if (-(divpos1[0]-0.3f)*3.0f<pageTurn) pageTurn = -(divpos1[0]-0.3f)*3.0f;


            if (divpos1[0] < 0.0f) { divpos1[0] = 0.0f; divpos1[1] = 0.0f; }
            if (divpos1[0]>1.0f) {
                steps = (divpos1[0]-1.0f) / pullnorm[0];
                divpos1[1] -= steps * pullnorm[1];
                divpos1[0] -= steps * pullnorm[0];
            }




            steps = (1.0f-dividerPosY) / pullnorm[1];
            divpos2[0] = dividerPosX + steps * pullnorm[0];
            divpos2[1] = dividerPosY + steps * pullnorm[1];
            if (-(divpos2[0]-0.3f)*3.0f<pageTurn) pageTurn = -(divpos2[0]-0.3f)*3.0f;


            if (divpos2[0] < 0.0f) { divpos2[1] = 1.0f; divpos2[0] = 0.0f; }
            if (divpos2[0]>1.0f) {
                steps = (divpos2[0]-1.0f) / -pullnorm[0];
                divpos2[1] -= steps * -pullnorm[1];
                divpos2[0] -= steps * -pullnorm[0];
            }
            if (pageTurn<0) pageTurn = 0;


                // correct,.. new dir is still required.
            dividerPosX = (divpos1[0] + divpos2[0])/2.0f;
            dividerPosY = (divpos1[1] + divpos2[1])/2.0f;

            pullDirNormY = -(divpos1[0] - divpos2[0]);
            pullDirNormX = (divpos1[1] - divpos2[1]);
            float nlength = sqrtf( pullDirNormX*pullDirNormX + pullDirNormY * pullDirNormY );
            if (nlength==0.0f) nlength = 1.0f;
            pullDirNormX /= nlength;
            pullDirNormY /= nlength;
        } else {
            dividerPosX = pullSourceX;
            dividerPosY = pullSourceY;

            pullDirNormX = pullDirX;
            pullDirNormY = -pullDirY;
        }



        pullDir2DLength = fabsf( pullDirX );
        if (pullDirX>0) pullDir2DLength = 0;
        exPullDir2DLength = pullDir2DLength;

        if (pullFromCenter) {
            pageTurn = pullDir2DLength;
            if (pageTurn<0) pageTurn = 0;
        } else {
        }
        if (pageTurn<0) pageTurn = 0;
        if (pageTurn>=0.99f) pageTurn = 0.99f;



        float pageAngle = pageTurn * 3.14159f;

        mainVector = QVector3D( currentPageDir*cosf( pageAngle), 0, sinf( pageAngle ));
        if (pullFromCenter==false) {
            secondVector = QVector3D( currentPageDir*pullDirNormX*(1-pageTurn),pullDirNormY*(1-pageTurn),(1-pageTurn)*0.1f );
            secondVector.normalize();
        } else {
            float negpower = (pageTurn * 1.8f) * (1.0f - pageTurn*pageTurn);
            if (negpower>1.0f) negpower = 1.0f;
            secondVector = QVector3D( currentPageDir*cosf( pageAngle - negpower * 3.14159f*0.9f), 0, sinf( pageAngle - negpower * 3.14159f * 0.9f ));
        }
    }


    expulling = pulling;

        // Does the geometry need refreshing
    if (moving) refreshGeometry = true;

    if (wasMoving==true && moving==false) {
            // Reset position
        setDirection( currentPageDir );
        refreshGeometry = true;
    }
    wasMoving = moving;
}


void BookPage::recalculateGeometry() {

    int x,y;
    float bx,by;
    float ftemp;


    float xitem_length = 1.0f/(renderer->getGridWidth()-1) * xscale;
    float turn_mul = xitem_length;


    GLfloat *t = vertices;
    for (y=0; y<renderer->getGridHeight(); y++) {
        by = (float)y / (float)(renderer->getGridHeight()-1);
        QVector3D curDir = QVector3D(0,0,1);
        QVector3D curPos = QVector3D(  0, -0.5*yscale + by * yscale, zbase );

        for (x=0; x<renderer->getGridWidth(); x++) {
            bx = (float)x / (float)(renderer->getGridWidth()-1);

            // Uv
            t[3] = bx;
            t[4] = 1.0f-by;

            ftemp = (bx - dividerPosX)*pullDirNormX + (by - dividerPosY)*pullDirNormY;

            t[0] = curPos.x();
            t[1] = curPos.y();
            t[2] = curPos.z();

            if (pullFromCenter) {
                ftemp =  0.4f-ftemp*3.0f;
                //ftemp =  1.0f-fabsf(ftemp)*4.0f;
                if (ftemp<0.0f) ftemp = 0.0f;
                if (ftemp>1.0f) ftemp = 1.0f;
            } else {
                ftemp =  -ftemp*4.0f;
                if (ftemp<0.0f) ftemp = 0.0f;
                if (ftemp>1.0f) ftemp = 1.0f;
            }

            curDir+=secondVector*ftemp*turn_mul + mainVector*(1-ftemp)*turn_mul;
            curDir.normalize();
            curDir *= xitem_length;
            curPos += curDir;

                // Normal
            t[5] = 0;
            t[6] = 0;
            t[7] = 0;
            t+=8;
        }
    }



    // Calculate the normals
    QVector3D tv1,tv2;
    int i1,i2,i3,i4;

    for (y=0; y<renderer->getGridHeight()-1; y++)
        for (x=0; x<renderer->getGridWidth()-1; x++) {
            i1 = (y*renderer->getGridWidth()+x);
            i2 = (i1+1);
            i3 = (i1+renderer->getGridWidth());
            i4 = (i2+renderer->getGridWidth());

            i1 *= 8;
            i2 *= 8;
            i3 *= 8;
            i4 *= 8;

            tv1 = QVector3D( vertices[i2] - vertices[i1],
                             vertices[i2+1] - vertices[i1+1],
                             vertices[i2+2] - vertices[i1+2] );
            tv1.normalize();

            tv2 = QVector3D( vertices[i3] - vertices[i1],
                             vertices[i3+1] - vertices[i1+1],
                             vertices[i3+2] - vertices[i1+2] );
            tv2.normalize();

            tv1 = QVector3D::crossProduct( tv1, tv2 );
          // NOTE, check the normalizations.


            // distribute
            vertices[i1+5] += tv1.x();
            vertices[i2+5] += tv1.x();
            vertices[i3+5] += tv1.x();
            vertices[i4+5] += tv1.x();

            vertices[i1+6] += tv1.y();
            vertices[i2+6] += tv1.y();
            vertices[i3+6] += tv1.y();
            vertices[i4+6] += tv1.y();

            vertices[i1+7] += tv1.z();
            vertices[i2+7] += tv1.z();
            vertices[i3+7] += tv1.z();
            vertices[i4+7] += tv1.z();
        }

    // Normalize the vertex normals
    float len;
    t=vertices;
    for (int i=0; i<renderer->getGridWidth() * renderer->getGridHeight(); i++) {
        len = sqrtf( t[5]*5[t]+t[6]*t[6]+t[7]*t[7] );
        if (len==0.0f) len = 0.0f; else len = 1.0f/len;
        t[5] *= -len;
        t[6] *= -len;
        t[7] *= -len;
        t+=8;
    }

}


void BookPage::render(QMatrix4x4 &proj, GLuint frontTexture, GLuint backTexture, BookPage *topPage) {

    if (frontTexture==0 && backTexture==0) return;      // No textures, no rendering

    if (refreshGeometry) {
        recalculateGeometry();
        refreshGeometry = false;
    }

    glEnable( GL_CULL_FACE );
    renderer->getProgram().bind();
    renderer->getIndexBuffer()->bind();

    renderer->getProgram().enableAttributeArray(0);
    renderer->getProgram().enableAttributeArray(1);
    renderer->getProgram().enableAttributeArray(2);
    renderer->getProgram().setAttributeArray( 0, (GLfloat*)vertices, 3, sizeof( GLfloat) * 8 );
    renderer->getProgram().setAttributeArray( 1, (GLfloat*)(vertices+3), 2, sizeof( GLfloat) * 8 );
    renderer->getProgram().setAttributeArray( 2, (GLfloat*)(vertices+5), 3, sizeof( GLfloat) * 8 );


    if (topPage) {
        float shadowpower = (0.2f - topPage->pageTurn)*5.0f;
        if (shadowpower<0) shadowpower = 0;

        renderer->getProgram().setUniformValue( renderer->getShadowPowerLocation(), shadowpower );
        renderer->getProgram().setUniformValue( renderer->getDividerMatLocation(),
                                                QVector4D( topPage->dividerPosX, topPage->dividerPosY,
                                                           topPage->pullDirNormX, topPage->pullDirNormY ));
    } else {
            // disable -> no effect
        renderer->getProgram().setUniformValue( renderer->getShadowPowerLocation(), 0.0f );
    }


    renderer->getProgram().setUniformValue( renderer->getProjmLocation(), proj );
    renderer->getProgram().setUniformValue( renderer->getSamplerLocation(), 0 );

        // Render frontside
    if (frontTexture!=0) {
        glCullFace( GL_BACK );
        glBindTexture( GL_TEXTURE_2D,frontTexture );
        renderer->getProgram().setUniformValue( "backsidemul", 1.0f);
        glDrawElements( GL_TRIANGLES, renderer->getIndexCount(), GL_UNSIGNED_SHORT, 0);
    }


        // Render backside
    if (backTexture!=0) {
        glCullFace( GL_FRONT );
        glBindTexture( GL_TEXTURE_2D,backTexture );
        renderer->getProgram().setUniformValue( "backsidemul", -1.0f);
        glDrawElements( GL_TRIANGLES, renderer->getIndexCount(), GL_UNSIGNED_SHORT, 0);
    }


}
