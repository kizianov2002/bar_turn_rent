#include <QGuiApplication>

#include <QSettings>
#include <QDebug>
#include <QSqlDatabase>
#include <QFileInfo>
#include <QSqlError>
#include <QTextStream>

#include "core_object.h"
#include "tcpserver.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QString com = argv[1];

    // ++++++++++++++++++++++++++++++++++++++ //
    // проверка на порторный запуск программы //
    // ++++++++++++++++++++++++++++++++++++++ //
    QSystemSemaphore sema("bar_turn_rent_sema", 1);
    bool isRunning;
    // если семафор занят - стоп, вероятно параллельно запускается другой такой же процесс
    sema.acquire();

    {
        // если шаред-мемори уже есть, но не используется - удалим его
        QSharedMemory shmem("bar_turn_rent_shamem");
        shmem.attach();
    }

    QSharedMemory shmem("bar_turn_rent_shamem");
    if (shmem.attach())
    {
        // если шаред-мемори уже есть и занята - стоп
        isRunning = true;
    }
    else
    {
        // шаред-мемори нет - создадим для того чтобы другие не смогли работать и поехали
        shmem.create(1);
        isRunning = false;
    }

    // семафор больше не нужен
    sema.release();

    if (isRunning) {
        qDebug() << " " << "!!! PROGRAM ALREADY LAUNCHED !!!";
        return 345;  // собственно выход, если программа уже запущена
    }


    // +++++++ //
    // ПОЕХАЛИ //
    // +++++++ //

    // файл настроек INI
    QString ini_path = "c:/_TURNSIDE_/bar_turn_rent.ini";
    QFileInfo fi(ini_path);
    ini_path = fi.absoluteFilePath();
    QSettings settings(ini_path, QSettings::IniFormat);
    int ini_status = settings.status();
    qDebug() << " " << "ini_status = " << " " << ini_status;

    // проверка даты и резервное копирование INI
    QDate last_date = settings.value("main/last_date", QDate::currentDate().toString("yyyy-MM-dd")).toDate();
    if (!last_date.isValid()) {
        last_date = QDate::currentDate();
        settings.setValue("main/last_date", last_date.toString("yyyy-MM-dd"));
        settings.sync();
    }
    if (last_date.daysTo(QDate::currentDate())>30) {
        QFile fff(ini_path);
        QString ini_path2 = ini_path.replace(".ini", "") + " - " + QDate::currentDate().toString("yyyy-MM-dd") + ".ini";
            QFile::remove(ini_path2);
            if (fff.copy(ini_path2)) {
                last_date = QDate::currentDate();
                settings.setValue("main/last_date", last_date.toString("yyyy-MM-dd"));
                settings.sync();
            } else {
                qDebug() << "INI backup - disk error";
                return -1;
            }
    }

    // прочитаем текущие настройки
    int timer_step = 5000;
    int port = 33333;
    QString data_folder = "c:/_TURNSIDE_/DATA";
    QString sdkUser = "ADMIN";
    QString sdkPass = "";
    bool resetFlag = false;

    QFile dbg_file(data_folder + "/debug.txt");
    dbg_file.remove();
    dbg_file.open(QFile::WriteOnly | QFile::Truncate);
    QTextStream debug(&dbg_file);

    timer_step = settings.value("main/timer_step_ms", "5000").toInt();
    port = settings.value("main/tcp_port", "33333").toInt();
    data_folder = settings.value("main/data_folder", data_folder).toString();
    sdkUser = settings.value("SDK/user", sdkUser).toString();
    sdkPass = settings.value("SDK/pass", sdkPass).toString();
    resetFlag = settings.value("SDK/resetFlag", resetFlag).toInt()>0;

    QString main_printer = "Brother PJ-663";
    main_printer    = settings.value("print/printer", main_printer).toString();
    int main_PgW = settings.value("print/PgW", "80").toInt();
    int main_PgH = settings.value("print/PgH","500").toInt();
    int main_MgT = settings.value("print/MgT",  "1").toInt();
    int main_MgB = settings.value("print/MgB",  "1").toInt();
    int main_MgL = settings.value("print/MgL",  "1").toInt();
    int main_MgR = settings.value("print/MgR",  "1").toInt();
    int main_X0  = settings.value("print/X0", "0").toInt();
    int main_Xa  = settings.value("print/Xa","20").toInt();
    int main_Y0  = settings.value("print/Y0", "0").toInt();
    int main_X1  = settings.value("print/X1","70").toInt();
    int main_Y1  = settings.value("print/Y1","1000").toInt();
    int main_Resolution = settings.value("print/Resolution",  "203").toInt();

    QString barcore_printer = "Brother PJ-663";
    barcore_printer = settings.value("barcode/printer", barcore_printer).toString();
    int barc_mode = settings.value("barcode/mode", "0").toInt();
    int barc_PgW = settings.value("barcode/PgW","30").toInt();
    int barc_PgH = settings.value("barcode/PgH","18").toInt();
    int barc_MgT = settings.value("barcode/MgT", "1").toInt();
    int barc_MgB = settings.value("barcode/MgB", "1").toInt();
    int barc_MgL = settings.value("barcode/MgL", "1").toInt();
    int barc_MgR = settings.value("barcode/MgR", "1").toInt();
    int barc_Resolution = settings.value("barcode/Resolution", "203").toInt();
    QString barc_font128 = settings.value("barcode/font128", "Code 128").toString();
    QString barc_font13  = settings.value("barcode/font13",  "EAN-13").toString();

    int ch_Ten = settings.value("turnside/num_Ten", "0").toInt();
    int ch_HexUp = settings.value("turnside/num_HexUp", "0").toInt();
    int ch_HexDn = settings.value("turnside/num_HexDn", "0").toInt();
    int ch_SaveTen = settings.value("turnside/save_Ten", "0").toInt();
    int ch_SaveHex = settings.value("turnside/save_Hex", "0").toInt();

    // главный таймер не должен работать чаще чем раз в 2 сек.
    if (timer_step<2000)  timer_step = 2000;

    // проверим/добавим папку данных
    if (data_folder=="c:")  data_folder = "c:/_TURNSIDE_/DATA";
    if (data_folder=="c:/_TURNSIDE_")  data_folder = "c:/_TURNSIDE_/DATA";

    QDir dir(data_folder);
    if (!dir.exists()) {
        if (!dir.mkpath(data_folder)) {
            data_folder = "c:/_TURNSIDE_/DATA";
            dir = QDir(data_folder);
            if (!dir.exists()) {
                if (!dir.mkpath(data_folder)) {
                    data_folder = "c:";
                }
            }
        }
    }

    // запомним в INI обновлённые данные
    settings.setValue("main/ini_path", ini_path);
    settings.setValue("main/timer_step_ms", QString::number(timer_step));
    settings.setValue("main/tcp_port", QString::number(port));
    settings.setValue("main/data_folder", data_folder);
    settings.setValue("SDK/user", sdkUser);
    settings.setValue("SDK/pass", sdkPass);
    settings.setValue("SDK/resetFlag", resetFlag ? "1" : "0");

    settings.setValue("print/printer", main_printer);
    settings.setValue("print/PgW", QString::number(main_PgW));
    settings.setValue("print/PgH", QString::number(main_PgH));
    settings.setValue("print/MgT", QString::number(main_MgT));
    settings.setValue("print/MgB", QString::number(main_MgB));
    settings.setValue("print/MgL", QString::number(main_MgL));
    settings.setValue("print/MgR", QString::number(main_MgR));

    settings.setValue("print/X0", QString::number(main_X0));
    settings.setValue("print/Xa", QString::number(main_Xa));
    settings.setValue("print/Y0", QString::number(main_Y0));
    settings.setValue("print/X1", QString::number(main_X1));
    settings.setValue("print/Y1", QString::number(main_Y1));

    settings.setValue("print/Resolution", QString::number(main_Resolution));

    settings.setValue("barcode/printer", barcore_printer);
    settings.setValue("barcode/mode", QString::number(barc_mode));
    settings.setValue("barcode/PgW", QString::number(barc_PgW));
    settings.setValue("barcode/PgH", QString::number(barc_PgH));
    settings.setValue("barcode/MgT", QString::number(barc_MgT));
    settings.setValue("barcode/MgB", QString::number(barc_MgB));
    settings.setValue("barcode/MgL", QString::number(barc_MgL));
    settings.setValue("barcode/MgR", QString::number(barc_MgR));
    settings.setValue("barcode/Resolution", QString::number(barc_Resolution));
    settings.setValue("barcode/font128", barc_font128);
    settings.setValue("barcode/font13",  barc_font13);

    settings.setValue("turnside/num_Ten",   ch_Ten);
    settings.setValue("turnside/num_HexDn", ch_HexUp);
    settings.setValue("turnside/num_HexUp", ch_HexDn);
    settings.setValue("turnside/save_Ten",  ch_SaveTen);
    settings.setValue("turnside/save_Hex",  ch_SaveHex);

    settings.sync();

    core_object *CO = new core_object(settings, debug, &app);

    TcpServer *TCP = new TcpServer(settings, *CO, debug, nullptr);

    QString s_err;

    bool f_start_tcp = TCP->startServer();
    if (!f_start_tcp) {
        debug << "-" << "TCP error";
        debug << "\n";  debug.flush();
        return -2;
    } else {
        debug << "-" << "TCP ok";
        debug << "\n";  debug.flush();
    }

    bool f_open_DB = CO->openDB(&s_err);
    if (!f_open_DB) {
        debug << "-" << "DB error -" << " " << s_err;
        debug << "\n";  debug.flush();
        return -2;
    } else {
        debug << "-" << "DB ok";
        debug << "\n";  debug.flush();
    }

    // tickets list from m_DB
    map_table data_tickets;
    CO->m_DB->select_map("select b.identifier "
                         "  from ticket t "
                         "  left join barcode b on(b.id=t.barcode_id) "
                         " where b.identifier is not NULL "
                         "   and t._status in (0, 1) ; ",
                         data_tickets);
    QString tickets_list = "-1";
    foreach (const map_rec rec, data_tickets) {
        QString identifier = rec["identifier"];
        bool ok = true;
        if (!tickets_list.isEmpty()) tickets_list += ", ";
        tickets_list += QString::number((ch_SaveHex>0
                                         ? identifier.toInt(&ok, 16)
                                         : identifier.toInt(&ok, 10)), 10);
    }

    bool f_open_PERCo = CO->openPERCo(&s_err);
    if (!f_open_PERCo) {
        debug << "-" << "PERCo DB error -" << " " << s_err;
        debug << "\n";  debug.flush();
        return -3;
    } else {
        debug << "-" << "PERCo DB ok";
        debug << "\n";  debug.flush();

        // на ПЕРКо закроем отрытые билеты старше суток
        QDateTime dt = QDateTime::currentDateTime().addDays(-1);
        CO->m_PERCo->exec("UPDATE STAFF_CARDS "
                          "   SET prohibit=1 "
                          " WHERE prohibit=0 "
                          "   AND date_begin<='" + dt.toString("dd.MM.yyyy hh:mm") + "'; ");
//        // kill old staff recs
//        CO->m_PERCo->exec("delete from STAFF "
//                          " WHERE ID_STAFF not in (select staff_id "
//                                                  "  from staff_cards "
//                                                  " where identifier in (" + tickets_list + ")); ");
//        CO->m_PERCo->exec("delete from staff_cards "
//                          " where identifier not in (" + tickets_list + "); ");

        // оно нам пока не надо
        CO->m_PERCo->disconnect();
    }



//    CO->m_SDK->passCard(28604, 66667, 28605);
//    CO->m_SDK->passCard(28608, 66668, 28609);

//    CO->onTimerTick();
//    CO->getEvents();

    CO->swichTimer(true);

    // +++++++++++++++++++++++++++++ //
    // очистка базы от старых данных //
    // +++++++++++++++++++++++++++++ //
    if (com=="clear") {
        QTextStream out(stdout);
        QTextStream inp(stdin);

        out << " " << "RESET TICKETS DATA? (yes/no)" << " " << endl;
        QString str = "";
        inp >> str;
        if (str.trimmed().toLower()=="yes")
            CO->resetTickets();
        else
            out << " " << "no" << " " << endl;

        out << " " << "RESET INVENTORY DATA? (yes/no)" << " " << endl;
        QString st2 = "";
        inp >> st2;
        if (st2.trimmed().toLower()=="yes")
            CO->resetInvents();
        else
            out << " " << "no" << " " << endl;
    }

    int res = 0;
    try {
        res = app.exec();
    } catch (...) {
        CO->m_PERCo->disconnect();
        CO->m_DB->disconnect();
        CO->m_SDK->disconnect();
    }
    CO->m_PERCo->disconnect();
    CO->m_DB->disconnect();
    CO->m_SDK->disconnect();

    return res;
}
