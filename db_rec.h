#ifndef DB_REC_H
#define DB_REC_H

#include <QString>
#include <QMap>
#include <QDateTime>

// массивы строк фиксированной структуры
typedef QStringList db_rec;
typedef QList<db_rec> db_table;

// массивы именованных строк произвольной структуры
struct map_rec: QMap<QString, QString>    // <поле, значение>
{
    QMap<QString, bool> nulls;

    void add (QString name, QString value) {
        insert(name, value);  }
    void addS(QString name, QString value) {
        insert(name, "'" + value + "'");  }
    void addI(QString name, int value) {
        insert(name, QString::number(value));  }
    void addF(QString name, double value) {
        insert(name, QString::number(value));  }
    void addD(QString name, QDate value) {
        insert(name, "'" + value.toString("yyyy-MM-dd") + "'");  }
    void addT(QString name, QTime value) {
        insert(name, "'" + value.toString("hh:mm:ss:zzz") + "'");  }
    void addDT(QString name, QDateTime value) {
        insert(name, "'" + value.toString("yyyy-MM-dd hh:mm:ss:zzz") + "'");  }

    QString toString() {
        QString str;
        QMapIterator<QString, QString> i(*this);
        int n = 0;
        while (i.hasNext()) {
            i.next();
            str += QString::number(n) + ") " + i.key() + ": " + i.value();
            n++;
        }
        return str;
    }
};

typedef QList<map_rec> map_table;


#endif // DB_REC_H
