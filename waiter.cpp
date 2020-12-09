#include "waiter.h"
#include <QDebug>
#include <QThread>

waiter::waiter(QTextStream &debug) :
    debug(debug)
{
//    m_timer = new QTimer(this);
//    connect(m_timer, SIGNAL(timeout()), this, SLOT(update()));
//    m_timer->start(100);
//    m_cnt = 0;
}

//void waiter::timeout()
//{
//    m_cnt++;
//}

int waiter::wait(int step)
{
//    int c0 = m_cnt;
    int n = 0;
    debug << "-" << " ~ wait" << " " << step << " " << "sec.";
    debug << "\n";  debug.flush();
    QThread::msleep(1000*step);
    return n;
}
