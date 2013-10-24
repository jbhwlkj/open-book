/**
 * Copyright (c) 2012 Nokia Corporation.
 */

#ifndef MYGAMEWINDOW_H
#define MYGAMEWINDOW_H

#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtGui/QImage>
#include <QtGui/QMouseEvent>
#include <QtGui/QStaticText>
#include <QtGui/QVector2D>
#include <QtOpenGL/QGLWidget>

#include "book.h"
#include "galleryprovider.h"
#include "iaengine.h"

#ifdef Q_OS_SYMBIAN
#include <QSystemScreenSaver>
QTM_USE_NAMESPACE
#endif

#ifdef Q_OS_SYMBIAN
#define MS_USE_TOUCH_EVENTS
#define MAX_TOUCH_POINTS 5
#endif

#ifdef Q_WS_MAEMO_6
#define MS_USE_TOUCH_EVENTS
#define MAX_TOUCH_POINTS 5
#endif


class MyGameWindow : public QGLWidget
{
    Q_OBJECT

public:
    explicit MyGameWindow(QWidget *parent = 0);
    virtual ~MyGameWindow();

private slots:
    void updateLogic();


protected: // QGLWidget
    void resizeGL(int width, int height);
    void initializeGL();
    void paintGL();
    void onDestroy();

    void renderQuad( float middleX, float middleY, float middleZ, float xr, float yr, GLuint texture );

#ifdef MS_USE_TOUCH_EVENTS
    bool event(QEvent*);
    float touchPositions[MAX_TOUCH_POINTS][2];
    bool touchEnabled[MAX_TOUCH_POINTS];
    bool exTouchEnabled[MAX_TOUCH_POINTS];
    float lastMultitouchDistance;
    int lastTouchCount;
#else
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
#endif

    bool checkUIButtons( float px, float py );
    void click( float x, float y );

    BE3D::Book *theBook;
    IAEngine *m_iaEngine;
    GalleryProvider *m_provider;
    QTimer m_mainTimer;
    QTimer m_slowTimer;

    bool eventFilter(QObject *, QEvent *);

    bool appExiting;
    bool paused;
#ifdef Q_OS_SYMBIAN
    QSystemScreenSaver *lightsOnSaver;
#endif

    bool mouseDown;
    float pressPos[2];
    float mouseDownPos[2];
    float mousePrevDeltaVector[2];
    bool pullPossible;

    GLuint tableTexture;            // texture for the table
    GLuint exitButtonTexture;
    GLuint backButtonTexture;

    QGLShaderProgram program;

    QTime lastRaiseTime;

    QTime prevtime;
    GLuint glhelpLoadTexture( QString imageFile, bool mipmaps = true );

        // Following are set by onSizeChanged event
    int currentWidth;
    int currentHeight;
    float currentAspectRatio;
    QMatrix4x4 projectionMatrix;

};

#endif // MYGAMEWINDOW_H
