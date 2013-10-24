/**
 * Copyright (c) 2012 Nokia Corporation.
 */

#ifndef __JPEG_THUMBNAILFETCHER__
#define __JPEG_THUMBNAILFETCHER__

#include <QtCore/QIODevice>
#include <QtGui/QImage>

class JpegThumbnailFetcher
{
public:
    JpegThumbnailFetcher() {}
    ~JpegThumbnailFetcher() {}

    static QImage fetchThumbnail(QIODevice &jpegFile);
    static QImage fetchThumbnail(QString filePath);

protected:
    static bool readWord(QIODevice &sdevice, unsigned short *target, bool invert = true);
    static bool exifScanloop(QIODevice &jpegFile, unsigned int &tnOffset, unsigned int &tnLength);
};

#endif
