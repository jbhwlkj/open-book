/**
 * Copyright (c) 2012-2014 Microsoft Mobile.
 */

#include "jpegthumbnailfetcher.h"

#include <QtCore/QBuffer>
#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtGui/QImageReader>

    // Exif defines
#define JPEG_SOI 0xffd8
#define JPEG_SOS 0xffda
#define JPEG_EOI 0xffd9
#define JPEG_APP1 0xffe1


/*!
  \class JpegThumbnailFetcher
  \brief Seeks through jpeg metadata for precalculated thumbnails for fast
         previews of an image.
*/

/*!
  Reads a word from a stream and inverts it if \a invert is true.
*/
bool JpegThumbnailFetcher::readWord(QIODevice &sdevice,
                                    unsigned short *target,
                                    bool invert)
{
    unsigned short t;

    if (sdevice.read((char*)&t, 2) != 2)
        return false;

    if (invert)
        *target = ((t&255) << 8) | ((t>>8)&255);
    else
        *target = t;

    return true;
}


/*!
  Scans though exif chunks, finds the app1 chunk and processes it.
*/
bool JpegThumbnailFetcher::exifScanloop(QIODevice &jpegFile,
                                        unsigned int &tnOffset,
                                        unsigned int &tnLength)
{
    // Loop through tags
    while (1) {
        unsigned short tagid, tagLength;

        if (!readWord(jpegFile, &tagid))
            return 0;

        if (tagid == JPEG_EOI || tagid == JPEG_SOS) {
                // Data ends
            break;
        }

        if (!readWord(jpegFile, &tagLength))
            return 0;

        if (tagid == JPEG_APP1) {
            char str[6];
            jpegFile.read(str,6);

            // Store the current position for offset calculation
            int basepos = jpegFile.pos();

            // read tiff - header
            unsigned short tifhead[2];

            for (int h = 0; h < 2; h++) {
                if (!readWord(jpegFile, &tifhead[h]))
                    return false;
            }

            if (tifhead[0] != 0x4949) {
                //qDebug() << "Invalid byte order";
                return false;
            }

            while (1) {
                unsigned int offset;
                jpegFile.read((char*)&offset, 4);

                if (offset == 0)
                    break;

                jpegFile.seek(basepos + offset);

                unsigned short fields;

                if (!readWord(jpegFile, &fields, false))
                    return false;

                while (fields > 0) {
                    char ifdentry[12];
                    jpegFile.read(ifdentry, 12);

                    unsigned short tagnumber =
                            (((unsigned short)ifdentry[0])
                             | (unsigned short)ifdentry[1]<<8);

                    // Offset of the thumbnaildata
                    if (tagnumber == 0x0201) {
                        memcpy(&tnOffset, ifdentry+8, 4);
                        tnOffset += basepos;
                    }
                    else { // Length of the thumbnaildata
                        if (tagnumber == 0x0202) {
                            memcpy(&tnLength, ifdentry+8, 4);
                        }
                    }

                    fields--;

                    if (tnOffset != 0 && tnLength!=0)
                        return true;
                }
            }

            return false;
        }

        jpegFile.seek(jpegFile.pos() + tagLength - 2);
    }

    return false;
}


QImage JpegThumbnailFetcher::fetchThumbnail(QIODevice &jpegFile)
{
    QImage empty;

    if (!jpegFile.open(QIODevice::ReadOnly))
        return empty;

    unsigned short jpegId;

    if (!readWord(jpegFile, &jpegId))
        return empty;

    if (jpegId!= JPEG_SOI) {
        // JPEG SOI must be here
        return empty;
    }

    unsigned int tnOffset = 0;
    unsigned int tnLength = 0;

    if (exifScanloop(jpegFile, tnOffset, tnLength)) {
        // Goto the thumbnail offset in the file
        jpegFile.seek( tnOffset );

        // Use image reader to decode jpeg-encoded thumbnail
        QByteArray tnArray = jpegFile.read(tnLength);
        QBuffer buf(&tnArray, 0);
        QImageReader reader(&buf);
        reader.setAutoDetectImageFormat(false);
        reader.setFormat("jpg");
        return reader.read();
    }

    return empty;
}


QImage JpegThumbnailFetcher::fetchThumbnail(QString filePath)
{
    QFile jpegFile(filePath);
    return fetchThumbnail(jpegFile);
}

