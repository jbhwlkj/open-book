# Copyright (c) 2012 Nokia Corporation.

QT += core gui opengl network

TARGET = OpenBook
TEMPLATE = app
VERSION = 1.0

INCLUDEPATH += src inneraddengine_src

HEADERS += \
    inneraddengine_src/iacapsule.h \
    inneraddengine_src/iaengine.h \
    inneraddengine_src/xmle.h \
    src/book.h \
    src/bookpage.h \
    src/galleryprovider.h \
    src/jpegthumbnailfetcher.h \
    src/mygamewindow.h \
    src/pagerenderer.h \
    src/provider.h \
    src/texturemanager.h

SOURCES += \
    inneraddengine_src/iaengine.cpp \
    inneraddengine_src/xmle.cpp \
    src/book.cpp \
    src/bookpage.cpp \
    src/galleryprovider.cpp \
    src/jpegthumbnailfetcher.cpp \
    src/main.cpp \
    src/mygamewindow.cpp \
    src/pagerenderer.cpp \
    src/texturemanager.cpp

RESOURCES += openbook.qrc

symbian {
    CONFIG += mobility
    MOBILITY += systeminfo

    TARGET.UID3 = 0xee69dfaa
    TARGET.EPOCSTACKSIZE = 0x14000
    TARGET.EPOCHEAPSIZE = 0x020000 0x800000

    # For orientation locking.
    LIBS += -lcone -leikcore -lavkon

    ICON = images/openbook.svg

    # For enabling the hardware floats
    MMP_RULES += "OPTION gcce -march=armv6"
    MMP_RULES += "OPTION gcce -mfpu=vfp"
    MMP_RULES += "OPTION gcce -mfloat-abi=softfp"
    MMP_RULES += "OPTION gcce -marm"
}
