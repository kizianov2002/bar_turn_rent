#ifndef SDK_MANAGER_H
#define SDK_MANAGER_H

#include <QObject>
#include <QDebug>

#include <ActiveQt/QAxBase>
#include <ActiveQt/QAxObject>
#include <QSettings>
#include <QDate>
#include <windows.h>
#include <QTextStream>

class sdk_manager : public QObject
{
    Q_OBJECT
    QSettings &settings;    
    QTextStream &debug;

public:
    sdk_manager(QSettings &settings, QTextStream &debug);
    void disconnect();

    QAxObject *comX, *xml_dom, *xml_dom_err, *word;

    void addStaff(QString lastName, QString firstName, QString middleName, int tabelId, int subdiv_id, qint64 cardNum, int shablon_id, QDate dBegin, QDate dEnd);
    void addStaffMass(int count, QString lastName, QString middleName, int tabelId, int subdiv_id, qint64 cardNum, int shablon_id, QDate dBegin, QDate dEnd);

    void passCard(int StaffID, qint64 CardNum, int CardID);
    void denyCard(int StaffID, qint64 CardNum, int CardID);
    void denyAllCards(QString reason);
    void saveAllCards();

    QString getEvents(QDate dateFrom=QDate::currentDate(), QDate dateTo=QDate::currentDate().addDays(1));

    void remCard (QString ID, int CardNum, QString reason);

//    qint64 Hex2Ten (qint64 cardNum);
//    qint64 Ten2Hex (qint64 cardNumCorr);
    qint64 saveCardNum (qint64 cardNum);
    qint64 readCardNum (qint64 cardNumCorr);
public slots:

    void on_comX_exception(int n, const QString &s1, const QString &s2, const QString &s3) {
 //       debug << "-" << "   !!! comX_exception - " << " " << n;
 //       debug << "-" << "   !!! s1 - " << " " << s1;
 //       debug << "-" << "   !!! s2 - " << " " << s2;
 //       debug << "-" << "   !!! s3 - " << " " << s3;
    }
    void on_xml_dom_exception(int n, const QString &s1, const QString &s2, const QString &s3) {
//        debug << "-" << "   !!! comX_exception - " << " " << n;
//        debug << "-" << "   !!! s1 - " << " " << s1;
//        debug << "-" << "   !!! s2 - " << " " << s2;
//        debug << "-" << "   !!! s3 - " << " " << s3;
    }
    void on_xml_dom_err_exception(int n, const QString &s1, const QString &s2, const QString &s3) {
//        debug << "-" << "   !!! comX_exception - " << " " << n;
//        debug << "-" << "   !!! s1 - " << " " << s1;
//        debug << "-" << "   !!! s2 - " << " " << s2;
//        debug << "-" << "   !!! s3 - " << " " << s3;
    }
};

#endif // SDK_MANAGER_H
