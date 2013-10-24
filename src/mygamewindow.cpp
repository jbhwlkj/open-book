/**
 * Copyright (c) 2012 Nokia Corporation.
 */

#include "mygamewindow.h"

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <math.h>

#define BOOK_PAGE_WIDTH 1.25
#define BOOK_PAGE_HEIGHT 1.5

// controlling
#define ZOOM_GESTURE_R 0.15


const char* strTableFragmentShader =
        "uniform sampler2D sampler2d;\n"
        "varying mediump vec2 texCoord;\n"
        "void main (void)\n"
        "{\n"
        "    gl_FragColor = texture2D(sampler2d, texCoord);\n"
        "}";


const char* strTableVertexShader =
        "attribute highp vec3 vertex;\n"
        "attribute highp vec2 uv;\n"
        "uniform mediump mat4 projMatrix;\n"
        "varying mediump vec2 texCoord;\n"
        "void main(void)\n"
        "{\n"
        "   gl_Position = projMatrix * vec4( vertex,1 );\n"
        //"   gl_Position = vec4(vertex, 1);\n"
        "   texCoord = uv;\n"
        "}";


/*!
  Constructor.
  Just zero everything. The initialization is done later at initializeGL
*/
MyGameWindow::MyGameWindow(QWidget *parent /* = 0 */)
    : QGLWidget(parent)
{
#ifdef Q_OS_SYMBIAN
    lightsOnSaver = new QSystemScreenSaver( this );
    lightsOnSaver->setScreenSaverInhibit();
#endif

    currentAspectRatio = 1.0f;
    paused = true;
    appExiting = false;
    pullPossible = false;


    srand( QTime::currentTime().msec() + QTime::currentTime().second() );
    setAutoFillBackground( false );
    setAttribute (Qt::WA_OpaquePaintEvent);
    setAttribute( Qt::WA_NoSystemBackground );
    setAttribute( Qt::WA_NativeWindow );
    setAttribute( Qt::WA_PaintOnScreen, true );
    setAttribute( Qt::WA_StyledBackground,false);
    setAutoBufferSwap(false);

    mouseDown = false;


#ifdef MS_USE_TOUCH_EVENTS
    lastTouchCount = 0;
    setAttribute( Qt::WA_AcceptTouchEvents );
    for (int f=0; f<MAX_TOUCH_POINTS; f++) {
        touchEnabled[f] = false;
        exTouchEnabled[f] = false;
    }
    lastMultitouchDistance = 0.0f;
    lastTouchCount = 0;
#endif

    m_iaEngine = new IAEngine(this, QString("Test_vtest_OVI"));
    m_provider = new GalleryProvider(m_iaEngine);


    theBook = new BE3D::Book(BOOK_PAGE_WIDTH, BOOK_PAGE_HEIGHT, m_provider, this );

    // Startup the timer which will call updateGL as fast as it can.
    QObject::connect(&m_mainTimer, SIGNAL(timeout()), this, SLOT(updateGL()));
    //timer->start();

    // Connect'n'start the update timer
    QObject::connect(&m_slowTimer, SIGNAL(timeout()), this, SLOT(updateLogic()));
    m_slowTimer.start(1000);
}


/*!
  Destructor.
*/
MyGameWindow::~MyGameWindow()
{
    if (m_iaEngine) delete m_iaEngine;
    if (theBook) delete theBook;
    if (m_provider) delete m_provider;
}

void MyGameWindow::updateLogic() {
    if (m_iaEngine->getCapsuleCount()<20)
        m_iaEngine->requestAd();
}


bool MyGameWindow::eventFilter(QObject *object, QEvent *event) {
    // http://doc.trolltech.com/4.7/qevent.html#QEvent
    if (event->type() == QEvent::ActivationChange) {
        if (appExiting == false) {
            if (paused) paused=false; else paused=true;
            qDebug() << "Paused : " << paused;
            if (paused) m_mainTimer.stop(); else m_mainTimer.start();
            return false;
        }
    }
#ifdef MY_OS_MEEGO
    else if (event->type() == QEvent::WindowStateChange )
    {
        /*
        QWindowStateChangeEvent *change = static_cast<QWindowStateChangeEvent *>(event);

        Qt::WindowStates current = windowState();
        Qt::WindowStates old = change->oldState();
        //qDebug() << "widget is minimized in eventFilter?" << isMinimized();

        // did minimized flag change?
        if ((current ^ old) & Qt::WindowMinimized) {
            if (current & Qt::WindowMinimized) {
                // In MeeGo, the gl surface is somehow broken by the QMeegoGraphicsSystem.
                // This is why we must kill the timer, to prevent errors resulting gl painting.
                //qDebug() << "HIDING";
                killTimer(timerid);
                timerid = 0;
            }
        }
        return false;
        */
    }
#endif
    else {
        // Standard event processing
        return QObject::eventFilter(object, event);
    }
    return false;
}




void MyGameWindow::initializeGL()
{
    qDebug() << "Initializing gl..";
    glViewport(0,0, width(), height());

    tableTexture = glhelpLoadTexture(":/images/bg.jpg", false);
    exitButtonTexture = glhelpLoadTexture(":/images/exit_button.png", false);
    backButtonTexture = glhelpLoadTexture(":/images/back_button.png", false);

    // Build the table program
    QGLShader *vshader = new QGLShader(QGLShader::Vertex, this);
    if (vshader->compileSourceCode( strTableVertexShader ) == false)
        qDebug() << "Table: Failed to compile vertexshader.";
    QGLShader *fshader = new QGLShader(QGLShader::Fragment, this);
    if (fshader->compileSourceCode( strTableFragmentShader ) == false)
        qDebug() << "Table: Failed to compile fragmentshader.";

    program.addShader( vshader );
    program.addShader( fshader );

    program.bindAttributeLocation( "vertex", 0 );
    program.bindAttributeLocation( "uv", 1 );

    if (program.link() == false)
        qDebug() << "Table: Error linking the shaderprogram.";
    else
        qDebug() << "Table: Building the shaderprogram finished.";


    if (theBook) theBook->initializeGL();
}


/*!
    From GameWindow
    \see GameWindow.cpp
*/
void MyGameWindow::onDestroy()
{
    glDeleteTextures( 1, &tableTexture );
    glDeleteTextures( 1, &exitButtonTexture );
    glDeleteTextures( 1, &backButtonTexture );
    program.release();
    if (theBook) theBook->destroyGL();
}


#ifdef MS_USE_TOUCH_EVENTS
bool MyGameWindow::event(QEvent* event)
{
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    {
        memcpy( exTouchEnabled, touchEnabled, sizeof(bool) * MAX_TOUCH_POINTS );
        QList<QTouchEvent::TouchPoint> touchPoints = static_cast<QTouchEvent *>(event)->touchPoints();
        int activePoints = touchPoints.count();


        foreach (const QTouchEvent::TouchPoint &touchPoint, touchPoints)
        {
            int ind = touchPoint.id();

#ifdef Q_WS_MAEMO_6
            QPoint pte = touchPoint.pos().toPoint();
            QPoint p = QPoint( height() - pte.y(), pte.x());
#else
            QPoint p = touchPoint.pos().toPoint();
#endif
            touchPositions[ind][0] = p.x();
            touchPositions[ind][1] = p.y();

            switch (touchPoint.state()) {
            case Qt::TouchPointPressed:
            {
                if (ind==0 && activePoints<2) {
                    pressPos[0] = (float)p.x() / (float)currentWidth;
                    pressPos[1] = (float)(height() - p.y()) / (float)currentHeight;
                    if ( !checkUIButtons( pressPos[0], pressPos[1] ) ) {
                        pullPossible = false;
                        mouseDown = true;
                        pressPos[0] = (float)p.x() / (float)currentWidth;
                        pressPos[1] = (float)(height() - p.y()) / (float)currentHeight;
                        mouseDownPos[0] = pressPos[0];
                        mouseDownPos[1] = pressPos[1];
                        mousePrevDeltaVector[0] = pressPos[0] - mouseDownPos[0];
                        mousePrevDeltaVector[1] = pressPos[1] - mouseDownPos[1];
                    }
                }
                touchEnabled[ind] = true;
                continue;
            }
            case Qt::TouchPointMoved:
            {
                if (ind==0 && activePoints<2) {
                    pressPos[0] = (float)p.x() / (float)currentWidth;
                    pressPos[1] = (float)(height() - p.y()) / (float)currentHeight;
                }
                touchEnabled[ind] = true;
                continue;
            }
            case Qt::TouchPointReleased:
            {
                if (ind==0 && activePoints<2) {
                    if (pullPossible == false) {
                            // Was a click
                        click( pressPos[0], pressPos[1] );
                    }
                    mouseDown = false;
                    pullPossible = false;
                }

                touchEnabled[ind] = false;
                continue;
            }
            default:
            case Qt::TouchPointStationary:
            {
                // The touch point did NOT move
                continue;
            }
            }
        }
        event->accept();

        // zoom in/out gesture..
        if (activePoints>1) {
            mouseDown = false;
            //if (theGame) theGame->cancelPress();

            float avgtouchx = 0.0f;
            float avgtouchy = 0.0f;
            for (int f=0; f<activePoints; f++) {
                avgtouchx+=touchPositions[f][0];
                avgtouchy+=touchPositions[f][1];
            }
            avgtouchx/=(float)activePoints;
            avgtouchy/=(float)activePoints;

            float collen = 0.0f;
            for (int f=1; f<activePoints; f++) {
                float dx = (touchPositions[f][0] - touchPositions[0][0]) / (float)currentWidth;
                float dy = (touchPositions[f][1] - touchPositions[0][1]) / (float)currentHeight;
                float l = sqrtf( dx*dx+dy*dy );
                collen += l;
            }

            if (lastTouchCount<2) {
                lastMultitouchDistance = collen;
            }

            // calculate the touch average as well to know which page should be zoomed in to.

            if ((lastMultitouchDistance - collen) > ZOOM_GESTURE_R) {
                // zoom out
                qDebug() << "Zoomout gesture";
                theBook->zoomOut();
            }



            if ((lastMultitouchDistance - collen) < -ZOOM_GESTURE_R) {
                // zoom in
                qDebug() << "Zoomin gesture";
                theBook->zoomIn();
            }
        }



        lastTouchCount = activePoints;
        return true;
    }

    default:
        return QWidget::event(event);
    }

}

#else
void MyGameWindow::mousePressEvent(QMouseEvent *e) {
    float px = (float)e->x() / (float)width();
    float py = (float)(height() - e->y()) / (float)height();
    if (checkUIButtons(px,py) == true) return;
    mouseDown = true;
    pullPossible = false;
    pressPos[0] = px;
    pressPos[1] = py;
    mouseDownPos[0] = pressPos[0];
    mouseDownPos[1] = pressPos[1];
    mousePrevDeltaVector[0] = pressPos[0] - mouseDownPos[0];
    mousePrevDeltaVector[1] = pressPos[1] - mouseDownPos[1];

}

void MyGameWindow::mouseMoveEvent(QMouseEvent *e) {
    if (!mouseDown) return;
    pressPos[0] = (float)e->x() / (float)width();
    pressPos[1] = (float)(height() - e->y()) / (float)height();
}

void MyGameWindow::mouseReleaseEvent(QMouseEvent *e) {
    pressPos[0] = (float)e->x() / (float)width();
    pressPos[1] = (float)(height() - e->y()) / (float)height();
    if (pullPossible == false) {
        click( pressPos[0], pressPos[1] );
    }
    pullPossible = false;
    mouseDown = false;
}
#endif


void MyGameWindow::click( float x, float y ) {
    theBook->click( x, y );
}

bool MyGameWindow::checkUIButtons( float px, float py ) {
    if (px>0.85f && py>0.85f) {
                // Exit
        qDebug() << "Exit/Back button pressed.";
        qApp->exit(0);
        return true;
    }
    return false;
}

void MyGameWindow::renderQuad( float middleX, float middleY, float middleZ, float xr, float yr, GLuint texture ) {
    glBindTexture( GL_TEXTURE_2D, texture );
    glDisable( GL_CULL_FACE );


    GLfloat quad_vertices[] = { middleX-xr,middleY-yr,middleZ,0,1,
                                middleX-xr,middleY+yr,middleZ,0,0,
                                middleX+xr,middleY-yr,middleZ,1,1,
                                middleX+xr,middleY+yr,middleZ,1,0
                              };

    program.setAttributeArray( 0, (GLfloat*)quad_vertices, 3, sizeof( GLfloat) * 5 );         // Vertices
    program.setAttributeArray( 1, (GLfloat*)(quad_vertices+3), 2, sizeof( GLfloat) * 5 );     // UV coordinates

    glDrawArrays(GL_TRIANGLE_STRIP,0,4);
}


void MyGameWindow::paintGL()
{
    QTime time = QTime::currentTime();
    int msecs = time.msecsTo( prevtime );
    prevtime = time;

    // update here
    float frameDelta = fabs( (float)msecs/1000.0f );
    if (frameDelta>0.05f) frameDelta = 0.05f;
    if (frameDelta<0.0001f) frameDelta = 0.0001f;

    if (theBook) theBook->update( frameDelta, currentAspectRatio );



    glClearColor(0,0,0,0);
    // NOTE,  colorclear can be removed when the background works.
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glDisable( GL_BLEND );
    glEnable( GL_DEPTH_TEST );



    // Render the bg
    program.bind();
    program.enableAttributeArray(0);
    program.enableAttributeArray(1);

    program.setUniformValue( program.uniformLocation("sampler2d"), 0);
    program.setUniformValue( program.uniformLocation("projMatrix"), projectionMatrix );

    glDepthMask( GL_FALSE );
    glDisable( GL_DEPTH_TEST );
    renderQuad( 0,0,-2.2f, currentAspectRatio,1, tableTexture );

    glEnable( GL_BLEND );
    // Exit / Back
    float size = 0.15f;
    renderQuad( currentAspectRatio-size/2.0f,
                1.0f-size/2.0f,
                -2.6f, size,size, exitButtonTexture );

    glDisable( GL_BLEND );
    glDepthMask( GL_TRUE );
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_CULL_FACE );






    if (theBook) {
        if (mouseDown
        #ifdef MS_USE_TOUCH_EVENTS
                && lastTouchCount<2
        #endif
                ) {
            float dx = pressPos[0]-mouseDownPos[0];
            float dy = pressPos[1]-mouseDownPos[1];

            if (pullPossible == false && sqrtf(dx*dx+dy*dy)>0.1f) {
                pullPossible = true;
            }


            // detect too large changes
            // This is because some Symbian^3
            // based devices mix the axis information
            // of different touchpoints sometimes
            // causing unstable movement vectors which would
            // break the page.
            float ddx = (dx-mousePrevDeltaVector[0]);
            float ddy = (dy-mousePrevDeltaVector[1]);
            float changeAmount = ddx*ddx+ddy*ddy;

            if (changeAmount > 10.0f) {
                theBook->disablePull(false);
                pullPossible = false;
            } else {
                if (pullPossible) {
                    theBook->setPull( pressPos[0],pressPos[1],
                                      (float)(dx)*1.7f,
                                      (float)(dy)*1.7f,
                                      currentAspectRatio );
                }
            }

            mousePrevDeltaVector[0] = dx;
            mousePrevDeltaVector[0] = dy;

        } else theBook->disablePull(false);

        theBook->render(projectionMatrix);
    }

    swapBuffers();
}



/*!
  The size of our window has been changed. Set new projectionmatrix..
*/
void MyGameWindow::resizeGL(int w, int h)
{
    glViewport(0,0, w,h );
    currentWidth = w;
    currentHeight = h;
    currentAspectRatio = (float)currentWidth / (float)currentHeight;
    projectionMatrix.setToIdentity();
    projectionMatrix.perspective(45.0f, currentAspectRatio, 0.1f, 3.5f );
    qDebug() << "ResizeGL: " << w << "," << h;
}



/*!
  Load an image with QImage, convert it to GL's format, create OpenGL texture and
  Return it's handle
*/
GLuint MyGameWindow::glhelpLoadTexture( QString name, bool mipmaps )
{
    GLuint handle = 0;
    QImage texture;
    if(texture.load(name))
    {
        // bindTexture has flipped y with default bind options
        if (mipmaps)
            handle=bindTexture(texture, GL_TEXTURE_2D, GL_RGBA, QGLContext::LinearFilteringBindOption | QGLContext::MipmapBindOption);
        else
            handle=bindTexture(texture, GL_TEXTURE_2D, GL_RGBA, QGLContext::LinearFilteringBindOption);
    }
    else
    {
        qDebug("load texture failed");
    }
    qDebug("loadGLTexture ret %d", handle);
    return handle;
}


