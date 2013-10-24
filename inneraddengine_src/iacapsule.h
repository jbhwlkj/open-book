/**
 * Copyright (c) 2012 Nokia Corporation.
 */

#ifndef __IACAPSULE__
#define __IACAPSULE__

#include <QtCore/QString>
#include <QtGui/QImage>


/*!
  \class IACapsule
  \brief Implementation of Inneractives HTTP interface for fetching ads.
*/

class IACapsule
{
public:
    IACapsule()
        : m_next(0),
          m_failed(false)
    {}

    virtual ~IACapsule() {}

public: // Data
    IACapsule *m_next;  // Pointer to next Ad. Just for listing
    QString text;       // Text string associated with this Ad
    QString targetUrl;  // Url to launch when this Ad is "clicked"
    QString imageUrl;   // Url to image (banner) of this Ad
    QImage m_image;     // Fetched and decoded image for displaying
    bool m_failed;      // True if some requests for this ad have been failed
};

#endif
