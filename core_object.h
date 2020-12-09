#pragma once

#include <QObject>

#include <QTimer>
#include <QSqlDatabase>
#define Q_DOCS
#include <ActiveQt/QAxBase>
#include <ActiveQt/QAxObject>
#include <QNetworkAccessManager>
#include <windows.h>

#include "db_rec.h"

#include <QSettings>
#include "waiter.h"
//#include "kiDebug.h"
#include "db_manager.h"
#include "sdk_manager.h"
#include "barcodeprinter.h"

class core_object : public QObject
{
    Q_OBJECT
    QSettings &settings;
    QTextStream &debug;

    QNetworkAccessManager man;

public:
    core_object(QSettings &settings, QTextStream &debug, QObject *parent = nullptr);
    bool debug_StringList(QString name, QStringList list);

    int timer_step;
    QTimer *main_timer = nullptr;
    waiter *m_wtr = nullptr;
    db_manager *m_DB = nullptr;
    db_manager *m_DBrent = nullptr;
    db_manager *m_PERCo = nullptr;
    sdk_manager *m_SDK = nullptr;
    BarcodePrinter *m_Printer = nullptr;
    BarcodePrinter *m_BarcodePrinter = nullptr;
    //kiDebug *m_Debug = nullptr;

    QString data_folder;

    bool openPERCo(QString *err_str = nullptr);
    bool openDB(QString *err_str = nullptr);
    void swichTimer(bool start);

//    bool genTickets(int count=100);
    
    bool resetTickets();
    bool resetInvents();
    bool genTicketsMass(int type_id, int count=100, int cardNum=-1);
    bool genInventsMass(int count=100);
    bool printTickets(int count=100, int print_mode=0);  // print_mode: 0 - раздельная печать, 1 - многостраничные документы, 2 - сплошная печать
    bool printInvents(int count=100, int print_mode=0);  // print_mode: 0 - раздельная печать, 1 - многостраничные документы, 2 - сплошная печать
    bool printInvent(QString barcode);  // print_mode: 0 - раздельная печать, 1 - многостраничные документы, 2 - сплошная печать
    bool printDocument(QString json);
    bool passCard(qint64 CardNum); // продан (статус 0 -> 1)
    bool denyCard(qint64 CardNum); // истёк (статус 2)
    bool lockCard(qint64 CardNum); // изъят (статус 3)
    bool getEvents();
    bool tstEvents(QString identifier);

    bool test_tickets_in_DB();
    bool test_tickets_in_PERCo();

signals:

public slots:
    void onTimerTick();
//    void ex(int a, QString b, QString c, QString d);
};
