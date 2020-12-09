#include "server.h"
#include <QUrl>
#include <QNetworkReply>
#include <QTextCodec>
#include <QJsonDocument>
#include <QJsonObject>

server::server(QString address, QString userId, QObject *parent) :
    QObject(parent),
    m_address(address),
    m_userId(userId)
{
    qnam = new QNetworkAccessManager(this);
}


bool server::save_rec(const QString &dest_name, map_rec &data) {
    emit res_save(true, QString("~") );

    QString str ( m_address +
                  dest_name + "/" +
                  m_userId + "/" );
    // JSON
//    str += "{ ";
//        QMapIterator<QString, QString> e(map_rec);
//        while (e.hasNext()) {
//            e.next();
//            str += "\"" + e.key + "\": \"" + e.value + "\", ";
//        }
//    str += "} ";

    QUrl url(str);
    QNetworkReply *reply = qnam->get(QNetworkRequest(url));
    connect(reply, SIGNAL(finished()), this, SLOT(save_reply(dest_name)) );
    return true;
}

int server::save_reply(const QString &dest_name) {
    int resId = -1;
    QNetworkReply *reply= qobject_cast<QNetworkReply *>(sender());

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray content = reply->readAll();
        QTextCodec *codec = QTextCodec::codecForName("utf8");
        QString reply = codec->toUnicode(content.data()).trimmed().simplified();
        reply = reply.mid(1,reply.length()-2);      // убрал скобки [... ...]

        QJsonDocument jsonResponse = QJsonDocument::fromJson(reply.toUtf8());
        QJsonObject resultInfo = jsonResponse.object();
        resId = resultInfo["result"].toInt();

        if ( resId>0 ) {
            // OK
            emit res_save(true, QString("+") );
        } else {
            // err
            resId = -1;
            emit res_save(true, QString("-") );
        }
    }
    else {
        // ошибка сети
        resId = -1;
        emit res_save(false, QString("! ") + QString::number(reply->error()) );
    }
    // разрешаем объекту-ответа "удалится"
    reply->deleteLater();

    return resId;
}
