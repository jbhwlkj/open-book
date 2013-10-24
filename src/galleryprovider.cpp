/**
 * Copyright (c) 2012 Nokia Corporation.
 */

#include "galleryprovider.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <math.h>

#include "jpegthumbnailfetcher.h"


/*!
  \class GalleryProvider
  \brief Implementation of a page provider BE3D::Provider which renders pages
         from device's photo-gallery.
*/

GalleryProvider::SGalleryItem *GalleryProvider::scanFolder( QString path, SGalleryItem *addto, bool subdirs ) {
    SGalleryItem *rval = addto;
    qDebug() << "Scanning gallery: " << path;
    QFileInfoList list;
    QDir dir(path);
    // images
    QStringList flist;
    flist << "*.jpg" << "*.JPG";
    list = dir.entryInfoList( flist, QDir::Files | QDir::NoSymLinks,QDir::Time );
    for (int i=0; i<list.size(); i++) {
        SGalleryItem *nitem = new SGalleryItem;
        nitem->next = 0;
        nitem->imagePath = list.at(i).absoluteFilePath();

        // add
        if (!rval)
            rval = nitem;
        else {
            SGalleryItem *l = rval;
            while (l && l->next) l = l->next;
            l->next = nitem;
        }
    }

    // subfolders.
    QDir dirsub( path );
    if (subdirs) {
        dirsub.setFilter( QDir::Dirs | QDir::NoSymLinks);
        list = dirsub.entryInfoList();
        for (int i=0; i<list.size(); i++) {
            if (list.at(i).isDir()) {
                QString fn = list.at(i).fileName();
                if (fn != "." && fn!="..") {
                    rval = scanFolder( list.at(i).absoluteFilePath(), rval, true );
                }
            }
        }
    }


    // Load the background image
    m_paperTexture = QImage(":/images/paper.jpg");
    // Load the tape image
    m_tapeImage = QImage(":/images/tape.png");


    return rval;
}


GalleryProvider::GalleryProvider(IAEngine *iaEngine) {
    m_galleryItems = 0;
#ifdef Q_OS_SYMBIAN
    m_galleryItems = scanFolder("C:/DCIM/", m_galleryItems, true );
    //m_galleryItems = scanFolder("C:/Images/", m_galleryItems, true );
    m_galleryItems = scanFolder("D:/DCIM/", m_galleryItems, true );
    //m_galleryItems = scanFolder("D:/Images/", m_galleryItems, true );
    m_galleryItems = scanFolder("E:/DCIM/", m_galleryItems, true );
    //m_galleryItems = scanFolder("E:/Images/", m_galleryItems, true );
    m_galleryItems = scanFolder("F:/DCIM/", m_galleryItems, true );
    //m_galleryItems = scanFolder("F:/Images/", m_galleryItems, true );



    //m_galleryItems = scanFolder("C:/Data/Images", m_galleryItems, true );
#else
    m_galleryItems = scanFolder("c:/coding", m_galleryItems, false);
#endif

    m_iaEngine = iaEngine;

    // calculate items
    m_galleryItemCount = 0;
    SGalleryItem *l = m_galleryItems;
    while (l) {
        m_galleryItemCount++;
        l = l->next;
    }
    qDebug() << "Gallery items scanned: " << m_galleryItemCount;

    // Create the shadowmask
    // shadowmask
    for (int y=0; y<SHADOW_Y_SIZE; y++) {
        float fy = ((float)y / (float)(SHADOW_Y_SIZE-1) - 0.5f)*2.0f;
        for (int x=0; x<SHADOW_X_SIZE; x++) {
            float fx = ((float)x / (float)(SHADOW_X_SIZE-1) - 0.5f)*2.0f;
            m_shadowMask[x][y] = (int)( (1.0f - sqrtf(fx*fx+fy*fy)) * 255.0f )* 4.0f;
            if (m_shadowMask[x][y]<0) m_shadowMask[x][y] = 0;
        }
    }

}


GalleryProvider::~GalleryProvider() {
    SGalleryItem *l = m_galleryItems;
    while (l) {
        SGalleryItem *n = l->next;
        delete l;
        l = n;
    }
    m_galleryItems = 0;
}


int GalleryProvider::getTotalPageCount() {

    return 4+((m_galleryItemCount/IMAGES_PER_PAGE)/2*2);
}


unsigned int edgeCols[] = {0x08000000, 0x22000000, 0x66000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
void GalleryProvider::frameCreationHelper( QImage &target, int x, int y, int width, int height ) {
    for (int f=0; f<height; f++) {
        int realx = x;
        int realy = y+f;
        unsigned int *t = (unsigned int*)target.bits() + (target.bytesPerLine()>>2) * (realy) + x;
        unsigned int *t_target = t+width;
        int dis;
        int iwidth = target.width();
        int iheight = target.height();

        while (t!=t_target) {
            dis = 255;
            if (realx<dis) dis = realx;
            if (realy<dis) dis = realy;
            if (iwidth-realx<dis) dis = iwidth-realx;
            if (iheight-realy<dis) dis = iheight-realy;
            *t = edgeCols[dis];
            realx++;
            t++;
        }
    }
}


void GalleryProvider::createFrame( QImage &target )
{
    // Draw the frame
        // Mark for shadows?
    frameCreationHelper(target, 0,0, target.width(), EDGE_SIZE);
    frameCreationHelper(target, 0,target.height()-EDGE_SIZE, target.width(), EDGE_SIZE);
    frameCreationHelper(target, 0,EDGE_SIZE, EDGE_SIZE, target.height()-EDGE_SIZE*2 );
    frameCreationHelper(target, target.width()-EDGE_SIZE,EDGE_SIZE, EDGE_SIZE, target.height()-EDGE_SIZE*2 );
}



QImage *GalleryProvider::generateGalleryPage( int firstImage, int imageCount )
{
    // Seek through gallery items to find requested page
    SGalleryItem *l = m_galleryItems;
    while (l && firstImage>0) { firstImage--; l = l->next; }

    // Set the random seed according our current page so we can always get same kind of page
    QImage *rval = new QImage(240, 360, QImage::Format_ARGB32 );
    //QImage *rval = new QImage(256, 512, QImage::Format_ARGB32 );

    float avgSize = (float)(rval->width() + rval->height())/2.0f;

    // Fill the new page with empty texture first
    for (int y=0; y<rval->height(); y++) {
        unsigned int *t = (unsigned int*)rval->bits() + rval->bytesPerLine()/4 * y;
        unsigned int *s = (unsigned int*)m_paperTexture.bits() + m_paperTexture.bytesPerLine()/4 * (y&(m_paperTexture.height()-1));
        for (int x=0; x<rval->width(); x++) {
            // Mask the alphacomponent away
            *t = (s[x&(m_paperTexture.width()-1)] & 0x00FFFFFF);
            t++;
        }
    }


    QImage galImage;


    // Burn an add if exists
    int adCount = m_iaEngine->getCapsuleCount();
    if (adCount>0) {
        IACapsule *cap = m_iaEngine->getCapsule(((rand()&255)*(adCount-1))>>8);
        if (cap)
        {
            m_latestAdUrl = cap->targetUrl;
            galImage = cap->m_image;
            float ang = 0.0f; // don't mess with the ad, let it be straight
            float adsize = rval->height()*0.05f;
            rotateBlit( rval, &galImage, rval->width()/2.0f, rval->height()*0.9f,sinf(ang)*adsize, cosf(ang)*adsize, 0xBBFFFFFF, 0x0 );
        }
    }

    JpegThumbnailFetcher fetcher;
    int index = 0;

    while (index<imageCount) {
        if (!l) break;          // FATAL ERROR. NO MORE IMAGE
        galImage = fetcher.fetchThumbnail( l->imagePath );
        m_latestPicture[index] = l->imagePath;

        if (galImage.isNull()==false) {

            // Choose which component is larger for better sizing
            bool heightlarger = true;
            if (galImage.width()>galImage.height()) heightlarger = false;

            // Create frame to the thumbnail
            createFrame( galImage );

            float randomAngle = (float)((rand() & 255)/255.0f  - 0.5f)*0.5f;
            float size =  avgSize/4.0f; // + ((float)(rand() & 255 ) / 255.0f)*avgSize/16.0f;

            if (heightlarger==false)        // width is larger, scale the size accordingly
                size = size * (float)galImage.height() / (float)galImage.width();


            float xpos = (float)(index+1)/(float)(IMAGES_PER_PAGE+1) * rval->width();
            float ypos = (float)(index+1)/(float)(IMAGES_PER_PAGE+1) * rval->height();

            float yvx = sinf( randomAngle ) * size;
            float yvy = cosf(randomAngle) * size;


            // Draw the image
            rotateBlit( rval, &galImage,
                        xpos, ypos,
                        yvx, yvy,
                        0xFFFFFFFF, 0xFF000000 );



            // Tape it on
            size = avgSize / 64.0f + (float)(rand() & 255)/255.0f * avgSize /128.0f;
            randomAngle += (float)(rand() & 255)/255.0f - 0.5f;
            rotateBlit( rval, &m_tapeImage,
                        xpos-yvx, ypos-yvy,
                        sinf( randomAngle ) * size, cosf(randomAngle) * size,
                        0x88888888, 0);



        }
        l = l->next;
        index++;
    }


    // Burn the shadow into the image
    //renderShadows(rval, 0, 0, rval->width(), rval->height());

    return rval;
}

void GalleryProvider::renderShadows( QImage *target, int bx, int by, int bw, int bh) {
    int rvpitch = (target->bytesPerLine()>>2);
    int sx,sy;
    unsigned int val;
    unsigned int shadowmul = (65536) / (SHADOW_X_SIZE*SHADOW_Y_SIZE);
    unsigned int *rvdata = (unsigned int*)target->bits();
    for (int y=bh-1; y>=0; y--) {

        unsigned int *t = rvdata + rvpitch*(by+y) + bx;
        for (int x=0; x<bw; x++) {
            if (((*t)>>24) == 0) {
                val  = 0;
                sy = y-SHADOW_Y_OFFSET*2;
                for (int ty=0; ty<SHADOW_Y_SIZE; ty++) {
                    if (sy >= 0 && sy<bh) {
                        sx = x-(SHADOW_X_SIZE);
                        for (int tx=0;  tx<SHADOW_X_SIZE; tx++) {
                            if (sx >= 0 && sx<bw) {
                                if ((rvdata[rvpitch*sy + sx]>>24)>128) val+=m_shadowMask[tx][ty];
                            }
                            sx+=2;
                        }
                    }
                    sy+=2;
                }

                if (val>0) {
                    //val /= (SHADOW_X_SIZE * SHADOW_Y_SIZE);
                    val = ((val*shadowmul)>>15);
                    val = 255-val;


                    // Darken the color accordingly, leave the alpha untouched.
                    if (val>0) {
                        *t = ((((*t)&255)*val)>>8) |
                                ((((((*t)>>8)&255)*val)>>8)<<8) |
                                ((((((*t)>>16)&255)*val)>>8)<<16);
                    }
                }
            }

            t++;
        }
    }
}

QImage *GalleryProvider::getPage( int pageIndex ) {
    if (pageIndex<0 || pageIndex>=getTotalPageCount()) return 0;

    char testr[256];
    if (pageIndex==0 || pageIndex==getTotalPageCount()-1) {
        if (pageIndex == 0)
            strcpy( testr, ":/images/front_cover.png");
        else
            strcpy( testr, ":/images/cover.png");
    } else
        if (pageIndex==1 || pageIndex==getTotalPageCount()-2) {
            if (pageIndex==1)
                strcpy( testr, ":/images/info_page.jpg");
            else
                strcpy( testr, ":/images/empty.png");
        } else {
            // Create the actual page requiring rasterization
            //int galleryIndex = (pageIndex-1) / IMAGES_PER_PAGE * IMAGES_PER_PAGE;
            int galleryIndex = (pageIndex-1)*IMAGES_PER_PAGE;
            // Create a gallerypage from images galleryIndex and galleryIndex + 1
            return generateGalleryPage( galleryIndex,IMAGES_PER_PAGE );
        }

    // Load a preset-page / cover or empty page
    QString fn = QString(testr);
    QImage *rval = new QImage( fn );
    return rval;
}


/**
 * rotateBlit and some fixedpoint-functions it requires
 *
 */

#define FP_BITS 14
#define FP_VAL 16384
#define FP_AND 16383

inline int fpdiv( const int i1, const int i2 ) {
    if (i2==0)
        return i1;
    else
        return (int)( (((__int64)i1)<<(__int64)(FP_BITS)) / ((__int64)i2) );
}

inline int fpmuldiv( const int m1, const int m2, const int div ) {
    return (int)( ((__int64)m1 * (__int64)m2) / div ) ;
}

inline int fpceil(const int x) { return ((x+FP_AND) >> FP_BITS); }

inline int fpmul( const int i1, const int i2 ) {
    return (int)( ((__int64)i1*(__int64)i2)>>FP_BITS );
}


void GalleryProvider::rotateBlit( QImage*target, QImage *source,
                                  float fmiddlex, float fmiddley,float fvx, float  fvy,
                                  unsigned int col, unsigned int seta )
{
    if (!target || !source) return;
    if (target->isNull() || source->isNull()) return;
    int middlex = (int)( fmiddlex * (float)FP_VAL );
    int middley = (int)( fmiddley * (float)FP_VAL );
    int vx = (int)( fvx * (float)FP_VAL );
    int vy = (int)( fvy * (float)FP_VAL );

    int spitch = (source->bytesPerLine()>>2);
    int swidth = source->width();
    int sheight = source->height();
    unsigned int c1,c2,c3,c4, ofsx, ofsy;

    int temp;
    int x1,y1,x2,y2,nx,ny;
    int leftx,lefty,rightx,righty;

    temp = (swidth<<FP_BITS) / sheight;
    if (vy>0) {
        nx = fpmul( -vy, temp );
        ny = fpmul( vx, temp );
        x1 = (middlex - vx);
        y1 = (middley - vy);
        x2 = (middlex + vx);
        y2 = (middley + vy);
        if (ny>0) {
            leftx = x1+nx;
            lefty = y1+ny;
            rightx = x1-nx+vx*2;
            righty = y1-ny+vy*2;
        } else {
            leftx = x1+nx + vx*2;
            lefty = y1+ny + vy*2;
            rightx = x1-nx;
            righty = y1-ny;
            nx=-nx; ny=-ny;
        }
    } else {
        nx = fpmul( vy, temp );
        ny = fpmul( -vx, temp );
        x1 = (middlex + vx);
        y1 = (middley + vy);
        x2 = (middlex - vx);
        y2 = (middley - vy);
        if (ny>0) {
            leftx = x1+nx;
            lefty = y1+ny;
            rightx = x1-nx-vx*2;
            righty = y1-ny-vy*2;
        } else {
            leftx = x1+nx - vx*2;
            lefty = y1+ny - vy*2;
            rightx = x1-nx;
            righty = y1-ny;
            nx=-nx; ny=-ny;
        }
    }
    x1 -= nx; y1 -= ny;
    x2 += nx; y2 += ny;

    int leftxinc = fpdiv( leftx-x1, (lefty-y1)+1 );
    int rightxinc = fpdiv( rightx-x1, (righty-y1)+1 );


    // subpixel, and yclipping
    int intery = ((y1>>FP_BITS)<<FP_BITS) + FP_VAL/2;
    if (intery<0) intery=0;
    temp = intery-y1;
    int interx_left = x1+fpmul( leftxinc, temp );
    int interx_right = x1+fpmul( rightxinc, temp );

    __int64 jtemp = ((__int64)vx*(__int64)vx + (__int64)vy*(__int64)vy)>>FP_BITS;
    jtemp+=(jtemp>>8);
    vx = (int)(((__int64)(vx * sheight)<<13) / (__int64)jtemp );
    vy = (int)(((__int64)(vy * sheight)<<13) / (__int64)jtemp );

    nx = vy;
    ny = -vx;




    int rasx;
    unsigned int *s = (unsigned int*)source->bits();
    unsigned int *t, *t_target;

    // Rasterize.
    while (intery<y2 && intery<(target->height()<<FP_BITS)) {
        if (intery>lefty) {
            leftxinc = fpdiv( x2-leftx,y2-lefty+1 );
            interx_left = leftx + fpmul( leftxinc, intery-lefty+1 );
            lefty=y2;			// never do this again
        };

        if (intery>righty) {
            rightxinc = fpdiv( x2-rightx, y2-righty+1 );
            interx_right = rightx + fpmul( rightxinc, intery-righty+1 );
            righty = y2;		// never do this again.
        };



        x1 = (source->height()<<13) +
                fpmul( intery-middley, nx ) +
                fpmul( interx_left-middlex, vx );
        y1 = (source->width()<<13) +
                fpmul( intery-middley, ny ) +
                fpmul( interx_left-middlex, vy );


        // SUBTEXEL ACCURACY IN INNER LOOP
        rasx = fpceil( interx_left );
        if (rasx<0) rasx = 0;
        temp = (rasx<<FP_BITS)-interx_left;
        x1 += fpmul( vx, temp );
        y1 += fpmul( vy, temp );


        temp = fpceil(interx_right);
        if (temp>target->width()) temp = target->width();
        if (temp>rasx) {
            unsigned int c;
            t = (unsigned int*)target->bits() + (intery>>FP_BITS)*(target->bytesPerLine()>>2);
            t_target = t+temp;
            t+=rasx;

            while (t!=t_target) {


                if (x1>0 && y1>0 && x1<(sheight-1) << FP_BITS && y1<(swidth-1) << FP_BITS) {
                    temp = (x1>>FP_BITS) *spitch  + (y1>>FP_BITS);
                    ofsx = y1&FP_AND;
                    ofsy = x1&FP_AND;
                    c1 = s[temp];
                    c2 = s[temp+1];
                    c3 = s[temp+1+spitch];
                    c4 = s[temp+spitch];

                    // resample alpha
                    temp = (((((  ((( ((c1>>24)&255)*(FP_AND^ofsx) + ((c2>>24)&255)*ofsx ) >> FP_BITS) * (FP_AND^ofsy) ) +
                                  ((( ((c4>>24)&255)*(FP_AND^ofsx) + ((c3>>24)&255)*ofsx ) >> FP_BITS) * (ofsy) ) ) >> FP_BITS)) * (col>>24)) >> 8);

                    if (temp>0) {
                        c1 = (((  ((( ((c1>>0)&255)*(FP_AND^ofsx) + ((c2>>0)&255)*ofsx ) >> FP_BITS) * (FP_AND^ofsy) ) +
                                  ((( ((c4>>0)&255)*(FP_AND^ofsx) + ((c3>>0)&255)*ofsx ) >> FP_BITS) * (ofsy) ) ) >> FP_BITS)<<0) |
                                (((  ((( ((c1>>8)&255)*(FP_AND^ofsx) + ((c2>>8)&255)*ofsx ) >> FP_BITS) * (FP_AND^ofsy) ) +
                                     ((( ((c4>>8)&255)*(FP_AND^ofsx) + ((c3>>8)&255)*ofsx ) >> FP_BITS) * (ofsy) ) ) >> FP_BITS)<<8) |
                                (((  ((( ((c1>>16)&255)*(FP_AND^ofsx) + ((c2>>16)&255)*ofsx ) >> FP_BITS) * (FP_AND^ofsy) ) +
                                     ((( ((c4>>16)&255)*(FP_AND^ofsx) + ((c3>>16)&255)*ofsx ) >> FP_BITS) * (ofsy) ) ) >> FP_BITS)<<16);

                        *t = ((( ((((c1>>0)&255)*temp*(col&255))>>8) + (((*t)>>0)&255)*(255^temp) )>>8)<<0) |
                                ((( ((((c1>>8)&255)*temp*((col>>8)&255))>>8) + (((*t)>>8)&255)*(255^temp) )>>8)<<8) |
                                ((( ((((c1>>16)&255)*temp*((col>>16)&255))>>8) + (((*t)>>16)&255)*(255^temp) )>>8)<<16) |
                                (seta);

                    }
                }

                x1 += vx;
                y1 += vy;
                t++;
            };

        }
        interx_left += leftxinc;
        interx_right += rightxinc;
        intery+=FP_VAL;
    }
}


