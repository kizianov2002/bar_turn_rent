#ifndef DB_TICKET_H
#define DB_TICKET_H

#include <QString>
#include <QDateTime>

#include "db_rec.h"

class db_ticket
{
public:
    db_ticket(map_rec &rec);
    map_rec &rec;

    int id, staff_id, card_id;
    qint64 identifier;
    QString barcode;
    int status;
    QDateTime  tm_create, tm_start,  tm_finish;
    bool has_create, has_start, has_finish;
    bool pass;

    int tariff;
    int time_min;
    QTime time_to;

    bool set_start(QTime tm_start);
    bool set_finish(QTime tm_finish);
    bool set_pass(bool pass);

    bool f_change;
    bool f_chPass;
};

class db_tickets_list: public QList<db_ticket> {
};

#endif // DB_TICKET_H
