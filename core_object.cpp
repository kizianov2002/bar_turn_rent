#include "core_object.h"
#include <Windows.h>
#include <QDebug>
#include <QtXml>
#include <QFile>
#include <ActiveQt/qaxbase.h>
#include <ActiveQt/qaxobject.h>

#include "PERCo.h"
#include "barcodeprinter.h"
#include "db_ticket.h"

core_object::core_object(QSettings &settings, QTextStream &debug, QObject *parent) :
    QObject(parent),
    settings(settings), debug(debug), main_timer(nullptr), m_wtr(nullptr)
{
    //m_Debug = new kiDebug();

    timer_step  = settings.value("main/timer_step_ms", "5000").toInt();
    data_folder = settings.value("main/data_folder", "c:").toString();
    m_wtr   = new waiter(debug);
    m_DB    = new db_manager("DB",    m_wtr, settings, debug/*, drvr, host, port, base, user, pass*/);
    m_DBrent= new db_manager("DBrent",m_wtr, settings, debug/*, drvr, host, port, base, user, pass*/);
    m_PERCo = new db_manager("PERCo", m_wtr, settings, debug/*, drvr, host, port, base, user, pass*/);

    m_SDK   = new sdk_manager(settings, debug);

    qDebug() << "Available printers: " << m_Printer->getAvailablePrinters();
    debug_StringList("Available printers", m_Printer->getAvailablePrinters());

    m_Printer = new BarcodePrinter("print", settings, debug, m_DB, this);
    m_Printer->configurePrinter(settings.value("print/printer", "Brother PJ-663").toString());
    m_BarcodePrinter = new BarcodePrinter("barcode", settings, debug, m_DB, this);
    m_BarcodePrinter->configurePrinter(settings.value("barcode/printer", "Brother PJ-663").toString());

    main_timer = new QTimer(this);
    main_timer->setInterval(timer_step>2000 ? timer_step : 2000);   // шаг главного таймера не меньше 2 секунд
    main_timer->setTimerType(Qt::CoarseTimer);

    connect(main_timer, SIGNAL(timeout()), this, SLOT(onTimerTick()));
    main_timer->stop();
}

bool core_object::debug_StringList(QString name, QStringList list)
{
    QString str = name + ": [\n";
    foreach (QString s, list) {
        s = s.trimmed();
        if (s.isEmpty()) continue;
        str += "    " + s + "\n";
    }
    str += "]";
   // debug << "-" << str << "\n";
   // debug.flush();
    return true;
}


bool core_object::openDB(QString *err_str)
{
    bool f_con = m_DB->connect();
    if (!f_con && err_str) {
        *(err_str) = m_DB->lastError();
    }
    bool f_con_rent = m_DBrent->connect();
    if (!f_con_rent && err_str) {
        *(err_str) = m_DBrent->lastError();
    }

    // синхронизация таблиц
    QString sql_main = "delete from ticket_type ; "
                       "delete from tariff_type ; "
                       "delete from tariff ; ";
    f_con &= m_DB->exec(sql_main);

    // TICKET_TYPE
    map_table data_rent;
    QString sql_rent = "select id, price, name "
                       "  from ticket_type ; ";
    m_DBrent->select_map(sql_rent, data_rent);

    foreach (const map_rec rec, data_rent) {
        sql_main = "insert into ticket_type(id, price, name) "
                   "values ( " + rec["id"   ] + ", "
                   "         " + rec["price"] + ", "
                   "        '" + rec["name" ] + "' )";
        f_con &= m_DB->exec(sql_main);
    }

    // TARIFF_TYPE
    map_table data_rent1;
    sql_rent = "select id, name "
               "  from tariff_type ; ";
    m_DBrent->select_map(sql_rent, data_rent);

    foreach (const map_rec rec, data_rent) {
        sql_main = "insert into tariff_type(id, name) "
                   "values ( " + rec["id"   ] + ", "
                   "        '" + rec["name" ] + "' )";
        f_con &= m_DB->exec(sql_main);
    }

    // TARIFF
    map_table data_rent2;
    sql_rent = "select id, type_id, time "
               "  from tariff ; ";
    m_DBrent->select_map(sql_rent, data_rent);

    foreach (const map_rec rec, data_rent) {
        sql_main = "insert into tariff(id, type_id, time) "
                   "values ( " + rec["id"     ] + ", "
                   "         " + rec["type_id"] + ", "
                   "        '" + rec["time"   ] + "' )";
        f_con &= m_DB->exec(sql_main);
    }

    return f_con;
}

bool core_object::openPERCo(QString *err_str)
{
    bool f_con = m_PERCo->connect();
    if (!f_con && err_str) {
        *(err_str) = m_PERCo->lastError();
    }
    return f_con;
}


void core_object::swichTimer(bool start)
{
    if (start)
        main_timer->start();
    else
        main_timer->stop();
}


bool core_object::genTicketsMass(int type_id, int count, int cardNum)
{
   // debug << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "  -  core_object::genTicketsMass()\n";
    // узнаем последнее значение identifier

    // подключение к базе PerCo
    if (m_PERCo->connect()) {
        // узнаем последнее значение tabel_id
        int okTabelId    = m_DB->selectId("select coalesce(max(cast(TABEL_ID as integer)), 0) from barcode where TABEL_ID is not NULL and TABEL_ID<>'' ; ");
        int okShablonId  = m_PERCo->selectId("select coalesce(id_shablon_main, 333) as id from SHABLON_MAIN where id_shablon_main=333 or display_NAME='Авто-шаблон' ; ");
        int okIdentifier = cardNum<1 ? m_DB->selectId("select coalesce(max(identifier_int), 66666) from barcode where identifier is not NULL and identifier_int>65535 and identifier_int<5570560 ; ") : cardNum;

        if (okTabelId<=1)    okTabelId=1;
        if (okShablonId<=1)  okShablonId=333;
        if (okIdentifier<=1) okIdentifier=66666;

        int resetFlag = settings.value("SDK/resetFlag", resetFlag).toInt();

        if ( okIdentifier==66666 && resetFlag<1 ) {
            // странная ситуация
            debug << " !!! okIdentifier = " << okIdentifier << "  resetFlag = " << " " << resetFlag << " !!! ";
            debug << " !!! Сбросить счётчик баркодов? !!! ";
            debug << "\n";  debug.flush();

            return false;
        };
        if ( okIdentifier==66666 && resetFlag>=1 ) {
            // сброс флага ресета
            settings.setValue("SDK/resetFlag", resetFlag? "1" : "0");
        };


       // debug << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "-" << "  -  genTicketsMass()  -  okTabelId" << " " << okTabelId << " " << "okIdentifier" << " " << okIdentifier << " " << "okShablonId" << " " << okShablonId;
       // debug << "\n"; // debug.flush();

        int newTabelId = okTabelId;
        int newIdentifier = okIdentifier;

        for (int i=0; i<count; i++) {
            newTabelId++;
            newIdentifier++;

            // проверка кода на корректность
            QString identStr = QString("00000000" + QString::number(newIdentifier)).right(8);
            unsigned char rem = m_Printer->calculateCheckCharacter(identStr);
            if (rem>94) {
                // недопустимое значение контрольного байта
                debug << "-" << "   wrong control byte" << " " << rem << " " << "to codestring" << " " << identStr;
                debug << "\n";  debug.flush();
                i--;
                continue;
            }

            // добавим сотрудника
           // debug << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "  -  " << " add_staff()" << " " << newTabelId << " " << "newIdentifier" << " " << newIdentifier;
           // debug << "\n"; // debug.flush();
            m_SDK->addStaff("Посетитель", QString::number(newTabelId), "", newTabelId, 1, newIdentifier, okShablonId, QDate::currentDate(), QDate::currentDate().addMonths(6));
           // debug << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "  -  " << " add_staff() - OK\n";

            // билет создан - узнаем STAFF_ID, CARD_NUM и CARD_ID
            m_PERCo->connect();
            map_rec rec;
            QString sql = "select s.id_staff as STAFF_ID, c.identifier as CARD_NUM, c.id_card as CARD_ID "
                          "  from staff_cards c "
                          "  left join staff s on(s.id_staff=c.staff_id) "
                          " where c.identifier = " + QString::number(m_SDK->saveCardNum(newIdentifier)) + " ; ";
            m_PERCo->selectOne_map(sql, rec);
            int okStaffId = rec["STAFF_ID"].toInt();
//            int okCardNum = m_SDK->Ten2Hex(rec["CARD_NUM"].toInt());
            int okCardId  = rec["CARD_ID"].toInt();
            m_PERCo->disconnect();
//           // debug << "-" << "sql" << " " << sql;

//           // debug << "-" << " ~~ add_staff()" << " " << newTabelId << " " << "newIdentifier" << " " << newIdentifier;

            // Обновим ticket_type из базы DBrent
            map_rec rec_rent;
            QString sql_rent = "select id, price, name "
                               "  from ticket_type "
                               " where id = " + QString::number(type_id) + " ; ";
            m_DBrent->selectOne_map(sql_rent, rec_rent);
            QString sql_main = "select count(id) "
                               "  from ticket_type "
                               " where id = " + QString::number(type_id) + " ; ";
            int cnt = m_DBrent->selectId(sql_main);
            bool ok = false;
            if (cnt>0) {
                QString sql_main = "update ticket_type "
                                   "   set price = " + QString(rec_rent["price"]) + ", "
                                   "       name = '" + QString(rec_rent["name"]) + "' "
                                   " where id = " + QString::number(type_id) + " ; ";
                ok = m_DBrent->selectId(sql_main);
            } else {
                QString sql_main = "insert into ticket_type (id, price, name) "
                                   "values (" + QString::number(type_id) + ", "
                                   "        " + QString(rec_rent["price"]) + ", "
                                   "       '" + QString(rec_rent["name"]) + "') ; " ;
                ok = m_DBrent->selectId(sql_main);
            }


            // внесём штрих-код в базу PG
            m_DB->exec("delete from BARCODE "
                       " where (IDENTIFIER is not NULL and IDENTIFIER='" + QString("00000000" + QString::number(newIdentifier)).right(8) + "') ; "
                       "insert into BARCODE(IDENTIFIER, IDENTIFIER_int, ticket_type_id, status, tabel_id, shablon_id, staff_id, card_id) "
                       "values('"+ QString("00000000" + QString::number(newIdentifier)).right(8) + "', "
                                 + QString::number(newIdentifier) + ", "
                                 + QString::number(type_id) + ", "
                                 " -1, "
                                 "'" + QString::number(newTabelId) + "', "
                                 + QString::number(okShablonId) + ", "
                                 + QString::number(okStaffId) + ", "
                                 + QString::number(okCardId) + ") ; ");
//            m_DB->exec( "update BARCODE "
//                        "   set IDENTIFIER_int = " + QString::number(newIdentifier) + ", "
//                        "       tabel_id = '" + QString::number(newTabelId) + "', "
//                        "       shablon_id = " + QString::number(okShablonId) + ", "
//                        "       staff_id = " + QString::number(okStaffId) + ", "
//                        "       card_id = " + QString::number(okCardId) + " "
//                        " where IDENTIFIER='" + QString("00000000" + QString::number(newIdentifier)).right(8) + "' ; " );

            m_DBrent->exec("delete from BARCODE "
                       " where (IDENTIFIER is not NULL and IDENTIFIER='" + QString("00000000" + QString::number(newIdentifier)).right(8) + "') ; "
                       "insert into BARCODE(IDENTIFIER, IDENTIFIER_int, ticket_type_id, status) "
                       "values('"+ QString("00000000" + QString::number(newIdentifier)).right(8) + "', "
                                 + QString::number(newIdentifier) + ", "
                                 + QString::number(type_id) + ", "
                                 " -1) ; ");
//            m_DBrent->exec("update BARCODE "
//                           "   set IDENTIFIER_int = " + QString::number(newIdentifier) + ", "
//                           "       ticket_type_id = " + QString::number(type_id) + ", "
//                           "       status = -1 "
//                           " where IDENTIFIER='" + QString("00000000" + QString::number(newIdentifier)).right(8) + "' ; ");
            debug << "-" << "   gen ticket" << " " << newIdentifier;
            debug << "\n";  debug.flush();
        }

    } else {
        //debug << "-" << "!!! database PERCo is too buisy !!!";
        //debug << "\n";  debug.flush();
        return false;
    }

    m_PERCo->disconnect();

    // отправим билеты на контроллер
   // debug << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "  -  " << " m_SDK->saveAllCards()";
   // debug << "\n"; // debug.flush();
    m_SDK->saveAllCards();

    return true;
}

bool core_object::genInventsMass(int count)
{
    // узнаем последнее значение identifier для инвентаря
    // !!! family=85, то есть больше/равно 5570560 !!!

    // узнаем последнее значение tabel_id
    int okIdentifier = m_DB->selectId("select coalesce(max(identifier_int), 5570560) as ID "
                                      "  from public.barcode_inventory "
                                      " where identifier_int>=5570560 ; ");
    int newIdentifier = okIdentifier;
    for (int i=0; i<count; i++) {
        newIdentifier++;

        // проверка кода на корректность
        QString identStr = QString("00000000" + QString::number(newIdentifier)).right(8);
        unsigned char rem = m_Printer->calculateCheckCharacter(identStr);
        //debug << "-" << "  identStr -" << " " << identStr << " " << "  encoded -" << " " << encoded << " " << "length" << " " << encoded.length();
        if (rem>94) {
            // недопустимое значение контрольного байта
            debug << "-" << "   wrong control byte" << " " << rem << " " << "to codestring" << " " << identStr;
            debug << "\n";  debug.flush();
            i--;
            continue;
        }

        // внесём штрих-код в базу PG
        m_DB->exec("delete from barcode_inventory "
                   " where (IDENTIFIER is not NULL and IDENTIFIER='" + QString("00000000" + QString::number(newIdentifier)).right(8) + "') ; "
                   "insert into barcode_inventory(IDENTIFIER, status) "
                   "values('" + QString("00000000" + QString::number(newIdentifier)).right(8) + "', -2) ; ");
        m_DBrent->exec("delete from barcode_inventory "
                   " where (IDENTIFIER is not NULL and IDENTIFIER='" + QString("00000000" + QString::number(newIdentifier)).right(8) + "') ; "
                   "insert into barcode_inventory(IDENTIFIER) "
                   "values('" + QString("00000000" + QString::number(newIdentifier)).right(8) + "') ; ");
       // debug << "-" << "   gen invent" << " " << newIdentifier;
       // debug << "\n"; // debug.flush();
    }

    return true;
}


bool core_object::printTickets(int count, int print_mode) {
    // если билетов не хватает - напечатаем что есть

//    // подсчитаем НЕ РАСПЕЧАТАННЫЕ ШТРИХ-КОДЫ билетов
//    QString sql_cnt = "select count(*) "
//                      "  from public.barcode "
//                      " where status=-1 ; ";
//    int cnt = 0;
//    while ((cnt = m_DB->selectId(sql_cnt)) < count) {
//        // облом - не хватает готовых штрих-кодов
//        // добавим
//        if (!genTicketsMass(20)) {
//            // облом - формирование штрих-кодов не сработало
//            return false;
//        }
//    }

//    // ПОЛУЧИМ тип билетов
//    QString sql_typ = "select name "
//                      "  from public.tariff_type "
//                      /*" where id==" + QString::number(type) + " ; "*/;
//    db_rec rec;
//    m_DB->selectOne(sql_typ, rec);
//    QString type_str = rec[0];

    // ПОЛУЧИМ НЕ РАСПЕЧАТАННЫЕ ШТРИХ-КОДЫ билетов
    QString sql_sel = "select b.identifier, t.name as type_str "
                      "  from public.barcode b "
                      "  left join ticket_type t on (b.ticket_type_id=t.id) "
                      " where status=-1"
                      " limit " + QString::number(count) + " ; ";
    map_table data;
    try {
        m_DB->select_map(sql_sel, data);

    } catch (...) {
        debug << "-" << "catch (...) *";
        debug << "\n";  debug.flush();
    }

    // инициализация печати
    switch (print_mode) {
    case 1:
        if (!m_BarcodePrinter->printTcStack_start())
            return false;
        break;
    case 2:
        if (!m_BarcodePrinter->printTcStrip_start())
            return false;
        break;
    default:
        break;
    }

    int i=0;
    // последовательная печать
    for (i=0; i<data.size(); i++) {
//       // debug << "-" << "i" << " " << i;
        map_rec &rec = data[i];
//       // debug << "-" << "rec" << " " << rec;
//       // debug << "\n"; // debug.flush();
        QString identifier = rec["identifier"];
        QString type_str   = rec["type_str"];

        switch (print_mode) {
        case 1:
            m_BarcodePrinter->printTcStack(identifier, type_str);
            break;
        case 2:
            m_BarcodePrinter->printTcStrip(identifier, type_str);
            break;
        default:
            m_BarcodePrinter->printTicket(identifier, type_str);
            break;
        }

        // запомним факт печати
        QString sql_sav = "update public.barcode "
                          "   set status = 1 "
                          " where identifier = '" + identifier + "' ; ";

        m_DB->exec(sql_sav);
        debug << "-" << "   print ticket" << " " << identifier;
        debug << "\n";  debug.flush();
    }

    // окончание печати
    switch (print_mode) {
    case 1:
        m_BarcodePrinter->printTcStack_end();
        break;
    case 2:
        m_BarcodePrinter->printTcStrip_end();
        break;
    default:
        break;
    }

    debug << "-" << " ~~~ print tickets" << " " << i;
    debug << "\n";  debug.flush();
    return true;
}

bool core_object::printInvents(int count, int print_mode) {
    // если билетов не хватает - напечатаем что есть

//    // подсчитаем НЕ РАСПЕЧАТАННЫЕ ШТРИХ-КОДЫ билетов
//    QString sql_cnt = "select count(*) "
//                      "  from public.barcode_inventory "
//                      " where status=-2 ; ";
//    int cnt = 0;
//    while ((cnt = m_DB->selectId(sql_cnt)) < count) {
//        // облом - не хватает готовых штрих-кодов
//        // добавим
//        if (!genInventsMass(20)) {
//            // облом - формирование штрих-кодов не сработало
//            return false;
//        }
//    }

    // ПОЛУЧИМ НЕ РАСПЕЧАТАННЫЕ ШТРИХ-КОДЫ инвентаря
    QString sql_sel = "select identifier "
                      "  from public.barcode_inventory "
                      " where status=-2"
                      " limit " + QString::number(count) + " ; ";
    map_table data;
    try {
        m_DB->select_map(sql_sel, data);

    } catch (...) {
        debug << "-" << "catch (...) *";
        debug << "\n";  debug.flush();
    }

    // инициализация печати
    switch (print_mode) {
    case 1:
        if (!m_BarcodePrinter->printTcStack_start())
            return false;
        break;
    case 2:
        if (!m_BarcodePrinter->printTcStrip_start())
            return false;
        break;
    default:
        break;
    }

    int i=0;
    // последовательная печать
    for (i=0; i<data.size(); i++) {
        map_rec &rec = data[i];
        QString identifier = rec["identifier"];

        switch (print_mode) {
        case 1:
            m_BarcodePrinter->printTcStack(identifier, "(инвентарь)");
            break;
        case 2:
            m_BarcodePrinter->printTcStrip(identifier, "(инвентарь)");
            break;
        default:
            m_BarcodePrinter->printTicket(identifier, "(инвентарь)");
            break;
        }

        // запомним факт печати
        QString sql_sav = "update public.barcode_inventory "
                          "   set status = 2 "
                          " where identifier = '" + identifier + "' ; ";
        m_DB->exec(sql_sav);
        debug << "-" << "   print invent" << " " << identifier;
        debug << "\n";  debug.flush();
    }

    // окончание печати
    switch (print_mode) {
    case 1:
        m_BarcodePrinter->printTcStack_end();
        break;
    case 2:
        m_BarcodePrinter->printTcStrip_end();
        break;
    default:
        break;
    }

   // debug << "-" << " ~~~ print tickets" << " " << i;
   // debug << "\n"; // debug.flush();
    return true;
}

bool core_object::printInvent(QString identifier) {
    m_BarcodePrinter->printTicket(identifier, "(инвентарь)");

    debug << "-" << " ~~~ printInvent" << " " << identifier;
    debug << "\n";  debug.flush();
    return true;
}

bool core_object::printDocument(QString json) {
    return m_Printer->printDocument(json, 1);
}


bool core_object::passCard(qint64 cardNum)
{
    QDateTime t_pick = QDateTime::currentDateTime();
    QDateTime t_pck2 = t_pick.addSecs(1);

    // подключение к базе PerCo
    if (m_PERCo->connect())
    {
        // узнаем подробные данные карты
        qint64 cardNum_cor = m_SDK->saveCardNum(cardNum);
        int StaffID = m_PERCo->selectId("select STAFF_ID from STAFF_CARDS where IDENTIFIER=" + QString::number(cardNum_cor) + " ; ");
        int CardID  = m_PERCo->selectId("select ID_CARD  from STAFF_CARDS where IDENTIFIER=" + QString::number(cardNum_cor) + " ; ");

        bool res_q = m_PERCo->exec("update STAFF_CARDS "
                                   "   set PROHIBIT=0 "
                                   " where IDENTIFIER=" + QString::number(cardNum_cor) + " ; ");
        debug << "-" << "PASS card in DB " << " " << cardNum << " " << res_q;
        debug << "\n";  debug.flush();

        m_SDK->passCard(StaffID, cardNum, CardID);

        // текущий тариф
        db_table qData;
        QString sql_tariff = "select t.id, t.type_id, t.time, "
                             "       (TIMESTAMP WITH TIME ZONE 'epoch' + (cast(t.time as bigint)) * INTERVAL '1 second') as date_time"
                             "  from tariff t "
                             " where t.id = (select max(tt.id) from tariff tt) ; ";
        m_DBrent->select(sql_tariff, qData);

       // debug << "*** NEW TICKET *** " << "\n";

        // срок действия билета
        QString finished_at = "NULL";
        int tariff_id = qData[0].at(0).toInt();
       // debug << "tariff_id = " << QString::number(tariff_id) << "\n";
        int type_id = qData[0].at(1).toInt();
       // debug << "type_id = " << QString::number(type_id) << "\n";
        QString time = qData[0].at(2);
       // debug << "time = " << time << "\n";
        QString date_time =  qData[0].at(3);
       // debug << "date_time = " << date_time << "\n";
        if (type_id>1) {
            // зададим срок действия
            if (time.length()<5) {
                // относительно момента продажи
                finished_at = "'" +QDateTime::currentDateTime().addSecs(time.toInt()*60).toString("yyyy-MM-dd hh:mm:ss") + "'";
            } else {
                // срок до конца работы катка
                finished_at = "'" + date_time + "'";
            }
        } else {
            // срок действия не известен до первого прохода
            finished_at = "NULL";
        }
       // debug << "finished_at = " << finished_at << "\n";

        // обновим тариф
        sql_tariff = "delete from tariff where id=" + QString::number(tariff_id) + " ; "
                     "insert into tariff (id, type_id, time) "
                     "values (" + QString::number(tariff_id) + ", " + QString::number(type_id) + ", '" + time + "') ; ";
        bool res_tariff = m_DB->exec(sql_tariff);

        // запишем в базу факт продажи билета
        QString identifier = QString("00000000" + QString::number(cardNum)).right(8);
        QString sql = "update barcode "
                      "   set status = 1 "
                      " where identifier = '" + identifier + "' "
                     // "   and status<1"
                      " ; "

                      "delete from ticket"
                      " where (barcode_id is not NULL and barcode_id in "
                      "       (select id from barcode where identifier='" + identifier + "')); "

                      "insert into ticket (barcode_id, tariff_id, _status, created_at, started_at, finished_at, prohibit) "
                      "values("
                      "       (select id from barcode where identifier='" + identifier + "'), "
                      "       " + QString::number(tariff_id) + ", "
                      "       0, "
                      "       '" + t_pick.toString("yyyy-MM-dd hh:mm:ss") + "', "
                      "       " + (type_id==1 ? " NULL " : ("'" + t_pck2.toString("yyyy-MM-dd hh:mm:ss") + "'" ) ) + ", "
                      "       " + finished_at + ", "
                      "       0 "
                      "      ) "
                      "returning id; ";
        int id = m_DB->selectId(sql);

        // DBrent
//        sql = "update barcode "
//              "   set status = 1"
//              " where identifier = '" + identifier + "' "
//              "   and status<1 ; "

//              "delete from ticket"
//              " where (barcode_id is not NULL and barcode_id in "
//              "       (select id from barcode where identifier='" + identifier + "') ); "

//              "insert into ticket (barcode_id, tariff_id, _status, created_at, started_at) "
//              "values("
//              "       (select id from barcode where identifier='" + identifier + "'), "
//              "       " + QString::number(tariff_id) + ", "
//              "       0, "
//              "       now(), "
//              "       now() "
//              "      ) ; ";

        sql = "update barcode "
              "   set status = 1"
              " where identifier = '" + identifier + "' "
             // "   and status<1 "
              " ; "

              "update ticket "
              "   set _status = 0, "
              "       tariff_id = " + QString::number(tariff_id) + ", "
              "       created_at = '" + t_pick.toString("yyyy-MM-dd hh:mm:ss") + "', "
              "       started_at = '" + t_pck2.toString("yyyy-MM-dd hh:mm:ss") + "' "
              " where barcode_id in (select id from barcode where identifier='" + identifier + "') "
              "   and barcode_id is not NULL ; ";
        m_DBrent->exec(sql);
    } else {
        //debug << "-" << "!!! database PERCo is too buisy !!!";
        //debug << "\n";  debug.flush();
        return false;
    }

    m_PERCo->disconnect();

    // отправим билеты на контроллер
    m_SDK->saveAllCards();

    return true;
}

bool core_object::denyCard(qint64 cardNum)
{
    // подключение к базе PerCo
    if (m_PERCo->connect())
    {
        // узнаем подробные данные карты
        int StaffID = m_PERCo->selectId("select STAFF_ID from STAFF_CARDS where IDENTIFIER=" + QString::number(m_SDK->saveCardNum(cardNum)) + " ; ");
        int CardID  = m_PERCo->selectId("select ID_CARD  from STAFF_CARDS where IDENTIFIER=" + QString::number(m_SDK->saveCardNum(cardNum)) + " ; ");

        bool res_q = m_PERCo->exec("update STAFF_CARDS "
                                   "   set PROHIBIT=1 "
                                   " where IDENTIFIER=" + QString::number(m_SDK->saveCardNum(cardNum)) + " ; ");
        debug << "-" << "DENY card in DB " << " " << cardNum << " " << res_q;
        debug << "\n";  debug.flush();

        m_SDK->denyCard(StaffID, cardNum, CardID);

        // запишем в базу факт блокировки билета
        QString identifier = QString("00000000" + QString::number(cardNum)).right(8);
        QString sql = "update barcode "
                      "   set status = 2 "
                      " where identifier = '" + identifier + "' ; "

                      "update ticket "
                      "   set _status = 2 "
                      " where barcode_id in (select id from barcode where identifier='" + identifier + "') ; ";
        m_DB->exec(sql);

        // DBrent
        m_DBrent->exec(sql);
    } else {
        //debug << "-" << "!!! database PERCo is too buisy !!!";
        //debug << "\n"; // debug.flush();
        return false;
    }

    m_PERCo->disconnect();

    // отправим билеты на контроллер
    m_SDK->saveAllCards();

    return true;
}

bool core_object::lockCard(qint64 cardNum)
{
    // подключение к базе PerCo
    if (m_PERCo->connect())
    {
        // узнаем подробные данные карты
        int StaffID = m_PERCo->selectId("select STAFF_ID from STAFF_CARDS where IDENTIFIER=" + QString::number(m_SDK->saveCardNum(cardNum)) + " ; ");
        int CardID  = m_PERCo->selectId("select ID_CARD  from STAFF_CARDS where IDENTIFIER=" + QString::number(m_SDK->saveCardNum(cardNum)) + " ; ");

        bool res_q = m_PERCo->exec("update STAFF_CARDS "
                                   "   set PROHIBIT=1 "
                                   " where IDENTIFIER=" + QString::number(m_SDK->saveCardNum(cardNum)) + " ; ");
        debug << "-" << "DENY card in DB " << " " << cardNum << " " << res_q;
        debug << "\n";  debug.flush();

        m_SDK->denyCard(StaffID, cardNum, CardID);

        // запишем в базу факт блокировки билета
        QString identifier = QString("00000000" + QString::number(cardNum)).right(8);
        QString sql = "update barcode "
                      "   set status = 3 "
                      " where identifier = '" + identifier + "' ; "

                      "update ticket "
                      "   set _status = 3 "
                      " where barcode_id in (select id from barcode where identifier='" + identifier + "') ; ";
        m_DB->exec(sql);

        // DBrent
        m_DBrent->exec(sql);
    } else {
        //debug << "-" << "!!! database PERCo is too buisy !!!";
        //debug << "\n"; // debug.flush();
        return false;
    }

    m_PERCo->disconnect();

    // отправим билеты на контроллер
    m_SDK->saveAllCards();

    return true;
}

bool core_object::resetTickets()
{
    // подключение к базе PerCo
    debug << "-" << " - Reset STAFF_CARDS in DB -";
    debug << "\n";  debug.flush();
    if (m_PERCo->connect())
    {
        // узнаем подробные данные карты
        m_PERCo->exec("delete from STAFF ; ");
        m_PERCo->exec("delete from STAFF_REF ; ");
        m_PERCo->exec("delete from STAFF_CARDS ; ");
        m_DB->exec("delete from ticket ; ");
        m_DB->exec("delete from barcode ; ");

        // отправим билеты на контроллер
        m_SDK->saveAllCards();
        debug << "-" << " - OK - ";
        debug << "\n";  debug.flush();
    } else {
        //debug << "-" << "!!! database PERCo is too buisy !!!";
        //debug << "\n";  debug.flush();
        return false;
    }

    m_PERCo->disconnect();
    return true;
}

bool core_object::resetInvents()
{
    // подключение к базе PerCo
    debug << "-" << " - Reset Inventory in DB -";
    debug << "\n";  debug.flush();
    m_DB->exec("delete from public.order ; ");
    m_DB->exec("delete from barcode_inventory ; ");
    m_DB->exec("delete from inventory ; ");
    m_DB->exec("delete from inventory_tariffs ; ");
    m_DB->exec("delete from inventory_tariffs_inventory ; ");
    m_DB->exec("delete from rent_inventory ; ");
    m_DB->exec("delete from rent_inventory_tariff ; ");
    debug << "-" << " - OK - ";
    debug << "\n";  debug.flush();

    return true;
}


void core_object::onTimerTick() {
    //// debug << "-" << "onTimerTick()";
    // debug << "|- "; // debug.flush();
    // подключение к базе PerCo
    if (m_PERCo->connect())
    {
        int maxIdStaff = m_PERCo->selectId("select max(ID_STAFF) as ID from STAFF ; ");
        int maxIdTickt = m_PERCo->selectId("select max(ID_CARD) as ID from STAFF_CARDS ; ");
        int maxIdentifier = m_SDK->readCardNum(m_PERCo->selectId("select max(IDENTIFIER) as ID from STAFF_CARDS ; "));

//       // debug << "-" << "maxIdStaff" << " " << maxIdStaff << " " << "maxIdTickt" << " " << maxIdTickt << " " << "maxIdentifier" << " " << maxIdentifier;
//       // debug << "\n"; // debug.flush();

        // сравним полученные максимальные с теми что есть в нашей базе
        int okIdStaff = m_DB->selectId("select value as ID from db_data where name='maxIdStaff' ; ");
        int okIdTickt = m_DB->selectId("select value as ID from db_data where name='maxIdTickt' ; ");
        int okIdentifier = m_DB->selectId("select value as ID from db_data where name='maxIdentifier' ; ");

        //// debug << "-" << "okIdStaff" << " " << okIdStaff << " " << "okIdTickt" << " " << okIdTickt << " " << "okIdentifier" << " " << okIdentifier;

        if (okIdStaff<maxIdStaff) {
            // новые данные персонала
            db_table qData;
            bool res_q = m_PERCo->select("select ID_STAFF, TABEL_ID, LAST_NAME, FIRST_NAME, MIDDLE_NAME "
                                         "  from STAFF "
                                         " where ID_STAFF>" + QString::number(okIdStaff) +
                                         "   and TABEL_ID is not NULL "
                                         "   and TABEL_ID<>'' "
                                         " order by ID_STAFF ;",
                                         qData);

            //// debug << "-" << qData;

            // догрузим данные билетов
            for (int i=0; i<qData.size(); i++) {
                bool res = m_DB->exec("delete from TURNSIDE.STAFF "
                                      " where (ID_STAFF is not NULL and ID_STAFF=" + qData[i].at(0) + ") "
                                      "    or (TABEL_ID is not NULL and TABEL_ID=" + qData[i].at(1) + ") ; "
                                      "insert into TURNSIDE.STAFF(ID_STAFF, TABEL_ID, LAST_NAME, FIRST_NAME, MIDDLE_NAME) "
                                      "values (" + qData[i].at(0) + ", " + qData[i].at(1) + ", '" + qData[i].at(2) + "', '" + qData[i].at(3) + "', '" + qData[i].at(4) + "') ; ");
                if (!res)
                    debug << "-" << "ошибка при копировании данных новых персонала";
               // debug << "-" << res;
               // debug << "\n"; // debug.flush();
            }

            // запомним последний ID
            bool res1 = m_DB->exec("delete from DB_DATA where name='maxIdStaff' ; "
                                   "insert into DB_DATA(name, value) "
                                   "values ('maxIdStaff', '" + QString::number(maxIdStaff) + "') ; ");
            if (!res1)
                debug << "-" << "ошибка при сохранении ID последнего билета";
           // debug << "-" << res1;
           // debug << "\n"; // debug.flush();
        }


        if (okIdTickt<maxIdTickt) {
            // новые данные персонала
            db_table qData;
            bool res_q = m_PERCo->select("select ID_CARD, STAFF_ID, IDENTIFIER, PROHIBIT "
                                         "  from STAFF_CARDS "
                                         " where ID_CARD>" + QString::number(okIdTickt) +
                                         " order by ID_CARD ;",
                                         qData);
            //// debug << "-" << res_q;

            // догрузим данные билетов
            for (int i=0; i<qData.size(); i++) {
                qint64 identifier = m_SDK->readCardNum(qData[i].at(2).toInt());
                bool res = m_DB->exec("delete from TURNSIDE.TICKETS "
                                      " where (ID_CARD is not NULL and ID_CARD=" + qData[i].at(0) + ") " +
                                      "    or (IDENTIFIER is not NULL and IDENTIFIER=" + QString::number(identifier) + ") ; "
                                      "insert into TURNSIDE.TICKETS(ID_CARD, ID_STAFF, IDENTIFIER, PROHIBIT) "
                                      "values (" + qData[i].at(0) + ", " + qData[i].at(1) + ", " + QString::number(identifier) + ", " + qData[i].at(3) + ") ; ");
                if (!res)
                    debug << "-" << "ошибка при копировании данных новых персонала";
//               // debug << "-" << res;
               // debug << "\n"; // debug.flush();
            }

            // запомним последний ID
            bool res1 = m_DB->exec("delete from DB_DATA where name='maxIdTickt' ; "
                                   "insert into DB_DATA(name, value) "
                                   "values ('maxIdTickt', '" + QString::number(maxIdTickt) + "') ; ");
            if (!res1)
                debug << "-" << "ошибка при сохранении ID последнего билета";
            //// debug << "-" << res1;
           // debug << "\n"; // debug.flush();
        }


        if (okIdentifier<maxIdentifier) {
            // поправим максимальный идентификатор карты
            bool res1 = m_DB->exec("delete from DB_DATA where name='maxIdentifier' ; "
                                   "insert into DB_DATA(name, value) "
                                   "values ('maxIdentifier', '" + QString::number(maxIdentifier) + "') ; ");
            if (!res1)
                debug << "-" << "ошибка при сохранении максимального идентификатора";
           // debug << "\n"; // debug.flush();
        }

    } else {
        //debug << "-" << "!!! database PERCo is too buisy !!!";
        //debug << "\n"; // debug.flush();
    }

    m_PERCo->disconnect();

    // проверим статусы билетов
    bool res1 = test_tickets_in_DB();

    // проверим проходы через турникет
    bool res2 = getEvents();
  //  bool res3 = tstEvents("00066739");

}


bool core_object::getEvents() {
//    QString fname = m_SDK->getEvents(QDate::currentDate().addDays(-1), QDate::currentDate().addDays(-1));
    QString fname = m_SDK->getEvents(QDate::currentDate(), QDate::currentDate().addDays(1));

    QDomDocument doc("events");
    QFile file(fname);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    if (!doc.setContent(&file)) {
        file.close();
        return false;
    }
    file.close();

    QDomElement docElem = doc.documentElement();
    QDomNode dc = doc.firstChild();
    while (!dc.isNull()) {

        if (dc.isElement()) {
            QDomElement ers=dc.toElement(); // try to convert the node to an element.
            for(QDomNode n = ers.firstChild(); !n.isNull(); n = n.nextSibling()) {
                QString name_er = n.nodeName();
                if (name_er=="eventsreport") {
                    QDomElement er=n.toElement();

                    if (n.isElement()) {
                        QDomElement ems=n.toElement(); // try to convert the node to an element.
                        for(QDomNode n = ems.firstChild(); !n.isNull(); n = n.nextSibling()) {
                            QString name_em = n.nodeName();
                            if (name_em=="employ") {
                                QDomElement em=n.toElement();

                                QString id_staff_external=em.attribute("id_staff_external");
                                QString id_staff_internal=em.attribute("id_staff_internal");
                                QString tab_n=em.attribute("tab_n");
                                QString fio=em.attribute("fio");
                                QString appoint_id_external=em.attribute("appoint_id_external");
                                QString subdiv_id_external=em.attribute("subdiv_id_external");
                                QString appoint_id_internal=em.attribute("appoint_id_internal");
                                QString subdiv_id_internal=em.attribute("subdiv_id_internal");
                                QString appoint_name=em.attribute("appoint_name");

                                if (n.isElement()) {
                                    QDomElement evs=n.toElement(); // try to convert the node to an element.
                                    for(QDomNode n = evs.firstChild(); !n.isNull(); n = n.nextSibling()) {
                                        QString name_ev = n.nodeName();
                                        if (name_ev=="events") {
                                            QDomElement ev=n.toElement();

                                            QString id_staff_external=ev.attribute("id_staff_external");
                                            QString id_staff_internal=ev.attribute("id_staff_internal");
                                            QString datetimeevent=ev.attribute("datetimeevent");
                                            QString typepass=ev.attribute("typepass");
                                            QString areas_name=ev.attribute("areas_name");
                                            QString reader_name=ev.attribute("reader_name");
                                            QString registred_area=ev.attribute("registred_area");
                                            QString participates_calculation=ev.attribute("participates_calculation");

                                            // фильтрация по дате и времени
                                            QString date_s = datetimeevent.left(10);
                                            QString time_s = datetimeevent.right(8);
                                            QDateTime dt_ev = QDateTime::fromString(date_s + " " + time_s, "dd.MM.yyyy hh:mm:ss").addSecs(-10800);
                                            date_s = dt_ev.toString("dd.MM.yyyy");
                                            time_s = dt_ev.toString("hh:mm:ss");
//                                            QDate date_ev = QDate::fromString(date_s, "dd.MM.yyyy");
//                                            QTime time_ev = QTime::fromString(time_s, "hh:mm:ss");
//                                           // debug << "-" << " fio " << " " << fio;
//                                           // debug << "-" << "datetimeevent" << " " << datetimeevent;
//                                           // debug << "-" << "date_s" << " " << date_s;
//                                           // debug << "-" << "date_ev" << " " << date_ev;
//                                           // debug << "-" << "time_ev" << " " << time_ev;
                                            if ( dt_ev.date()==QDate::currentDate()                      // только сегодняшние события
                                                /* && time_ev>QTime::currentTime().addSecs(-3600)*/ ) { // события не старше 1 часа

                                                // проверим наличие такой записи
                                                QString sql = "select count(*) "
                                                              "  from events "
                                                              " where id_staff_internal=" + id_staff_internal + " "
                                                              "   and date_ev='" + date_s + "' "
                                                              "   and time_ev='" + time_s + "' ";
//                                               // debug << "-" << sql;
//                                               // debug << "\n"; // debug.flush();
                                                int cnt = m_DB->selectId(sql);
                                                if (cnt<1) {
                                                    // дорбавим событие в базу на PG
                                                    map_rec map;
                                                    map.addI("id_staff_internal", id_staff_internal.toInt());
                                                    map.addI("tab_n", tab_n.toInt());
                                                    map.addS("fio", fio);
                                                    map.addS("date_ev", date_s);
                                                    map.addS("time_ev", time_s);
                                                    map.addI("subdiv_id_internal", subdiv_id_internal.toInt());
                                                    map.addI("appoint_id_internal", appoint_id_internal.toInt());
                                                    map.addS("areas_name", areas_name);
                                                    map.addS("reader_name", reader_name);
                                                    map.addS("typepass", typepass);
                                                    bool res = m_DB->insert_map("events", map);

                                                    if (!res) {
                                                       // debug << "-" << "ошибка при добавлении события" << " " << map.toString();
                                                       // debug << "\n"; // debug.flush();
                                                    }

                                                    // обработаем новое событие прохода черех турникет

                                                    // -------------------------------
                                                    // прочитаем билеты из базы данных
                                                    // -------------------------------
                                                    QString sql = "select t.id, b.identifier, b.family, b.number, b.staff_id, b.card_id, "
                                                                  "       t._status as status, t.prohibit, "
                                                                  "       (t.created_at  /*+ time '3:00:00'*/) as created_at, "
                                                                  "       (t.started_at  /*+ time '3:00:00'*/) as started_at, "
                                                                  "       (t.finished_at /*+ time '3:00:00'*/) as finished_at, "
                                                                  "       tt.type_id as tariff_type, tt.time as time_min, "
                                                                  "       (TIMESTAMP WITH TIME ZONE 'epoch' + (cast(tt.time as bigint)) * INTERVAL '1 second') as time_to, "
                                                                  "       s.setting, s.value "
                                                                  "  from ticket t "
                                                                  "  left join tariff tt on(tt.id=t.tariff_id) "
                                                                  "  left join barcode b on(b.id=t.barcode_id) "
                                                                  "  left join settings s on(s.setting='tariff' and s.is_act=0) "
                                                                  " where b.staff_id=" + id_staff_internal + " "
                                                                  "   and t._status in (0,1) "
                                                                  "   and tt.type_id in (1,2,3) "
                                                                  "   and t.created_at is not NULL ; ";
//                                                   // debug << "-" << sql;
                                                   // debug << "\n"; // debug.flush();
                                                    map_rec rec;
                                                    m_DB->selectOne_map(sql, rec);


                                                    // если данные есть (такой билет есть и продан) ...
                                                    if (rec.size()>0) {
                                                        db_ticket ticket(rec);

                                                       // debug << "\n !!! ";
                                                       // debug << "\n-" << " ** ID" << " " << ticket.identifier;
                                                       // debug << "\n-" << "has_create" << " " << ticket.has_create;
                                                       // debug << "\n-" << "tm_create" << " " << ticket.tm_create.toString("yyyy-MM-dd hh:mm:ss");
                                                       // debug << "\n-" << "has_start" << " " << ticket.has_start;
                                                       // debug << "\n-" << "tm_start" << " " << ticket.tm_start.toString("yyyy-MM-dd hh:mm:ss");
                                                       // debug << "\n-" << "has_finish" << " " << ticket.has_finish;
                                                       // debug << "\n-" << "tm_finish" << " " << ticket.tm_finish.toString("yyyy-MM-dd hh:mm:ss");
                                                       // debug << "\n-" << "time_min" << " " << ticket.time_min;
                                                       // debug << "\n-" << "time_to" << " " << ticket.time_to.toString("hh:mm:ss");
                                                       // debug << "\n >>> ";
                                                       // debug << "\n"; // debug.flush();

                                                        ticket.f_change = false;
                                                        ticket.f_chPass = false;

                                                        // --------------------------------
                                                        // проверка/задание времени катания
                                                        if (ticket.tariff==1) {
                                                            // разрешаем проход на каток
                                                            // время катания начинается прямо сечас и закончится через time_min минут
                                                            ticket.set_pass(true);

                                                            if (!ticket.has_start) {
                                                                ticket.tm_start = QDateTime::currentDateTime();
                                                                ticket.has_start = true;
                                                                ticket.tm_finish = ticket.tm_start.addSecs(ticket.time_min *60/* + 10800*/);
                                                                ticket.has_finish = true;
                                                                ticket.f_change = true;
                                                            }
                                                            if (!ticket.has_finish) {
                                                                ticket.tm_finish = ticket.tm_start.addSecs(ticket.time_min *60/* + 10800*/);
                                                                ticket.has_finish = true;
                                                                ticket.f_change = true;
                                                            }
                                                        }
                                                        else if (ticket.tariff==2) {
                                                            // разрешаем проход на каток
                                                            // время катания начинается прямо сечас и закончится через time_min минут
                                                            ticket.set_pass(true);

                                                            if (!ticket.has_start) {
                                                                ticket.tm_start = QDateTime::currentDateTime();
                                                                ticket.has_start = true;
                                                                ticket.tm_finish = ticket.tm_start.addSecs(ticket.time_min *60/* + 10800*/);
                                                                ticket.has_finish = true;
                                                                ticket.f_change = true;
                                                            }
                                                            if (!ticket.has_finish) {
                                                                ticket.tm_finish = ticket.tm_start.addSecs(ticket.time_min *60/* + 10800*/);
                                                                ticket.has_finish = true;
                                                                ticket.f_change = true;
                                                            }
                                                        }
                                                        else if (ticket.tariff==3) {
                                                            // разрешаем проход на каток
                                                            // время катания начинается прямо сечас и продолжится до вечера
                                                            ticket.set_pass(true);

                                                            if (!ticket.has_start) {
                                                                ticket.tm_start = QDateTime::currentDateTime();
                                                                ticket.has_start = true;
                                                                ticket.tm_finish = QDateTime(QDate::currentDate(), ticket.time_to);
                                                                ticket.has_finish = true;
                                                                ticket.f_change = true;
                                                            }
                                                            if (!ticket.has_finish) {
                                                                ticket.tm_finish = QDateTime(QDate::currentDate(), ticket.time_to);
                                                                ticket.has_finish = true;
                                                                ticket.f_change = true;
                                                            }
                                                        }


                                                        // -------------------------------------------------
                                                        // если это первый проход - активируем время катания
                                                        if (ticket.status==0) {
                                                            ticket.status = 1;
                                                            ticket.f_change = true;
                                                        }


                                                        // --------------------------------
                                                        // контроль срока действия билета
                                                        if (!ticket.has_finish) {
                                                            // срок катания ещё не известен
                                                        }
                                                        else if (ticket.tm_finish > QDateTime::currentDateTime()) {
                                                            // время не истекло - пусть катается
                                                        }
                                                        else if (ticket.tm_finish <= QDateTime::currentDateTime()) {
                                                            // время истекло - стоп билета
                                                            ticket.set_pass(false);
                                                            ticket.status = 2;
                                                        }

                                                        // ------------------------------------------------------
                                                        // добавим данные визита
                                                        QString sql = "insert into visits(ticket_id, time) "
                                                                      "values( " + QString::number(ticket.id) + ", "
                                                                            " '" + QDateTime::currentDateTime().addSecs(-10800).toString("yyyy-MM-dd hh:mm:ss") + "') ; ";
                                                        m_DB->exec(sql);

                                                        QString sq2 = "insert into visits(ticket_id, time) "
                                                                      "values( (select t.id "
                                                                               "  from ticket t left join barcode b on (t.barcode_id=b.id) "
                                                                               " where b.identifier='" + ticket.barcode + "' ), "
                                                                            " '" + QDateTime::currentDateTime().addSecs(-10800).toString("yyyy-MM-dd hh:mm:ss") + "') ; ";
                                                        m_DBrent->exec(sq2);


                                                        // ------------------------------------------------------
                                                        // статус прохода на каток изменился - надо его применить
                                                        if (ticket.f_chPass) {
                                                            if (ticket.pass) {
                                                                m_SDK->passCard(ticket.staff_id, ticket.identifier, ticket.card_id);
                                                            } else {
                                                                m_SDK->denyCard(ticket.staff_id, ticket.identifier, ticket.card_id);
                                                            }
                                                        }

                                                        // ------------------------------------------------------
                                                        // данные билета изменились - надо их применить в базу
                                                        if (ticket.f_change) {
                                                            QString sql = "update ticket "
                                                                          "   set _status = " + QString::number(ticket.status) + ", " +
                                                   (ticket.has_start  ? ( "       started_at  = '" + ticket.tm_start.toString("yyyy-MM-dd hh:mm:ss") + "', ") : "" ) +
                                                   (ticket.has_finish ? ( "       finished_at = '" + ticket.tm_finish.toString("yyyy-MM-dd hh:mm:ss") + "', ") : "" ) +
                                                                          "       prohibit = " + (ticket.pass ? "0" : "1") + " "
                                                                          " where id = " + QString::number(ticket.id) + "; ";
                                                            m_DB->exec(sql);



                                                            QString sq2 = "update ticket "
                                                                          "   set " +
                                                   (ticket.status>1   ? ( "       _status = " + QString::number(ticket.status) + ", ") : "" ) +
                                                   (ticket.has_start  ? ( "       started_at  = '" + ticket.tm_start.toString("yyyy-MM-dd hh:mm:ss") + "' ") : "" ) +
                                                                          " where id = (select t.id "
                                                                          "               from ticket t left join barcode b on (t.barcode_id=b.id) "
                                                                          "              where b.identifier='" + ticket.barcode + "' ); ";

//                                                          QString sq2 = "update ticket "
//                                                                        "   set " +
//                                                 (ticket.status>1   ? ( "       _status = " + QString::number(ticket.status) + ", ") : "" ) +
//                                                 (ticket.has_start  ? ( "       started_at  = '" + ticket.tm_start.toString("yyyy-MM-dd hh:mm:ss") + "', ") : "" ) +
//                                                                        " where id = " + QString::number(ticket.id) + "; ";

                                                            m_DBrent->exec(sq2);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        dc = dc.nextSibling();
    }

    return true;
}



bool core_object::tstEvents(QString barcode) {

    bool flOk = true;
    int identifier = barcode.toInt();
    if (settings.value("turnside/save_Hex", "0").toInt()>0)
        identifier = barcode.toInt(&flOk, 16);
    if (!flOk)  return false;

    map_rec rec;
    m_PERCo->connect();
    bool flStaff = m_PERCo->selectOne_map ("select s.ID_STAFF, s.TABEL_ID, s.FULL_FIO "
                                           "  from STAFF_CARDS c "
                                           "  left join STAFF s on(s.ID_STAFF = c.STAFF_ID)"
                                           " where c.IDENTIFIER=" + QString::number(identifier) + " ; ",
                                           rec);
    if (!flStaff)  return false;

    QString id_staff_internal  = rec["ID_STAFF"];
   // debug << "\n- " << " id_staff_internal " << " " << id_staff_internal << "\n";
    QString tab_n = rec["TABEL_ID"];
   // debug << "- " << " tab_n " << " " << tab_n << "\n";
    QString fio   = rec["FULL_FIO"];
   // debug << "- " << " fio " << " " << fio << "\n";
    QString subdiv_id_internal   = "1";
    QString appoint_id_internal  = "1";
    QString areas_name   = " - ";
    QString reader_name  = " - ";
    QString typepass     = " - ";

    m_PERCo->disconnect();

    // фильтрация по дате и времени
    QDateTime dt_ev = QDateTime::currentDateTime().addSecs(-10);
    QString date_s = dt_ev.toString("dd.MM.yyyy");
    QString time_s = dt_ev.toString("hh:mm:ss");
//                                            QDate date_ev = QDate::fromString(date_s, "dd.MM.yyyy");
//                                            QTime time_ev = QTime::fromString(time_s, "hh:mm:ss");
//                                           // debug << "-" << " fio " << " " << fio;
//                                           // debug << "-" << "datetimeevent" << " " << datetimeevent;
//                                           // debug << "-" << "date_s" << " " << date_s;
//                                           // debug << "-" << "date_ev" << " " << date_ev;
//                                           // debug << "-" << "time_ev" << " " << time_ev;
    if ( dt_ev.date()==QDate::currentDate()                      // только сегодняшние события
        /* && time_ev>QTime::currentTime().addSecs(-3600)*/ ) { // события не старше 1 часа

        // проверим наличие такой записи
        QString sql = "select count(*) "
                      "  from events "
                      " where id_staff_internal=" + id_staff_internal + " "
                      "   and date_ev='" + date_s + "' "
                      "   and time_ev='" + time_s + "' ";
//                                               // debug << "-" << sql;
//                                               // debug << "\n"; // debug.flush();
        int cnt = m_DB->selectId(sql);
        if (cnt<1) {
            // дорбавим событие в базу на PG
            map_rec map;
            map.addI("id_staff_internal", id_staff_internal.toInt());
            map.addI("tab_n", tab_n.toInt());
            map.addS("fio", fio);
            map.addS("date_ev", date_s);
            map.addS("time_ev", time_s);
            map.addI("subdiv_id_internal", subdiv_id_internal.toInt());
            map.addI("appoint_id_internal", appoint_id_internal.toInt());
            map.addS("areas_name", areas_name);
            map.addS("reader_name", reader_name);
            map.addS("typepass", typepass);
            bool res = m_DB->insert_map("events", map);

            if (!res) {
               // debug << "-" << "ошибка при добавлении события" << " " << map.toString();
               // debug << "\n"; // debug.flush();
            }

            // обработаем новое событие прохода черех турникет

            // -------------------------------
            // прочитаем билеты из базы данных
            // -------------------------------
            QString sql = "select t.id, b.identifier, b.family, b.number, b.staff_id, b.card_id, "
                          "       t._status as status, t.prohibit, "
                          "       (t.created_at  /*+ time '3:00:00'*/) as created_at, "
                          "       (t.started_at  /*+ time '3:00:00'*/) as started_at, "
                          "       (t.finished_at /*+ time '3:00:00'*/) as finished_at, "
                          "       tt.type_id as tariff_type, tt.time as time_min, "
                          "       (TIMESTAMP WITH TIME ZONE 'epoch' + (cast(tt.time as bigint)) * INTERVAL '1 second') as time_to, "
                          "       s.setting, s.value "
                          "  from ticket t "
                          "  left join tariff tt on(tt.id=t.tariff_id) "
                          "  left join barcode b on(b.id=t.barcode_id) "
                          "  left join settings s on(s.setting='tariff' and s.is_act=0) "
                          " where b.staff_id=" + id_staff_internal + " "
                          "   and t._status in (0,1) "
                          "   and tt.type_id in (1,2,3) "
                          "   and t.created_at is not NULL ; ";
//                                                   // debug << "-" << sql;
           // debug << "\n"; // debug.flush();
            map_rec rec;
            m_DB->selectOne_map(sql, rec);


            // если данные есть (такой билет есть и продан) ...
            if (rec.size()>0) {
                db_ticket ticket(rec);

                ticket.f_change = false;
                ticket.f_chPass = false;

                // --------------------------------
                // проверка/задание времени катания
                if (ticket.tariff==1) {
                    // разрешаем проход на каток
                    // время катания начинается прямо сечас и закончится через time_min минут
                    ticket.set_pass(true);

                    if (!ticket.has_start) {
                        ticket.tm_start = QDateTime::currentDateTime();
                       // debug<<"\n??? " << " ticket.tm_start" << " " << ticket.tm_start.toString("yyyy-MM-dd hh:mm:ss") << "\n";
                        ticket.has_start = true;
                        ticket.tm_finish = QDateTime::currentDateTime().addSecs(ticket.time_min *60);
                       // debug << "??? " << " ticket.tm_finish"<< " " << ticket.tm_finish.toString("yyyy-MM-dd hh:mm:ss")<< "\n";
                        ticket.has_finish = true;
                        ticket.f_change = true;
                    }
                    if (!ticket.has_finish) {
                        ticket.tm_finish = QDateTime::currentDateTime().addSecs(ticket.time_min *60);
                        ticket.has_finish = true;
                        ticket.f_change = true;
                    }
                }
                else if (ticket.tariff==2) {
                    // разрешаем проход на каток
                    // время катания начинается прямо сечас и закончится через time_min минут
                    ticket.set_pass(true);

                    if (!ticket.has_start) {
                        ticket.tm_start = QDateTime::currentDateTime();
                        ticket.has_start = true;
                        ticket.tm_finish = QDateTime::currentDateTime().addSecs(ticket.time_min *60);
                        ticket.has_finish = true;
                        ticket.f_change = true;
                    }
                    if (!ticket.has_finish) {
                        ticket.tm_finish = QDateTime::currentDateTime().addSecs(ticket.time_min *60);
                        ticket.has_finish = true;
                        ticket.f_change = true;
                    }
                }
                else if (ticket.tariff==3) {
                    // разрешаем проход на каток
                    // время катания начинается прямо сечас и продолжится до вечера
                    ticket.set_pass(true);

                    if (!ticket.has_start) {
                        ticket.tm_start = QDateTime::currentDateTime();
                        ticket.has_start = true;
                        ticket.tm_finish = QDateTime(QDate::currentDate(), ticket.time_to);
                        ticket.has_finish = true;
                        ticket.f_change = true;
                    }
                    if (!ticket.has_finish) {
                        ticket.tm_finish = QDateTime(QDate::currentDate(), ticket.time_to);
                        ticket.has_finish = true;
                        ticket.f_change = true;
                    }
                }


                // -------------------------------------------------
                // если это первый проход - активируем время катания
                if (ticket.status==0) {
                    ticket.status = 1;
                    ticket.f_change = true;
                }


                // --------------------------------
                // контроль срока действия билета
                if (!ticket.has_finish) {
                    // срок катания ещё не известен
                }
                else if (ticket.tm_finish > QDateTime::currentDateTime()) {
                    // время не истекло - пусть катается
                }
                else if (ticket.tm_finish <= QDateTime::currentDateTime()) {
                    // время истекло - стоп билета
                    ticket.set_pass(false);
                    ticket.status = 2;
                }

                // ------------------------------------------------------
                // добавим данные визита
                QString sql = "insert into visits(ticket_id, time) "
                              "values( " + QString::number(ticket.id) + ", "
                                    " '" + QDateTime::currentDateTime().addSecs(-10800).toString("yyyy-MM-dd hh:mm:ss") + "') ; ";
                m_DB->exec(sql);

                QString sq2 = "insert into visits(ticket_id, time) "
                              "values( (select t.id "
                                       "  from ticket t left join barcode b on (t.barcode_id=b.id) "
                                       " where b.identifier='" + ticket.barcode + "' ), "
                                    " '" + QDateTime::currentDateTime().addSecs(-10800).toString("yyyy-MM-dd hh:mm:ss") + "') ; ";
                m_DBrent->exec(sq2);


                // ------------------------------------------------------
                // статус прохода на каток изменился - надо его применить
                if (ticket.f_chPass) {
                    if (ticket.pass) {
                        m_SDK->passCard(ticket.staff_id, ticket.identifier, ticket.card_id);
                    } else {
                        m_SDK->denyCard(ticket.staff_id, ticket.identifier, ticket.card_id);
                    }
                }

                // ------------------------------------------------------
                // данные билета изменились - надо их применить в базу
                if (ticket.f_change) {
                    QString sql = "update ticket "
                                  "   set _status = " + QString::number(ticket.status) + ", " +
           (ticket.has_start  ? ( "       started_at  = '" + ticket.tm_start.toString("yyyy-MM-dd hh:mm:ss") + "', ") : "" ) +
           (ticket.has_finish ? ( "       finished_at = '" + ticket.tm_finish.toString("yyyy-MM-dd hh:mm:ss") + "', ") : "" ) +
                                  "       prohibit = " + (ticket.pass ? "0" : "1") + " "
                                  " where id = " + QString::number(ticket.id) + "; ";
                    m_DB->exec(sql);

                    QString sq2 = "update ticket "
                                  "   set _status = " + QString::number(ticket.status) + ", " +
           (ticket.has_start  ? ( "       started_at  = '" + ticket.tm_start.toString("yyyy-MM-dd hh:mm:ss") + "' ") : "" ) +
                                  " where id = (select t.id "
                                  "               from ticket t left join barcode b on (t.barcode_id=b.id) "
                                  "              where b.identifier='" + ticket.barcode + "' ); ";
                    m_DBrent->exec(sq2);
                }
            }
        }
    }
    return true;
}


bool core_object::test_tickets_in_DB() {
    // -----------------------------------
    // прочитаем все билеты из базы данных
    // -----------------------------------

    QString sql = "select t.id, b.identifier, b.family, b.number, b.staff_id, b.card_id, "
                  "       t._status as status, t.prohibit, "
                  "       (t.created_at  /*+ time '3:00:00'*/) as created_at, "
                  "       (t.started_at  /*+ time '3:00:00'*/) as started_at, "
                  "       (t.finished_at /*+ time '3:00:00'*/) as finished_at, "
                  "       tt.type_id as tariff_type, tt.time as time_min, "
                  "       (TIMESTAMP WITH TIME ZONE 'epoch' + (cast(tt.time as bigint)) * INTERVAL '1 second') as time_to, "
                  "       s.setting, s.value, "
                  "       b.identifier as barcode "
                  "  from ticket t "
                  "  left join tariff tt on(tt.id=t.tariff_id) "
                  "  left join barcode b on(b.id=t.barcode_id) "
                  "  left join settings s on(s.setting='tariff' and s.is_act=0) "
                  " where t._status in (0,1/*,2,3*/) "
                  "   and tt.type_id in (1,2,3) "
                  "   and t.created_at is not NULL "
                  "   /*and (CURRENT_TIMESTAMP - (t.created_at + INTERVAL '3:00:00')) < (interval '24:00:00')*/ ; "
            ;
//    debug << "-" << sql;
//    debug << "\n";  debug.flush();
    map_table data;
    m_DB->select_map(sql, data);

    db_tickets_list tickets;
    for (int i=0; i<data.size(); i++) {
        map_rec &rec = data[i];
        tickets.append( db_ticket(rec) );
    }

    for (int i=0; i<tickets.size(); i++) {
        db_ticket &ticket = tickets[i];

        if (ticket.status<2) {
           // debug << "\n <<<";
           // debug << "\n-" << " * ID" << " " << ticket.identifier ;
           // debug << "\n-" << " status" << " " << QString::number(ticket.status) ;
           // debug << "\n-" << "has_create" << " " << ticket.has_create ;
           // debug << "\n-" << "tm_create" << " " << ticket.tm_create.toString("yyyy-MM-dd hh:mm:ss") ;
           // debug << "\n-" << "has_start" << " " << ticket.has_start ;
           // debug << "\n-" << "tm_start" << " " << ticket.tm_start.toString("yyyy-MM-dd hh:mm:ss") ;
           // debug << "\n-" << "has_finish" << " " << ticket.has_finish ;
           // debug << "\n-" << "tm_finish" << " " << ticket.tm_finish.toString("yyyy-MM-dd hh:mm:ss") ;
           // debug << "\n-" << "time_min" << " " << ticket.time_min ;
           // debug << "\n-" << "time_to" << " " << ticket.time_to.toString("hh:mm:ss") ;
           // debug << "\n"; // debug.flush();
        }

        ticket.f_change = false;
        ticket.f_chPass = false;

        if (ticket.status==0) {

            if (ticket.tariff==1) {
                // разрешаем проход в ожидании когда клиент прийдёт на каток
                // время катания начнётся после первого прохода
                ticket.set_pass(true);
            }
            else if (ticket.tariff==2) {
                // разрешаем проход на каток
                // время катания начинается прямо сечас и закончится через time_min минут
                ticket.set_pass(true);

                if (!ticket.has_start) {
                    ticket.tm_start = QDateTime::currentDateTime();
                    ticket.has_start = true;
                    ticket.tm_finish = ticket.tm_start.addSecs(ticket.time_min *60/* + 10800*/);
                    ticket.has_finish = true;
                }
                if (!ticket.has_finish) {
                    ticket.tm_finish = ticket.tm_start.addSecs(ticket.time_min *60/* + 10800*/);
                    ticket.has_finish = true;
                }
                // билет с таким тарифом сразу переходит в статус 1!
                ticket.status = 1;
                ticket.f_change = true;
            }
            else if (ticket.tariff==3) {
                // разрешаем проход на каток
                // время катания начинается прямо сечас и продолжится до вечера
                ticket.set_pass(true);

                if (!ticket.has_start) {
                    ticket.tm_start = QDateTime::currentDateTime();
                    ticket.has_start = true;
                    ticket.tm_finish = QDateTime(QDate::currentDate(), ticket.time_to);
                    ticket.has_finish = true;
                }
                if (!ticket.has_finish) {
                    ticket.tm_finish = QDateTime(QDate::currentDate(), ticket.time_to);
                    ticket.has_finish = true;
                }
                // билет с таким тарифом сразу переходит в статус 1!
                ticket.status = 1;
                ticket.f_change = true;
            }
            ticket.f_change = true;

        }
        else

        if (ticket.status==1) {

            // проверка времени начала катания и времени окончания катания
            if (ticket.tariff==1) {
                // разрешаем проход в ожидании когда клиент прийдёт на каток
                // время катания начнётся после первого прохода
                ticket.set_pass(true);
            }
            else if (ticket.tariff==2) {
                // проход на каток
                ticket.set_pass(true);

                if (!ticket.has_start) {
                    ticket.tm_start = QDateTime::currentDateTime();
                    ticket.has_start = true;
                    ticket.tm_finish = ticket.tm_start.addSecs(ticket.time_min *60/* + 10800*/);
                    ticket.has_finish = true;
                    ticket.f_change = true;
                }
                if (!ticket.has_finish) {
                    ticket.tm_finish = ticket.tm_start.addSecs(ticket.time_min *60/* + 10800*/);
                    ticket.has_finish = true;
                    ticket.f_change = true;
                }
            }
            else if (ticket.tariff==3) {
                // проход на каток
                ticket.set_pass(true);

                if (!ticket.has_start) {
                    ticket.tm_start = QDateTime::currentDateTime();
                    ticket.has_start = true;
                    ticket.tm_finish = QDateTime(QDate::currentDate(), ticket.time_to);
                    ticket.has_finish = true;
                    ticket.f_change = true;
                }
                if (!ticket.has_finish) {
                    ticket.tm_finish = QDateTime(QDate::currentDate(), ticket.time_to);
                    ticket.has_finish = true;
                    ticket.f_change = true;
                }
            }

            if (ticket.status<2) {
               // debug << "\n V ";
               // debug << "\n-" << " ** ID" << " " << ticket.identifier;
               // debug << "\n-" << "has_create" << " " << ticket.has_create;
               // debug << "\n-" << "tm_create" << " " << ticket.tm_create.toString("yyyy-MM-dd hh:mm:ss");
               // debug << "\n-" << "has_start" << " " << ticket.has_start;
               // debug << "\n-" << "tm_start" << " " << ticket.tm_start.toString("yyyy-MM-dd hh:mm:ss");
               // debug << "\n-" << "has_finish" << " " << ticket.has_finish;
               // debug << "\n-" << "tm_finish" << " " << ticket.tm_finish.toString("yyyy-MM-dd hh:mm:ss");
               // debug << "\n-" << "time_min" << " " << ticket.time_min;
               // debug << "\n-" << "time_to" << " " << ticket.time_to.toString("hh:mm:ss");
               // debug << "\n >>> ";
               // debug << "\n"; // debug.flush();
            }

            // контроль срока действия билета
            if (!ticket.has_finish) {
                // срок катания ещё не известен
                if (ticket.has_start && ticket.time_min<360) {
                    ticket.tm_finish = ticket.tm_start.addSecs(ticket.time_min *60);
                    ticket.has_finish = true;
                    ticket.f_change = true;
                }
            }
            else if (ticket.tm_finish > QDateTime::currentDateTime()) {
                // время не истекло - пусть катается
            }
            else if (ticket.tm_finish <= QDateTime::currentDateTime()) {
                // время истекло - стоп билета
                ticket.set_pass(false);
                ticket.status = 2;
                ticket.f_change = true;
            }

        }

        if (ticket.status==2) {

            if (ticket.tariff==1) {
                // время истекло - стоп билета
                ticket.set_pass(false);
            }
            else
            if (ticket.tariff==2) {
                // время истекло - стоп билета
                ticket.set_pass(false);
            }
            else
            if (ticket.tariff==3) {
                // время истекло - стоп билета
                ticket.set_pass(false);
            }

        }
        else

        if (ticket.status==3) {

            if (ticket.tariff==1) {
                // время истекло - стоп билета
                ticket.set_pass(false);
            }
            else
            if (ticket.tariff==2) {
                // время истекло - стоп билета
                ticket.set_pass(false);
            }
            else
            if (ticket.tariff==3) {
                // время истекло - стоп билета
                ticket.set_pass(false);
            }

        }

        else {
        }


        // ------------------------------------------------------
        // статус прохода на каток изменился - надо его применить
        if (ticket.f_chPass) {
            if (ticket.pass) {
                this->passCard(ticket.identifier);
                ticket.f_change = true;
            } else {
                this->denyCard(ticket.identifier);
                ticket.f_change = true;
            }
        }

        // ---------------------------------------------------
        // данные билета изменились - надо их применить в базу
        if ( ticket.f_change
           && ticket.status!=0 )
        {
            QString sql = "update barcode"
                          "   set status = " + QString::number(ticket.status) + " "
                          " where identifier = '" + ticket.barcode + "' ; "

                          "update ticket "
                          "   set _status = " + QString::number(ticket.status) + ", "
  + (ticket.has_start  ? ("       started_at  = '" + ticket.tm_start .toString("yyyy-MM-dd hh:mm:ss") + "', ") : "")
  + (ticket.has_finish ? ("       finished_at = '" + ticket.tm_finish.toString("yyyy-MM-dd hh:mm:ss") + "', ") : "") +
                          "       prohibit = " + (ticket.pass ? "0" : "1") + " "
                          " where id = " + QString::number(ticket.id) + "; ";
            debug << "-" << "sql" << " " << sql;
            debug << "\n";  debug.flush();
            m_DB->exec(sql);

            // DBrent
            sql = "update barcode"
                  "   set status = " + QString::number(ticket.status) + " "
                  " where identifier = '" + ticket.barcode + "' "
                  "   and status<1 ; "

                  "update ticket "
                  "   set _status = " + QString::number(ticket.status) + " "
 +(ticket.has_start ? (", started_at  = '" + ticket.tm_start .toString("yyyy-MM-dd hh:mm:ss") + "' ") : " ") +
                  " where barcode_id = (select id from barcode where identifier='" + ticket.barcode + "') ; ";
            debug << "-" << "sql" << " " << sql;
            debug << "\n";  debug.flush();
            m_DBrent->exec(sql);
        }
    }

    return true;
}
