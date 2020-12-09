#include "tcpserver.h"

TcpServer::TcpServer(QSettings &settings, core_object &CO, QTextStream &debug, QObject *parent) :
    QObject(parent),
    CO(CO), settings(settings), debug(debug)
{
    port = settings.value("main/tcp_port", 44444).toInt();
}

bool TcpServer::debug_MapRec(QString name, map_rec map)
{
    QString str = name + ": [\n";
    QStringList keys = map.keys();
    foreach (QString key, keys) {
        str += "    " + key + "=" + map[key] + " null=" + map.nulls[key] + ", \n";
    }
    str += "]";
    debug << "-" << str << "\n";
    debug.flush();
    return true;
}



bool TcpServer::startServer()
{
    int port = settings.value("main/tcp_port", 9999).toInt();
    TcpData.clear();
    tcpServer = new QTcpServer(this);
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(newUser()));
    if (!tcpServer->listen(QHostAddress::Any, port) && server_status==0) {
         debug << "-" <<  QObject::tr("Unable to start the server: %1.").arg(tcpServer->errorString());
         debug << "\n";  debug.flush();
        TcpData.append(tcpServer->errorString());
        return false;
    } else {
        server_status=1;
         debug << "-" << tcpServer->isListening() << " " << "TCP Socket listen on port " << " " << port;
        TcpData.append(QString::fromUtf8("TCP server start!"));
         debug << "-" << QString::fromUtf8("TCP server start!");
         debug << "\n";  debug.flush();
    }
    return true;
}

bool TcpServer::stopServer()
{
    if(server_status==1){
        foreach(int i,SClients.keys()){
            QTextStream os(SClients[i]);
            os.setAutoDetectUnicode(true);
            os << " " << QDateTime::currentDateTime().toString() << " " << "\n";
            SClients[i]->close();
            SClients.remove(i);
        }
        tcpServer->close();
        TcpData.append(QString::fromUtf8("TCP server stop!"));
         debug << "-" << QString::fromUtf8("TCP server stop!");
         debug << "\n";  debug.flush();
        server_status=0;
    }
    return true;
}

void TcpServer::newUser()
{
    if(server_status==1){
         debug << "-" << QString::fromUtf8("TCP new user!");
         debug << "\n";  debug.flush();
        TcpData.append(QString::fromUtf8("TCP new user!"));
        QTcpSocket* clientSocket=tcpServer->nextPendingConnection();
        int idusersocs=clientSocket->socketDescriptor();
        SClients[idusersocs]=clientSocket;
        connect(SClients[idusersocs],SIGNAL(readyRead()),this, SLOT(readClient()));
    }
}

QString TcpServer::decode(QByteArray &rawData) {
    // debug << "-" << "  -----";
    // debug << "-" << " decode >> in" << " " << rawData;
    // уберём %
    int len = rawData.length();
    QByteArray rawData2;
    bool ok;
    for (int i=0; i<rawData.size(); i++) {
        char c = rawData.at(i);
        if (c=='%' && i<len-2) {
            QString s;
            s.append(QChar(rawData.at(i+1)));
            s.append(QChar(rawData.at(i+2)));
            char c = s.toInt(&ok, 16);
            rawData2.append(c);
            i +=2;
        } else {
            rawData2.append(rawData.at(i));
        }
    }
    QString res = QString::fromUtf8(rawData2);
    res.replace("+", " ");

//    // преобразуем юникод-16
//    debug << "-" << "  -----";
//    debug << "-" << " decode >> UTF8" << " " << rawData2;
//    QDataStream ds(&rawData2, QIODevice::ReadOnly);
//    Q_ASSERT(rawData2.count() % 2 == 0);
//    QVector<ushort> buf(rawData2.count()/2 + 1);
//    int n = 0;
//    ds.setByteOrder(QDataStream::BigEndian);
//    while (!ds.atEnd()) {
//        ushort s;
//        ds >> s;
//        buf[n++] = s;
//    }
//    buf[n] = 0;
//    QString res = QString::fromUtf16(buf.constData());
//    debug << "-" << " decode >> res" << " " << res;

    return res;

//    int pos = 0, pos1 = 0;
//    QString res = "";
//    QString cs;
//    int len = rawData.length(), i;
//    while (pos<len && (pos1 = rawData.indexOf("%", pos))>=0) {
//          res += rawData.mid(pos, pos1-pos);
//          cs = rawData.mid(pos1+1, 2);
//          i = cs.toInt(&ok, 16);
//          res += QChar(i);
//          pos = pos1+3;
//    }
//    if (pos<len)
//        res += rawData.right(len-pos);
//     debug << "-" << " decode >> out1" << " " << res;
//    res.replace("\\\"","\"");
//    // debug << "-" << "  >> out2" << " " << res;
//    if (res.left(1)=="'" && res.right(1)=="'")
//        res = res.mid(1, res.size()-2);
//    if (res.right(1)=="'")
//        res = res.mid(0, res.size()-1);
//    if (res.left(1)=="'")
//        res = res.mid(1, res.size()-1);
////     debug << "-" << " decode >> out3" << " " << res;
//    return res;
}

#include <parser.h>

int TcpServer::getParams(QString data, QString &quest, map_rec &map) {
    data.replace("GET/", "GET /").replace("HTTP /", "HTTP/");
    int pos0 = data.indexOf("GET /") + 5;
    int pos1 = data.indexOf("?", pos0) + 1;
    int pos2 = data.indexOf("HTTP/", pos1);
    quest = data.mid(pos0, pos1-pos0-1).trimmed().toLower();
    QString s_param = data.mid(pos1, pos2-pos1);
    // debug << "-" << "   TCP s_param" << " " << s_param;
    QStringList lst = s_param.split("&");
    // debug << "-" << "   TCP lst" << " " << lst;
    map.clear();
    QString s, name, val;
    for (int i=0; i<lst.size(); i++) {
        s = lst[i].trimmed();
        QStringList ss = s.split("=");
        if (ss.size()<1)
            continue;
        if (ss.size()<2) {
            name = ss[0].trimmed().toLower();
            val  = "";
        } else {
            name = ss[0].trimmed().toLower();
            val  = ss[1].trimmed();
            // debug << "-" << "   TCP name" << " " << name << " " << "val" << " " << val;
            if ( val.left(1)=="'" && val.right(1)=="'")
                val = val.mid(1, val.length()-2);
        }
         // debug << "-" << "   8888 -" << " " << name << " " << val;
        map.add(name, val);
    }
     // debug << "-" << "map" << " " << map;
    debug << "\n";  debug.flush();
    return map.size();
}

void TcpServer::readClient()
{
    debug << "-" << "  Tcp ----- in";
    debug << "\n";  debug.flush();
    QTcpSocket* clientSocket = (QTcpSocket*)sender();
    int idusersocs=clientSocket->socketDescriptor();
    QTextStream os(clientSocket);
    QByteArray rawData = clientSocket->readAll();
    QString data = decode(rawData);
    debug << "TCP data " << data;
    debug.flush();
    TcpData.append("ReadClient:" + data + "\n\r");

    // проверим, что пришло
    QString quest;
    map_rec map_params;
    int cnt_params = getParams(data, quest, map_params);

    int print_mode = settings.value("barcode/mode", "0").toInt();

    debug << "-" << "TCP quest" << "\n" << quest;
    debug << "\n"; debug.flush();
    debug_MapRec("TCP map_params", map_params);

    if (quest=="generate_tickets") {
        // ----------------
        // generate_tickets
        // ----------------
        int cnt  = map_params["quantity"].toInt();
        int type = map_params["type"].toInt();

        os.setAutoDetectUnicode(true);
        if (CO.genTicketsMass(type, cnt)) {
            // debug << "-" << "   TCP genTicketsMass()  OK";
            os << "HTTP/1.0 200 Ok\r\n"
                  "Content-Type: text/html; charset=\"utf-8\"\r\n"
                  "Access-Control-Allow-Origin: *\r\n"
                  "\r\n"
                  "<h1>1</h1>\n";
//                  << " " << QDateTime::currentDateTime().toString() << " " << "\n";
        } else {
            // debug << "-" << "   TCP genTicketsMass()  fail";
            os << "HTTP/1.0 200 ERROR\r\n"
                  "Content-Type: text/html; charset=\"utf-8\"\r\n"
                  "Access-Control-Allow-Origin: *\r\n"
                  "\r\n"
                  "<h1>0</h1>\n";
//                  << " " << QDateTime::currentDateTime().toString() << " " << "\n";
        }

    } else if (quest=="generate_invents") {
        // ----------------
        // generate_invents
        // ----------------
        int cnt  = map_params["quantity"].toInt();

        os.setAutoDetectUnicode(true);
        if (CO.genInventsMass(cnt)) {
            // debug << "-" << "   TCP genInventsMass()  OK";
            os << "HTTP/1.0 200 Ok\r\n"
                  "Content-Type: text/html; charset=\"utf-8\"\r\n"
                  "Access-Control-Allow-Origin: *\r\n"
                  "\r\n"
                  "<h1>1</h1>\n";
//                  << " " << QDateTime::currentDateTime().toString() << " " << "\n";
        } else {
            // debug << "-" << "   TCP genInventsMass()  fail";
            os << "HTTP/1.0 200 ERROR\r\n"
                  "Content-Type: text/html; charset=\"utf-8\"\r\n"
                  "Access-Control-Allow-Origin: *\r\n"
                  "\r\n"
                  "<h1>0</h1>\n";
//                  << " " << QDateTime::currentDateTime().toString() << " " << "\n";
        }

    } else if (quest=="print_tickets") {
        // ------------
        // print_tickets
        // --------------
        int cnt  = map_params["quantity"].toInt();
        //int type = map_params["type"].toInt();

        os.setAutoDetectUnicode(true);

        if (CO.printTickets(cnt, print_mode)) {
            // debug << "-" << "   TCP printTickets()  OK";
            os << "HTTP/1.0 200 Ok\r\n"
                  "Content-Type: text/html; charset=\"utf-8\"\r\n"
                  "Access-Control-Allow-Origin: *\r\n"
                  "\r\n"
                  "<h1>1</h1>\n";
//                      << " " << QDateTime::currentDateTime().toString() << " " << "\n";
        } else {
            // debug << "-" << "   TCP printTickets()  fail";
            os << "HTTP/1.0 200 ERROR\r\n"
                  "Content-Type: text/html; charset=\"utf-8\"\r\n"
                  "Access-Control-Allow-Origin: *\r\n"
                  "\r\n"
                  "<h1>0</h1>\n";
//                      << " " << QDateTime::currentDateTime().toString() << " " << "\n";
        }

        // debug << "-" << "  ~~~ print_tickets() out ";


    } else if (quest=="print_invents") {
        // -------------
        // print_invents
        // -------------
        int cnt  = map_params["quantity"].toInt();
        //int type = map_params["type"].toInt();

        os.setAutoDetectUnicode(true);

        if (!CO.genInventsMass(cnt)) {
            // debug << "-" << "   TCP genInventsMass()  fail";
            os << "HTTP/1.0 200 ERROR\r\n"
                  "Content-Type: text/html; charset=\"utf-8\"\r\n"
                  "Access-Control-Allow-Origin: *\r\n"
                  "\r\n"
                  "<h1>0</h1>\n";
        }
        if (CO.printInvents(cnt, print_mode)) {
            // debug << "-" << "   TCP printInvents()  OK";
            os << "HTTP/1.0 200 Ok\r\n"
                  "Content-Type: text/html; charset=\"utf-8\"\r\n"
                  "Access-Control-Allow-Origin: *\r\n"
                  "\r\n"
                  "<h1>1</h1>\n";
        } else {
            // debug << "-" << "   TCP printInvents()  fail";
            os << "HTTP/1.0 200 ERROR\r\n"
                  "Content-Type: text/html; charset=\"utf-8\"\r\n"
                  "Access-Control-Allow-Origin: *\r\n"
                  "\r\n"
                  "<h1>0</h1>\n";
        }

    } else if (quest=="print_invent") {
        // ------------
        // print_invent
        // ------------
//        int barcode = map_params["barcode"].trimmed().toInt();
//        QString identifier = QString::number(barcode);
//        int type = map_params["type"].toInt();
        QString identifier = map_params["barcode"].trimmed();

        os.setAutoDetectUnicode(true);
        if (CO.printInvent(identifier)) {
            // debug << "-" << "   TCP printInvent()  OK";
            os << "HTTP/1.0 200 Ok\r\n"
                  "Content-Type: text/html; charset=\"utf-8\"\r\n"
                  "Access-Control-Allow-Origin: *\r\n"
                  "\r\n"
                  "<h1>1</h1>\n";
//                  << " " << QDateTime::currentDateTime().toString() << " " << "\n";
        } else {
            // debug << "-" << "   TCP printInvent()  fail";
            os << "HTTP/1.0 200 ERROR\r\n"
                  "Content-Type: text/html; charset=\"utf-8\"\r\n"
                  "Access-Control-Allow-Origin: *\r\n"
                  "\r\n"
                  "<h1>0</h1>\n";
//                  << " " << QDateTime::currentDateTime().toString() << " " << "\n";
        }

    } else if (quest=="print_document") {
        // --------------
        // print_document
        // --------------
        QString json = map_params["json"];
        // debug << "-" << "### json 1" << " " << json;
//        json = json.mid(1, json.length()-2);

        if (json.left(1)=="'" && json.right(1)=="'")
            json = json.mid(1, json.size()-2);
        if (json.right(1)=="'")
            json = json.mid(0, json.size()-1);
        if (json.left(1)=="'")
            json = json.mid(1, json.size()-1);

        // // debug << "-" << "### json 2" << " " << json;


        os.setAutoDetectUnicode(true);
        if (CO.printDocument(json)) {
            // debug << "-" << "   TCP printDocument()  OK";
            os << "HTTP/1.0 200 Ok\r\n"
                  "Content-Type: text/html; charset=\"utf-8\"\r\n"
                  "Access-Control-Allow-Origin: *\r\n"
                  "\r\n"
                  "<h1>1</h1>\n";
//                  << " " << QDateTime::currentDateTime().toString() << " " << "\n";
        } else {
            // debug << "-" << "   TCP printDocument()  fail";
            os << "HTTP/1.0 200 ERROR\r\n"
                  "Content-Type: text/html; charset=\"utf-8\"\r\n"
                  "Access-Control-Allow-Origin: *\r\n"
                  "\r\n"
                  "<h1>0</h1>\n";
//                  << " " << QDateTime::currentDateTime().toString() << " " << "\n";
        }

    } else if (quest=="pass_ticket") {
        // -----------
        // pass_ticket
        // -----------
        QString barcode = map_params["barcode"];

        os.setAutoDetectUnicode(true);
        if (CO.passCard(barcode.toInt())) {
            // debug << "-" << "   TCP passCard()  OK";
            os << "HTTP/1.0 200 Ok\r\n"
                  "Content-Type: text/html; charset=\"utf-8\"\r\n"
                  "Access-Control-Allow-Origin: *\r\n"
                  "\r\n"
                  "<h1>1</h1>\n";
//                  << " " << QDateTime::currentDateTime().toString() << " " << "\n";
        } else {
            // debug << "-" << "   TCP passCard()  fail";
            os << "HTTP/1.0 200 ERROR\r\n"
                  "Content-Type: text/html; charset=\"utf-8\"\r\n"
                  "Access-Control-Allow-Origin: *\r\n"
                  "\r\n"
                  "<h1>0</h1>\n";
//                  << " " << QDateTime::currentDateTime().toString() << " " << "\n";
        }
        
    } else if (quest=="deny_ticket") {
        // -----------
        // deny_ticket
        // -----------
        QString barcode = map_params["barcode"];

        os.setAutoDetectUnicode(true);
        // теперь для изьятия билета используется метод lockCard
        // denyCard - при истечении времени катания
        if (CO.lockCard(barcode.toInt())) {
            // debug << "-" << "   TCP denyCard()  OK";
            os << "HTTP/1.0 200 Ok\r\n"
                  "Content-Type: text/html; charset=\"utf-8\"\r\n"
                  "Access-Control-Allow-Origin: *\r\n"
                  "\r\n"
                  "<h1>1</h1>\n";
                 // << " " << QDateTime::currentDateTime().toString() << " " << "\n";
        } else {
            os << "HTTP/1.0 200 ERROR\r\n"
                  "Content-Type: text/html; charset=\"utf-8\"\r\n"
                  "Access-Control-Allow-Origin: *\r\n"
                  "\r\n"
                  "<h1>0</h1>\n";
                 // << " " << QDateTime::currentDateTime().toString() << " " << "\n";
        }
        
    } else if (quest=="reset_tickets") {
        // -----------
        // reset_tickets
        // -----------
        QString code_str = map_params["code"];
        
        os.setAutoDetectUnicode(true);
        if (code_str=="reset full tickets codes") {    
            if (CO.resetTickets()) {
                // debug << "-" << "   TCP reset_tickets()  OK";
                os << "HTTP/1.0 200 Ok\r\n"
                      "Content-Type: text/html; charset=\"utf-8\"\r\n"
                      "Access-Control-Allow-Origin: *\r\n"
                      "\r\n"
                      "<h1>1</h1>\n";
            } else {
                os << "HTTP/1.0 200 ERROR\r\n"
                      "Content-Type: text/html; charset=\"utf-8\"\r\n"
                      "Access-Control-Allow-Origin: *\r\n"
                      "\r\n"
                      "<h1>0</h1>\n";
            }
        } else {
            os << "HTTP/1.0 200 WRONG CODE\r\n"
                  "Content-Type: text/html; charset=\"utf-8\"\r\n"
                  "Access-Control-Allow-Origin: *\r\n"
                  "\r\n"
                  "<h1>0</h1>\n";
        }
        
    } else if (quest=="reset_invents") {
        // -----------
        // reset_invents
        // -----------
        QString code_str = map_params["code"];
        
        os.setAutoDetectUnicode(true);
        if (code_str=="reset full inventory codes") {    
            if (CO.resetInvents()) {
                // debug << "-" << "   TCP reset_invents()  OK";
                os << "HTTP/1.0 200 Ok\r\n"
                      "Content-Type: text/html; charset=\"utf-8\"\r\n"
                      "Access-Control-Allow-Origin: *\r\n"
                      "\r\n"
                      "<h1>1</h1>\n";
            } else {
                os << "HTTP/1.0 200 ERROR\r\n"
                      "Content-Type: text/html; charset=\"utf-8\"\r\n"
                      "Access-Control-Allow-Origin: *\r\n"
                      "\r\n"
                      "<h1>0</h1>\n";
            }
        } else {
            os << "HTTP/1.0 200 WRONG CODE\r\n"
                  "Content-Type: text/html; charset=\"utf-8\"\r\n"
                  "Access-Control-Allow-Origin: *\r\n"
                  "\r\n"
                  "<h1>0</h1>\n";
        }
        
    } else {
        // ------------
        //  Не понятно
        // ------------
//        os.setAutoDetectUnicode(true);
//        debug << "-" << "   TCP TCP   XXX-((( ";
//        os << "HTTP/1.0 200 ??????\r\n"
//              "Content-Type: text/html; charset=\"utf-8\"\r\n"
//              "Access-Control-Allow-Origin: *\r\n"
//              "\r\n"
//              "<h1>Не понятно... :((</h1>\n"
//              << " " << QDateTime::currentDateTime().toString() << " " << "\n";
    }

    // Если нужно закрыть сокет
    clientSocket->close();
//    SClients.remove(idusersocs);
    debug << "-" << "  Tcp ----- ok";
    debug << "\n";  debug.flush();
}
