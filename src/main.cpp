/**
 * Copyright (c) 2012 Nokia Corporation.
 */

#ifndef __GAMEENABLEREXAMPLEMAIN__
#define __GAMEENABLEREXAMPLEMAIN__

#include <QtGui/QApplication>
#include <QtGui/QPaintEngine>

#include "mygamewindow.h"

#ifdef Q_OS_SYMBIAN
    #include <eikenv.h>
    #include <eikappui.h>
    #include <aknenv.h>
    #include <aknappui.h>
#endif

#ifdef Q_WS_MAEMO_6
#include <QX11Info>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

enum MyOrientation
{
    Landscape = 0,
    Portrait = 270,
    LandscapeInverted = 180,
    PortraitInverted = 90
};


static void writeX11OrientationAngleProperty(QWidget* widget, MyOrientation orientation = Portrait)
{
    if (widget) {
        WId id = widget->winId();
        Display *display = QX11Info::display();
        if (!display) return;
        Atom orientationAngleAtom = XInternAtom(display, "_MEEGOTOUCH_ORIENTATION_ANGLE", False);
        XChangeProperty(display, id, orientationAngleAtom, XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&orientation, 1);
    }
}
#endif


Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Lock orientation to landscape
#ifdef Q_OS_SYMBIAN
    CAknAppUi* appUi = dynamic_cast<CAknAppUi*>(CEikonEnv::Static()->AppUi());
    TRAPD(error,
    if (appUi) {
        appUi->SetOrientationL(CAknAppUi::EAppUiOrientationLandscape);
    }
    );
#endif

    MyGameWindow theGameWindow;

    theGameWindow.setWindowState(Qt::WindowNoState);
    a.installEventFilter( &theGameWindow );

#ifdef Q_WS_MAEMO_6
    writeX11OrientationAngleProperty(&theGameWindow);
#endif

#ifdef Q_OS_WIN32
    theGameWindow.setGeometry(0,0, 640*2, 360*2 );
    theGameWindow.show();
#else
    theGameWindow.showFullScreen();
#endif

    return a.exec();
}


#endif /* __GAMEENABLEREXAMPLEMAIN__ */
