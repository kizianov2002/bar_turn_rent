#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>

#include <QtNetwork>
#include <QTcpSocket>
#include <QObject>
#include <QByteArray>
//#include <QDebug>

#include "core_object.h"

class TcpServer : public QObject
{
    Q_OBJECT
    QSettings &settings;
    QTextStream &debug;

    core_object &CO;
    int port;

private:
    QTcpServer *tcpServer;
    int server_status;
    QMap<int, QTcpSocket*> SClients;
    QStringList TcpData;

public:
    TcpServer(QSettings &settings, core_object &CO, QTextStream &debug, QObject *parent);
    bool debug_MapRec(QString name, map_rec map);

    bool startServer();
    bool stopServer();

    int getParams(QString query_data, QString &quest, map_rec &map);

    QString decode(QByteArray &data);

public slots:
    void newUser();
    void readClient();

};

#endif // TCPSERVER_H
