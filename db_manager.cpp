#include "db_manager.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QDebug>

db_manager::db_manager( QString name,
                        waiter *wtr,
                        QSettings &settings,
                        QTextStream &debug/*,
                        QString drvr,
                        QString host,
                        int     port,
                        QString base,
                        QString user,
                        QString pass*/ ) :
    name(name),
    m_settings(settings),
    m_wtr(wtr),
    debug(debug),
//    m_drvr(drvr),
//    m_host(host),
//    m_port(port),
//    m_base(base),
//    m_user(user),
//    m_pass(pass),
    m_db(QSqlDatabase())
{
    QString pref = name.isEmpty() ? ("") : (name + "/");
    m_drvr = settings.value(pref + "drvr", "XXX").toString();
    m_host = settings.value(pref + "host", "XXX").toString();
    m_port = settings.value(pref + "port", 0).toInt();
    m_base = settings.value(pref + "base", "XXX").toString();
    m_user = settings.value(pref + "user", "XXX").toString();
    m_pass = settings.value(pref + "pass", "XXX").toString();

    // подключение к БД
    m_db = QSqlDatabase::addDatabase(m_drvr, name);
    m_db.setHostName(m_host);
    m_db.setPort(m_port);
    m_db.setDatabaseName(m_base);
    m_db.setUserName(m_user);
    m_db.setPassword(m_pass);
}


bool db_manager::connect()
{
//    // подключение к БД
//    m_db = QSqlDatabase::addDatabase(m_drvr, name);
//    m_db.setHostName(m_host);
//    m_db.setPort(m_port);
//    m_db.setDatabaseName(m_base);
//    m_db.setUserName(m_user);
//    m_db.setPassword(m_pass);

  mm:

    /*if ( m_db.isOpen()) {
         return true;
    } else*/
    if (m_db.open()) {
        debug << name  << "is opened" ;
        debug << "\n";  debug.flush();

        m_settings.beginGroup(name);
        m_settings.setValue("drvr", m_drvr);
        m_settings.setValue("host", m_host);
        m_settings.setValue("port", m_port);
        m_settings.setValue("base", m_base);
        m_settings.setValue("user", m_user);
        m_settings.setValue("pass", m_pass);
        m_settings.endGroup();
        m_settings.sync();

        return true;
    } else {
        debug << "-" << name << " " << "connection error: " << " " << m_db.lastError().text();
        debug << "\n";  debug.flush();
        // m_wtr->wait(30);
        if (m_db.open()) {
            debug << name  << "is opened" ;
            debug << "\n";  debug.flush();

            m_settings.beginGroup(name);
            m_settings.setValue("drvr", m_drvr);
            m_settings.setValue("host", m_host);
            m_settings.setValue("port", m_port);
            m_settings.setValue("base", m_base);
            m_settings.setValue("user", m_user);
            m_settings.setValue("pass", m_pass);
            m_settings.endGroup();
            m_settings.sync();

            return true;
        }
        // goto mm;
        return false;
    }
}


void db_manager::disconnect()
{
    m_db.close();
    debug << name << " is closed";
    debug << "\n";  debug.flush();
}


bool db_manager::exec(QString sql)
{
    debug << "-" << " >> SQL >> " << "\n";
    debug << "- ( " << sql << " )\n";
    debug << "\n";  debug.flush();

    //m_db.transaction();

    QSqlQuery query(m_db);
    bool res = query.exec(sql);

    if (res) {
        //m_db.commit();
    } else {
        //m_db.rollback();

        debug << "-" << " >> SQL >> " << "\n";
        debug << "- ( " << sql << " )\n";
        debug << "-" << "DB: " + query.lastError().databaseText() << " " << ";  driver: " << " " << query.lastError().driverText();
        debug << "\n";  debug.flush();
    }

    return res;
}


int db_manager::selectId(QString sql)
{
    //m_db.transaction();

    QSqlQuery query(m_db);
    bool res = query.exec(sql);

    if (res) {
        int val = 0;
        if (this->m_drvr=="QPSQL") {
            if (query.next()) {
                int size = query.size();
                val = size>0 ? query.value(0).toInt() : -1;
            }
        } else {
            if (query.next()) {
                val = query.value(0).toInt();
            }
        }
        return val;
        //m_db.commit();
    } else {
       //m_db.rollback();

        debug << "-" << " >> SQL >> " << "\n";
        debug << "- ( " << sql << " )\n";
        debug << "-" << "DB: " + query.lastError().databaseText() << " " << ";  driver: " << " " << query.lastError().driverText();
        debug << "\n";  debug.flush();
    }

    return -1;
}


bool db_manager::selectOne(QString sql, db_rec &data)
{
    //m_db.transaction();

    data.clear();
    QSqlQuery query(m_db);
    bool res = query.exec(sql);

    if (res) {
        QSqlRecord rec = query.record();
        if (query.next()) {
            for (int i=0; i<rec.count(); i++) {
                QString field = rec.fieldName(i);
                QString value = query.value(i).toString();
                data.append(value);
            }
        }
        //m_db.commit();
    } else {
       //m_db.rollback();

        debug << "-" << " >> SQL >> " << "\n";
        debug << "- ( " << sql << " )\n";
        debug << "-" << "DB: " + query.lastError().databaseText() << " " << ";  driver: " << " " << query.lastError().driverText();
        debug << "\n";  debug.flush();
    }

    return res;
}


bool db_manager::select(QString sql, db_table &data)
{
    //m_db.transaction();

    data.clear();
    QSqlQuery query(m_db);
    bool res = query.exec(sql);

    if (res) {
        QSqlRecord rec = query.record();

//        debug << "-" << " >> SQL >> " << "\n";
//        debug << "- ( " << sql << " )\n";

        while (query.next()) {

            QStringList list;
            for (int i=0; i<rec.count(); i++) {
                QString field = rec.fieldName(i);
                QString value = query.value(i).toString();
                list.append(value);
            }
            data.append(list);
        }
        //m_db.commit();
    } else {
       //m_db.rollback();

        debug << "-" << " >> SQL >> " << "\n";
        debug << "- ( " << sql << " )\n";
        debug << "-" << "DB: " + query.lastError().databaseText() << " " << ";  driver: " << " " << query.lastError().driverText();
        debug << "\n";  debug.flush();
    }

    return res;
}


bool db_manager::selectOne_map(QString sql, map_rec &data)
{
    //m_db.transaction();
    debug << "-" << "   sql   " << " " << sql << "\n";

    data.clear();
    QSqlQuery query(m_db);
    bool res = query.exec(sql);
    QString err_db  = query.lastError().databaseText();
    QString err_drv = query.lastError().driverText();


    debug << "- res = '" << res << "'\n";
    debug << "- err_db = '"  << err_db  << "' -> " << err_db.isEmpty() << "\n";
    debug << "- err_drv = '" << err_drv << "' -> " << err_drv.isEmpty()<< "\n";
    debug << "- test = '" << ( res || (err_db.isEmpty() && err_drv.isEmpty()) ) << "\n";
    debug.flush();

    if ( res
     // || (err_db.isEmpty() && err_drv.isEmpty())
       ) {
        QSqlRecord rec = query.record();

        if (query.next()) {

            for (int i=0; i<rec.count(); i++) {
                QString field = rec.fieldName(i);
                QString value = query.value(i).toString();
                debug << "- -" << field << " = " << value << "";
                bool is_null = query.value(i).isNull();
                data[field] = value;
                data.nulls[field] = is_null;
            }
            debug << "\n";
        }
        //m_db.commit();
    } else {
       //m_db.rollback();

        debug << "- " << " >> SQL >> " << "\n";
        debug << "- ( " << sql << " )\n";
        debug << "- " << "DB: " + err_db << " " << ";  driver: " << " " << err_drv;
        debug << "\n";  debug.flush();
    }

    return res;
}


bool db_manager::insert_map(QString table, map_rec &data)
{
    //m_db.transaction();

    QString sql = "insert into " + table + "(";
        int n = 0;
        for(auto e : data.keys())
        {
            if (n!=0)  sql += ", ";
            n++;
            sql += e;
        }
    sql += ") ";
    sql += "values (";
        n = 0;
        for(auto e : data.values())
        {
            if (n!=0)  sql += ", ";
            n++;
            sql += e;
//            debug << "-" << e;
//            debug << "-" << sql;
//            debug << "\n";  debug.flush();
        }
    sql += ") ; ";

//    debug << "-" << " >> SQL >> " << "\n";
    debug << "- ( " << sql << " )\n";
    debug << "\n";  debug.flush();

    QSqlQuery query(m_db);
    bool res = query.exec(sql);

    if (res) {
        //m_db.commit();
    } else {
        //m_db.rollback();
    }

    return res;
}


bool db_manager::select_map(QString sql, map_table &data)
{
    //m_db.transaction();
     debug << "-" << "   sql =" << " " << sql << "\n";

    data.clear();
    QSqlQuery query(m_db);
    bool res = query.exec(sql);

    if (res) {

        QSqlRecord rec = query.record();
//        debug << "-" << " get rec data ";

        while (query.next()) {

            map_rec list;
            for (int i=0; i<rec.count(); i++) {
                QString field = rec.fieldName(i);
                QString value = query.value(i).toString();
                bool is_null = query.value(i).isNull();
                list[field] = value;
                list.nulls[field] = is_null;
//                debug << "-" << "    " << " " << field << " " << list[field] << " " << list.nulls[field];
            }

            data.append(list);
        }
        //m_db.commit();
//        debug << "-" << " get rec data OK";
    } else {
       //m_db.rollback();

        debug << "-" << " >> SQL >> " << "\n";
        debug << "- ( " << sql << " )\n";
        debug << "-" << "DB: " + query.lastError().databaseText() << " " << ";  driver: " << " " << query.lastError().driverText();
        debug << "\n";  debug.flush();
    }

    return res;
}
