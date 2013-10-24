/**
 * Copyright (c) 2012 Nokia Corporation.
 */

#ifndef __BE3D_PROVIDER__
#define __BE3D_PROVIDER__

class QImage;


/*!
  \class Provider
  \brief Interface used for the class that provides the actual book pages as
         GL texture handles.
*/

namespace BE3D
{
    class Provider
    {
    public:
        Provider() {};
        virtual ~Provider() {};

        virtual int getTotalPageCount() = 0;
        virtual QImage *getPage(int pageIndex) = 0;
    };
}

#endif
