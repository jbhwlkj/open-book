/**
 * Copyright (c) 2012 Nokia Corporation.
 */

#ifndef __IAENGINE__
#define __IAENGINE__

#include <QtCore/QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include "iacapsule.h"
#include "xmle.h"


class IAEngine : public QObject {
    Q_OBJECT

    enum eREQUESTTYPE { eXML, eIMAGE };

public:
    IAEngine( QObject *parent, QString appid );
    ~IAEngine();
    void releaseCapsules();


        // Send a request to get a new ad, returns true if the request
        // were succesfully made.
    bool requestAd();

        // How ads have been stored
    int getCapsuleCount();

        // Get an ad capsule of specified index
    IACapsule *getCapsule( int index );

        // True if a requerst is active and new calls cannot be made.
    bool isProcessing() { if (m_latestRequestReply != 0) return true; else return false; }


private slots:
    void requestFinished( QNetworkReply *reply );


protected:
    bool capsuleExists( IACapsule *capsule );
    static void nodeReadyCallback( TXML::Node *node, void *thispointer );

protected: // Data
    TXML::TextData m_clientId;
    QNetworkAccessManager *m_networkAccessManager;
    QString m_appId;

    QNetworkReply *m_latestRequestReply;
    eREQUESTTYPE m_latestRequestType;
    IACapsule *m_latestRequestCapsule;

    IACapsule *m_adCapsules;
};

#endif
