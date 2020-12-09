#include "db_ticket.h"
#include <QDebug>

db_ticket::db_ticket(map_rec &rec)
    : rec(rec)
{
    id = rec["id"].toInt();
    identifier = rec["identifier"].toInt();
    barcode = rec["identifier"];
    staff_id = rec["staff_id"].toInt();
    card_id = rec["card_id"].toInt();
    status = rec["status"].toInt();

    if (has_create) {
        QString s = rec["created_at"].left(19).replace("T", " ");
        tm_create = QDateTime::fromString(s, "yyyy-MM-dd hh:mm:ss");
        if (tm_create.date()>QDate::currentDate()) {
            tm_create = QDateTime(QDate::currentDate(), tm_create.time());
        }
    }
    else tm_create = QDateTime::currentDateTime();
    has_create = tm_create.isValid();

    has_start = !(rec.nulls["started_at"]);
    if (has_start) {
        QString s = rec["started_at"].left(19).replace("T", " ");
        tm_start = QDateTime::fromString(s, "yyyy-MM-dd hh:mm:ss");
    }
//    else tm_start = QTime::currentTime();
    has_start = tm_start.isValid();

    has_finish = !(rec.nulls["finished_at"]);
    if (has_finish) {
        QString s = rec["finished_at"].left(19).replace("T", " ");
        tm_finish = QDateTime::fromString(s, "yyyy-MM-dd hh:mm:ss");
    }
//    else tm_finish = QTime::currentTime();
    has_finish = tm_finish.isValid();

    pass = (rec["prohibit"]=="0");
    tariff = rec["tariff_type"].toInt();
    time_min = rec["time_min"].toInt();
    {
        QString s = rec["time_to"].left(19).replace("T", " ");
        time_to = QDateTime::fromString(s, "yyyy-MM-dd hh:mm:ss").time();
    }
    // если время не известно - 2 часа дня или 10 вечера
    if (time_to.isNull() || !time_to.isValid()) {
        if (QTime::currentTime()<QTime(13,30))
             time_to = QTime(14,0);
        else time_to = QTime(22,0);
    }

    f_change = false;
    f_chPass = false;
}

bool db_ticket::set_pass(bool pass_in)
{
    if (pass!=pass_in) {
        pass = pass_in;
        f_chPass = true;
        return true;
    }
    return false;
}

bool db_ticket::set_start(QTime start)
{
    if (!has_start) {
        has_start = true;
        tm_start = QDateTime(QDate::currentDate(), start);
        f_change = true;
        return true;
    }
    return false;
}

bool db_ticket::set_finish(QTime finish)
{
    if (!has_finish) {
        has_finish = true;
        tm_finish = QDateTime(QDate::currentDate(), finish);
        f_change = true;
        return true;
    }
    return false;
}
