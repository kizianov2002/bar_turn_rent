#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QNetworkAccessManager>

#include "db_rec.h"

class server : public QObject
{
    Q_OBJECT

    QNetworkAccessManager *qnam;
    QString m_address;
    QString m_userId;

public:
    explicit server(QString address, QString userId, QObject *parent = nullptr);

    bool save_rec(const QString & dest_name, map_rec &data);

public slots:
    int save_reply(const QString &dest_name);

signals:
    void res_save(bool is_ok, QString msg_text /*,bool is_save*/);
};

#endif // SERVER_H
