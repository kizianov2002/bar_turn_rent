#ifndef WAITER_H
#define WAITER_H

#include <QTextStream>

#include <QObject>
#include <QTimer>

class waiter : public QObject
{
    Q_OBJECT
    QTextStream &debug;

public:
    waiter(QTextStream &debug);

//    int m_cnt;
    int wait(int step=5);

//public slots:
//    void timeout();
};

#endif // WAITER_H
