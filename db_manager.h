#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include <QString>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>
#include <QTextStream>

#include "db_rec.h"
#include "waiter.h"

class db_manager
{
    QString name;
    waiter *m_wtr;
    QString m_drvr;
    QString m_host;
    int     m_port;
    QString m_base;
    QString m_user;
    QString m_pass;

    QTextStream &debug;

    QSettings &m_settings;
    QSqlDatabase m_db;

public:
    db_manager(QString name,
               waiter *wtr,
               QSettings &settings,
               QTextStream &debug/*,
               QString drvr,
               QString host,
               int     port,
               QString base,
               QString user,
               QString pass*/);

    bool connect();
    void disconnect();

    QString lastError() {
        return "driver: " + m_db.lastError().driverText() + ";  "
               "DB: " + m_db.lastError().databaseText();
    }

    bool exec(QString sql);

    bool select(QString sql, db_table &data);
    bool selectOne(QString sql, db_rec &data);

    bool select_map(QString sql, map_table &data);
    bool selectOne_map(QString sql, map_rec &data);

    bool insert_map(QString table, map_rec &data);

    int selectId(QString sql);
};

#endif // DB_MANAGER_H
