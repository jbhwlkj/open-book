/**
 * Copyright (c) 2012 Nokia Corporation.
 */

#ifndef __GALLERYPROVIDER__
#define __GALLERYPROVIDER__

#include "iaengine.h"
#include "provider.h"


// How many images will be burned into a single page of the book.
#define IMAGES_PER_PAGE 2

// Shadow mask's information
#define SHADOW_X_SIZE 5
#define SHADOW_Y_SIZE 7
#define SHADOW_Y_OFFSET 4
// Photo's white frame
#define EDGE_SIZE 7


class GalleryProvider : public BE3D::Provider {
public:
    GalleryProvider(IAEngine *iaEngine);
    ~GalleryProvider();


    // Book::Provider method implementations
    int getTotalPageCount();
    //GLuint getPage( int pageIndex );
    QImage *getPage( int pageIndex );

    QString getLatestPicture(int index) { return m_latestPicture[index]; }
    QString getLatestUrl() { return m_latestAdUrl; }

protected:
    QString m_latestPicture[ IMAGES_PER_PAGE ];
    QString m_latestAdUrl;
    void renderShadows( QImage *target, int x, int y, int w, int h);
    IAEngine *m_iaEngine;
    QImage m_paperTexture;
    QImage m_tapeImage;

    QImage *generateGalleryPage( int firstImage, int imageCount );


    struct SGalleryItem {
        QString imagePath;
        SGalleryItem *next;
    };

    int m_shadowMask[ SHADOW_X_SIZE ][ SHADOW_Y_SIZE ];



    SGalleryItem *m_galleryItems;
    int m_galleryItemCount;
    SGalleryItem *scanFolder( QString path, SGalleryItem *addto, bool subdirs );

        // Helpers
    void frameCreationHelper( QImage &target, int x, int y, int width, int height );
    void createFrame( QImage &target );

    // Highquality rotate/blit/zoomer
    void rotateBlit( QImage*target, QImage *source,
                     float fmiddlex, float fmiddley,float fvx, float  fvy,
                     unsigned int col, unsigned int seta );
};

#endif
