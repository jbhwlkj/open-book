/**
 * Copyright (c) 2012-2014 Microsoft Mobile.
 */

#include "iaengine.h"

#include <QtCore/QBuffer>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QUrl>
#include <QtGui/QImageReader>

#include "xmle.h"


/*!
  \class IAEngine
  \brief Implements Inneractive advertisement HTTP API.
*/


IAEngine::IAEngine( QObject *parent, QString appId ) :
    QObject( parent ),
    m_appId(appId),
    m_adCapsules(0),
    m_latestRequestReply(0),
    m_latestRequestCapsule(0)
{
    m_clientId.set("7058347345");
    // Create the network access manager
    m_networkAccessManager = new QNetworkAccessManager(this);

    // Connect a signal so we get a notification when a request
    // is completed.
    QObject::connect(m_networkAccessManager,
                     SIGNAL(finished(QNetworkReply*)),
                     this,
                     SLOT(requestFinished(QNetworkReply*)));


    requestAd();
}


IAEngine::~IAEngine()
{
    releaseCapsules();
    if (m_networkAccessManager) delete m_networkAccessManager;
}


void IAEngine::releaseCapsules() {
    IACapsule *l = m_adCapsules;
    while (l) {
        m_adCapsules = l->m_next;
        delete l;
        l = m_adCapsules;
    }
}


bool IAEngine::requestAd()
{
    if (m_latestRequestReply != 0) return false;          // cannot continue while still waiting for answer

        // ID's of some stores
    // OVI = 551
    // iphone = 642
    // android = 600

    QString urls =
            QString("http://m2m1.inner-active.mobi/simpleM2M/clientRequestAd?aid=") +
            QString( m_appId ) +
            QString("&po=642&v=Sm2m-1.5.3&cid=") +
            QString( m_clientId.getData() );

    QUrl url( urls );
    m_latestRequestReply = m_networkAccessManager->get(QNetworkRequest(url));
    m_latestRequestType = eXML;
    m_latestRequestCapsule = 0;

    return true;
}


void IAEngine::nodeReadyCallback( TXML::Node *node, void *thispointer )
{
    IAEngine *e = (IAEngine*)thispointer;
    if (node->name() != 0 &&
        node->name()[0] != 0 &&
        strcmp( node->name(), "tns:Ad") == 0)
    {
        TXML::Node *targetUrlNode = node->gotoPath("tns:URL");
        TXML::Node *imageUrlNode = node->gotoPath("tns:Image");
        TXML::Node *adTextNode = node->gotoPath("tns:Text");


        if (targetUrlNode && imageUrlNode) {
            IACapsule *newCapsule = new IACapsule;
            if (imageUrlNode->data())
                newCapsule->imageUrl = QString(imageUrlNode->data());
            if (targetUrlNode->data()) {
                newCapsule->targetUrl = QString(targetUrlNode->data());
                // Replace "&amp;" with just "&" to make the url work
                newCapsule->targetUrl.replace("&amp;", "&");
            } if (adTextNode && adTextNode->data())
                newCapsule->text = QString( adTextNode->data());

            // add it into the capsule list
            if (!e->m_adCapsules)
                e->m_adCapsules = newCapsule;
            else {
                IACapsule *l = e->m_adCapsules;
                while (l->m_next) l = l->m_next;
                l->m_next = newCapsule;
            }
        } else {
            qDebug() << "tns:Ad didn't provide enough data. Discarding.";
        }
    }
    else {
        if (node->name()!=0 &&
            node->name()[0] != 0 &&
            strcmp( node->name(), "tns:Client") == 0) {     // tns:Client tag, store the new ID
            char *newId = node->getAttribute("Id");
            qDebug() << "tns:Client: Setting client ID: " << newId;
            e->m_clientId.set( newId );
        };
    }
}

bool IAEngine::capsuleExists( IACapsule *capsule )
{
    IACapsule *l = m_adCapsules;
    while (l) {
        if (capsule->text == l->text &&
                capsule->imageUrl == l->imageUrl &&
                capsule->targetUrl == l->targetUrl ) return true;
        l = l->m_next;
    }
    return false;
}


void IAEngine::requestFinished(QNetworkReply* reply)
{
    if (reply==0) {
        qDebug() << "ERROR, NULL REPLY.";
        m_latestRequestReply = 0;
        return;
    }
    if (reply!=m_latestRequestReply) {
        qDebug() << "Warning, different reply than stored.";
    }


    QVariant statusCodeV =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

    qDebug() << "http status: " << statusCodeV;



    // no error received?
    if (reply->error() == QNetworkReply::NoError)
    {
        if (m_latestRequestType==eIMAGE) {
            if (m_latestRequestCapsule) {
                // read data from QNetworkReply here
                QImageReader imageReader(reply);
                m_latestRequestCapsule->m_image = imageReader.read();
                qDebug() << "Incoming image, readed width: " << m_latestRequestCapsule->m_image.width() << " height:" << m_latestRequestCapsule->m_image.height();
            } else {
                qDebug() << "Error. Incoming image without attached capsule.";
            }
        } else {            // XML

            qDebug() << "parsing xml.";
            if (reply->size()>8) {
                TXML::Reader xmlreader( nodeReadyCallback, this );
                TXML::Node *xmldata = xmlreader.scanXML( *reply );


                QFile testFile( QString("test.txt") );
                testFile.open( QIODevice::WriteOnly );
                xmldata->saveXML( testFile );
                testFile.close();

                qDebug() << "xml parsed: " << xmldata->name();
                delete xmldata;
            } else qDebug() << "Null answer, ignoring";
        }

    }
    // Some http error received
    else
    {
        // handle errors here
    }

    m_latestRequestCapsule = 0;
    m_latestRequestReply = 0;
    reply->deleteLater();

    // check if some images needs to be fetched
    IACapsule *l = m_adCapsules;
    while (l) {
        if (l->m_image.isNull() && l->m_failed == false && l->imageUrl.isEmpty() == false && l->imageUrl.isNull()==false) {
            QUrl url( l->imageUrl );
            m_latestRequestReply = m_networkAccessManager->get(QNetworkRequest(url));
            m_latestRequestType = eIMAGE;
            m_latestRequestCapsule = l;
            return;
        }
        l = l->m_next;
    }
}

int IAEngine::getCapsuleCount()
{
    int rval = 0;
    IACapsule *l = m_adCapsules;
    while (l) { l = l->m_next; rval++; }
    return rval;
}

IACapsule *IAEngine::getCapsule( int index )
{
    IACapsule *l = m_adCapsules;
    while (l && index>0) {
        l = l->m_next;
        index--;
    }
    return l;
}
