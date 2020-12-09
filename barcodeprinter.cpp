#include "barcodeprinter.h"
#include "db_manager.h"
#include <QDateTime>
#include <QFont>
#include "json.h"

BarcodePrinter::BarcodePrinter(QString name, QSettings &settings, QTextStream &debug, db_manager *DB,  QObject *parent) :
    QObject(parent), settings(settings), debug(debug), name(name), mPrinter(nullptr), mPainter(nullptr), m_DB(DB),
    printerConfigured(FALSE)
{
    settings.sync();
    // страница
    int PgW = settings.value(name+"/PgW", "80").toInt();
    int PgH = settings.value(name+"/PgH","500").toInt();
    int MgT = settings.value(name+"/MgT",  "1").toInt();
    int MgB = settings.value(name+"/MgB",  "1").toInt();
    int MgL = settings.value(name+"/MgL",  "1").toInt();
    int MgR = settings.value(name+"/MgR",  "1").toInt();

    int X0 = settings.value("print/X0", "0").toInt();
    int Xa = settings.value("print/Xa","20").toInt();
    int Y0 = settings.value("print/Y0", "0").toInt();
    int X1 = settings.value("print/X1","70").toInt();
    int Y1 = settings.value("print/Y1","70").toInt();
    int St = settings.value("print/St", "4").toInt();
    QString font = settings.value("print/font", "PT Sans").toString();
    int text_size = settings.value("print/size", "8").toInt();
//    QString sprn1 = settings.value("print/printer1", "Brother PJ-663").toString();
//    QString sprn2 = settings.value("print/printer2", "Bullzip PDF Printer").toString();
//    QString sprn3 = settings.value("print/printer3", "ZDesigner TLP 2824 Plus (ZPL)").toString();

    int BX = settings.value("barcode/X0", "0").toInt();
    int BY = settings.value("barcode/Y0", "0").toInt();
    int BH = settings.value("barcode/height", "8").toInt();
    int BW = settings.value("barcode/width","35").toInt();
    int BS = settings.value("barcode/size","24").toInt();
    QString Bf = settings.value("barcode/font128", "Code 128").toString();

    settings.setValue("print/X0", X0);
    settings.setValue("print/Xa", Xa);
    settings.setValue("print/X1", X1);
    settings.setValue("print/St", St);
    settings.setValue("print/Y0", Y0);
    settings.setValue("print/Y1", Y1);
    settings.setValue("print/font", font);
    settings.setValue("print/size", text_size);
//    settings.setValue("print/printer1", sprn1);
//    settings.setValue("print/printer2", sprn2);
//    settings.setValue("print/printer3", sprn3);

    settings.setValue("barcode/X0", BX);
    settings.setValue("barcode/Y0", BY);
    settings.setValue("barcode/height", BH);
    settings.setValue("barcode/width", BW);
    settings.setValue("barcode/size", BS);
    settings.setValue("barcode/font128", Bf);

    settings.setValue(name+"/PgW", PgW);
    settings.setValue(name+"/PgH", PgH);
    settings.setValue(name+"/MgT", MgT);
    settings.setValue(name+"/MgB", MgB);
    settings.setValue(name+"/MgL", MgL);
    settings.setValue(name+"/MgR", MgR);
    settings.sync();

    qDebug() << "-" << "Use printer:" << name;
}

QStringList BarcodePrinter::getAvailablePrinters()
{
    QStringList availablePrinters;

    QPrinterInfo pInfo;
    foreach (QPrinterInfo info, pInfo.availablePrinters()) {
        availablePrinters << " " << info.printerName();
    }

    return availablePrinters;
}

bool BarcodePrinter::configurePrinter(QString printerName)
{
    delete mPrinter;
    mPrinter = new QPrinter();

    bool printerAvailable = getAvailablePrinters().contains(printerName);
    if(printerAvailable)
    {
        mPrinter->setPrinterName(printerName);
        m_printerName = printerName;
        configurePage();

        qDebug() << "Printer initialized:" << printerName;
        debug << "Printer initialized:" << printerName;
        debug << "\n";  debug.flush();
    }
    else //If the printer is unavailable, generate pdf instead
    {
        mPrinter->setOutputFileName("c:/TEMP/print.pdf");
        mPrinter->setOutputFormat(QPrinter::PdfFormat);
        configurePage();

        debug << "-" << "Printer initialized to generate pdf";
        debug << "\n";  debug.flush();
    }

    printerConfigured = TRUE;

    return printerAvailable;

}

void BarcodePrinter::configurePage()
    {
    debug << "-" << m_printerName << " " << name;
    debug << "\n";  debug.flush();
    settings.sync();
    int X0 = settings.value("print/X0", "0").toInt();
    int Xa = settings.value("print/Xa", "20").toInt();
    int Y0 = settings.value("print/Y0", "0").toInt();
    int X1 = settings.value("print/X1","70").toInt();
    int Y1 = settings.value("print/Y1","70").toInt();

    PgW = settings.value(name+"/PgW","30").toInt();
    PgH = settings.value(name+"/PgH","18").toInt();
    MgT = settings.value(name+"/MgT", "1").toInt();
    MgB = settings.value(name+"/MgB", "1").toInt();
    MgL = settings.value(name+"/MgL", "1").toInt();
    MgR = settings.value(name+"/MgR", "1").toInt();
    Resolution = settings.value(name+"/Resolution", "203").toInt();
    debug << "-" << PgW << " " << PgH << " " << MgT << " " << MgB << " " << MgL << " " << MgR << " " << Resolution;
    debug << "\n";  debug.flush();
    //
    mPrinter->setColorMode(QPrinter::GrayScale);
    mPrinter->setPageSizeMM(QSizeF(PgW-MgL-MgR, PgH-MgT-MgB));
    mPrinter->setPaperSize(QSizeF(PgW, PgH), QPrinter::Millimeter);
    mPrinter->setResolution(Resolution);
    mPrinter->setPageMargins(MgL, MgT, MgR, MgB, QPrinter::Millimeter);
//    mPrinter->setOrientation(name=="print" ? QPrinter::Portrait : QPrinter::Landscape);
//    debug << "-" << "page" << " " << (PgW-MgL-MgR) << " " << "X" << " " << (PgH-MgT-MgB) << " " << (name=="print" ? "Portrait" : "Landscape");
//    debug << "\n";  debug.flush();
}


void BarcodePrinter::printTicket(QString barcodeText,  QString type)
{
    if(!printerConfigured) {
        debug << "-" << "Printer not configured, abort.";
        debug << "\n";  debug.flush();
        return;
    }

    settings.sync();
    int X0 = settings.value("barcode/X0", "0").toInt();
    int Y0 = settings.value("barcode/Y0", "0").toInt();
    int BH = settings.value("barcode/height", "8").toInt();
    int BW = settings.value("barcode/width","30").toInt();
    int PgH= settings.value("barcode/PgH","20").toInt();
    QString font = settings.value("print/font", "PT Sans").toString();
    int text_size = settings.value("print/size", "8").toInt();
    int BS = settings.value("barcode/size","5").toInt();

    int ch_Ten = settings.value("turnside/num_Ten", "0").toInt();
    int ch_HexUp = settings.value("turnside/num_HexUp", "0").toInt();
    int ch_HexDn = settings.value("turnside/num_HexDn", "0").toInt();

    if (ch_HexDn>0) {
        barcodeText = QString(QString("00000000") + QString::number(barcodeText.toInt(), 16).toLower()).right(8);
    } else if (ch_HexUp>0) {
        barcodeText = QString(QString("00000000") + QString::number(barcodeText.toInt(), 16).toUpper()).right(8);
    }

    mPrinter->setCopyCount(1);

    double MmToDot = 8; //Printer DPI = 203 => 8 dots per mm

    delete mPainter;
    mPainter = new QPainter(mPrinter);

    mPainter->drawLine((X0)*MmToDot, (Y0+1 )*MmToDot, (X0+BW+5)*MmToDot, (Y0+1 )*MmToDot);

    QRect barcodeRect = QRect(   X0*MmToDot, (Y0+2   )*MmToDot, BW*MmToDot, BH*MmToDot );
    QRect barTypeRect = QRect(   (X0+5)*MmToDot, (Y0+BH+3)*MmToDot, (BW-10)*MmToDot, 3 *MmToDot );
    QRect barTextRect = QRect(   (X0+5)*MmToDot, (Y0+BH+3)*MmToDot, (BW-10)*MmToDot, 3 *MmToDot );

    mPainter->drawLine((X0)*MmToDot, (PgH-2)*MmToDot, (X0+BW+5)*MmToDot, (PgH-2)*MmToDot);

    mPainter->setFont(QFont(font, text_size-2));
    mPainter->drawText(barTypeRect, Qt::AlignLeft, type);

    barcode(barcodeRect, barTextRect, barcodeText, BS);

    mPainter->end();

    qDebug() << "-" << "Print ticket " << barcodeText << type;
    debug << "-" << "Print ticket " << barcodeText << type;
    debug << "\n";  debug.flush();
}

void BarcodePrinter::printTicket(QString barcodeText)
{
    if (!printerConfigured) {
        debug << "-" << "Printer not configured, abort.";
        debug << "\n";  debug.flush();
        return;
    }

    settings.sync();
    int X0 = settings.value("barcode/X0", "0").toInt();
    int Y0 = settings.value("barcode/Y0", "0").toInt();
    int BH = settings.value("barcode/height", "8").toInt();
    int BW = settings.value("barcode/width","30").toInt();
    int BS = settings.value("barcode/size","5").toInt();

    int ch_Ten = settings.value("turnside/num_Ten", "0").toInt();
    int ch_HexUp = settings.value("turnside/num_HexUp", "0").toInt();
    int ch_HexDn = settings.value("turnside/num_HexDn", "0").toInt();

    if (ch_HexDn>0) {
        barcodeText = QString(QString("00000000") + QString::number(barcodeText.toInt(), 16).toLower()).right(8);
    } else if (ch_HexUp>0) {
        barcodeText = QString(QString("00000000") + QString::number(barcodeText.toInt(), 16).toUpper()).right(8);
    }

    QString Bf = settings.value("barcode/font128", "Code 128").toString();
    QString font = settings.value("print/font", "PT Sans").toString();
    int text_size = settings.value("print/size", "8").toInt();

    mPrinter->setCopyCount(1);

    double MmToDot = 8; //Printer DPI = 203 => 8 dots per mm

    delete mPainter;
    mPainter = new QPainter(mPrinter);

    mPainter->drawLine((X0)*MmToDot, (Y0+1 )*MmToDot, (X0+BW+5)*MmToDot, (Y0+1 )*MmToDot);

    QRect barcodeRect = QRect(   X0*MmToDot, (Y0+2   )*MmToDot, BW*MmToDot, BH*MmToDot );
    QRect barTextRect = QRect(   X0*MmToDot, (Y0+BH+3)*MmToDot, BW*MmToDot, 2 *MmToDot );

    mPainter->drawLine((X0)*MmToDot, (Y0+BH+6)*MmToDot, (X0+BW+5)*MmToDot, (Y0+BH+6)*MmToDot);

    barcode(barcodeRect, barTextRect, barcodeText, BS);

    mPainter->end();

    qDebug() << "-" << "Print ticket " << barcodeText;
    debug << "-" << "Print ticket " << barcodeText;
    debug << "\n";  debug.flush();
}


bool BarcodePrinter::printTcStack_start()
{
    if (!printerConfigured) {
        debug << "-" << "Printer not configured, abort.";
        debug << "\n";  debug.flush();
        return false;
    }
    mPrinter->setCopyCount(1);
    delete mPainter;
    mPainter = new QPainter(mPrinter);

    qDebug() << "-" << "printTcStack_start";
    debug << "-" << "printTcStack_start ";
    debug << "\n";  debug.flush();

    return true;
}

void BarcodePrinter::printTcStack(QString barcodeText,  QString type)
{
    settings.sync();
    int X0 = settings.value("barcode/X0", "0").toInt();
    int Y0 = settings.value("barcode/Y0", "0").toInt();
    int BH = settings.value("barcode/height", "8").toInt();
    int BW = settings.value("barcode/width","30").toInt();
    int BS = settings.value("barcode/size","5").toInt();
    int PgH= settings.value("barcode/PgH","20").toInt();

    int ch_Ten = settings.value("turnside/num_Ten", "0").toInt();
    int ch_HexUp = settings.value("turnside/num_HexUp", "0").toInt();
    int ch_HexDn = settings.value("turnside/num_HexDn", "0").toInt();

    if (ch_HexDn>0) {
        barcodeText = QString(QString("00000000") + QString::number(barcodeText.toInt(), 16).toLower()).right(8);
    } else if (ch_HexUp>0) {
        barcodeText = QString(QString("00000000") + QString::number(barcodeText.toInt(), 16).toUpper()).right(8);
    }

    QString Bf = settings.value("barcode/font128", "Code 128").toString();
    QString font = settings.value("print/font", "PT Sans").toString();
    int text_size = settings.value("print/size", "8").toInt();

    double MmToDot = 8; //Printer DPI = 203 => 8 dots per mm
    mPainter->drawLine((X0)*MmToDot, (Y0+1 )*MmToDot, (X0+BW+5)*MmToDot, (Y0+1 )*MmToDot);

    QRect barcodeRect = QRect(   X0*MmToDot, (Y0+2   )*MmToDot, BW*MmToDot, BH*MmToDot );
    QRect barTypeRect = QRect(   (X0+5)*MmToDot, (Y0+BH+3)*MmToDot, (BW-10)*MmToDot, 3 *MmToDot );
    QRect barTextRect = QRect(   (X0+5)*MmToDot, (Y0+BH+3)*MmToDot, (BW-10)*MmToDot, 3 *MmToDot );

    mPainter->drawLine((X0)*MmToDot, (PgH-2)*MmToDot, (X0+BW+5)*MmToDot, (PgH-2)*MmToDot);

    mPainter->setFont(QFont(font, text_size-2));
    mPainter->drawText(barTypeRect, Qt::AlignLeft, type);

    barcode(barcodeRect, barTextRect, barcodeText, BS);

    mPrinter->newPage();

    qDebug() << "-" << "printTcStack" << barcodeText << type;
    debug << "-" << "printTcStack" << barcodeText << type;
    debug << "\n";  debug.flush();

}

void BarcodePrinter::printTcStack(QString barcodeText)
{
    settings.sync();
    int X0 = settings.value("barcode/X0", "0").toInt();
    int Y0 = settings.value("barcode/Y0", "0").toInt();
    int BH = settings.value("barcode/height", "8").toInt();
    int BW = settings.value("barcode/width","30").toInt();
    int BS = settings.value("barcode/size","5").toInt();

    int ch_Ten = settings.value("turnside/num_Ten", "0").toInt();
    int ch_HexUp = settings.value("turnside/num_HexUp", "0").toInt();
    int ch_HexDn = settings.value("turnside/num_HexDn", "0").toInt();

    if (ch_HexDn>0) {
        barcodeText = QString(QString("00000000") + QString::number(barcodeText.toInt(), 16).toLower()).right(8);
    } else if (ch_HexUp>0) {
        barcodeText = QString(QString("00000000") + QString::number(barcodeText.toInt(), 16).toUpper()).right(8);
    }

    QString Bf = settings.value("barcode/font128", "Code 128").toString();
    QString font = settings.value("print/font", "PT Sans").toString();
//    int text_size = settings.value("print/size", "8").toInt();

    double MmToDot = 8; //Printer DPI = 203 => 8 dots per mm
    mPainter->drawLine((X0)*MmToDot, (Y0+1 )*MmToDot, (X0+BW+5)*MmToDot, (Y0+1 )*MmToDot);

    QRect barcodeRect = QRect(   X0*MmToDot, (Y0+2   )*MmToDot, BW*MmToDot, BH*MmToDot );
    QRect barTextRect = QRect(   X0*MmToDot, (Y0+BH+3)*MmToDot, BW*MmToDot, 2 *MmToDot );

    mPainter->drawLine((X0)*MmToDot, (Y0+BH+6)*MmToDot, (X0+BW+5)*MmToDot, (Y0+BH+6)*MmToDot);

    barcode(barcodeRect, barTextRect, barcodeText, BS);

    mPrinter->newPage();

    qDebug() << "-" << "printTcStack" << barcodeText;
    debug << "-" << "printTcStack" << barcodeText;
    debug << "\n";  debug.flush();

}

void BarcodePrinter::printTcStack_end()
{
    mPainter->end();

    qDebug() << "-" << "printTcStack_end";
    debug << "-" << "printTcStack_end ";
    debug << "\n";  debug.flush();

}


bool BarcodePrinter::printTcStrip_start()
{
    if (!printerConfigured) {
        debug << "-" << "Printer not configured, abort.";
        debug << "\n";  debug.flush();
        return false;
    }
    mPrinter->setCopyCount(1);
    delete mPainter;
    mPainter = new QPainter(mPrinter);

    settings.sync();
    double MmToDot = 8; //Printer DPI = 203 => 8 dots per mm
    Y = settings.value("barcode/Y0", "0").toInt();

    qDebug() << "-" << "printTcStrip_start";
    debug << "-" << "printTcStrip_start ";
    debug << "\n";  debug.flush();

    return true;
}

void BarcodePrinter::printTcStrip(QString barcodeText,  QString type)
{
    double MmToDot = 8; //Printer DPI = 203 => 8 dots per mm

    settings.sync();
    int X0 = settings.value("barcode/X0", "0").toInt();
    int Y0 = Y+3;
    int BH = settings.value("barcode/height", "8").toInt();
    int BW = settings.value("barcode/width","30").toInt();
    int BS = settings.value("barcode/size","5").toInt();
    int PgH= settings.value("barcode/PgH","20").toInt();

    int ch_Ten = settings.value("turnside/num_Ten", "0").toInt();
    int ch_HexUp = settings.value("turnside/num_HexUp", "0").toInt();
    int ch_HexDn = settings.value("turnside/num_HexDn", "0").toInt();

    if (ch_HexDn>0) {
        barcodeText = QString(QString("00000000") + QString::number(barcodeText.toInt(), 16).toLower()).right(8);
    } else if (ch_HexUp>0) {
        barcodeText = QString(QString("00000000") + QString::number(barcodeText.toInt(), 16).toUpper()).right(8);
    }

    QString Bf = settings.value("barcode/font128", "Code 128").toString();
    QString font = settings.value("print/font", "PT Sans").toString();
    int text_size = settings.value("print/size", "8").toInt();

    mPainter->drawLine((X0)*MmToDot, (Y0+1 )*MmToDot, (X0+BW+5)*MmToDot, (Y0+1 )*MmToDot);

    QRect barcodeRect = QRect(   X0*MmToDot, (Y0+2   )*MmToDot, BW*MmToDot, BH*MmToDot );
    QRect barTypeRect = QRect(   (X0+5)*MmToDot, (Y0+BH+3)*MmToDot, (BW-10)*MmToDot, 3 *MmToDot );
    QRect barTextRect = QRect(   (X0+5)*MmToDot, (Y0+BH+3)*MmToDot, (BW-10)*MmToDot, 3 *MmToDot );

    mPainter->drawLine((X0)*MmToDot, (PgH-2)*MmToDot, (X0+BW+5)*MmToDot, (PgH-2)*MmToDot);

    mPainter->setFont(QFont(font, text_size-2));
    mPainter->drawText(barTypeRect, Qt::AlignLeft, type);

    barcode(barcodeRect, barTextRect, barcodeText, BS);

    Y = Y+BH+12;
    if (Y>PgH-BH-18){
        Y = 0;
        mPrinter->newPage();
    }

    qDebug() << "-" << "printTcStrip" << barcodeText << type;
    debug << "-" << "printTcStrip" << barcodeText << type;
    debug << "\n";  debug.flush();

}

void BarcodePrinter::printTcStrip(QString barcodeText)
{
    double MmToDot = 8; //Printer DPI = 203 => 8 dots per mm

    settings.sync();
    int X0 = settings.value("barcode/X0", "0").toInt();
    int Y0 = Y+3;
    int BH = settings.value("barcode/height", "8").toInt();
    int BW = settings.value("barcode/width","30").toInt();
    int BS = settings.value("barcode/size","5").toInt();

    int ch_Ten = settings.value("turnside/num_Ten", "0").toInt();
    int ch_HexUp = settings.value("turnside/num_HexUp", "0").toInt();
    int ch_HexDn = settings.value("turnside/num_HexDn", "0").toInt();

    if (ch_HexDn>0) {
        barcodeText = QString(QString("00000000") + QString::number(barcodeText.toInt(), 16).toLower()).right(8);
    } else if (ch_HexUp>0) {
        barcodeText = QString(QString("00000000") + QString::number(barcodeText.toInt(), 16).toUpper()).right(8);
    }

    QString Bf = settings.value("barcode/font128", "Code 128").toString();
    QString font = settings.value("print/font", "PT Sans").toString();
    int text_size = settings.value("print/size", "8").toInt();

    mPainter->drawLine((X0)*MmToDot, (Y0+1 )*MmToDot, (X0+BW+5)*MmToDot, (Y0+1 )*MmToDot);

    QRect barcodeRect = QRect(   X0*MmToDot, (Y0+2   )*MmToDot, BW*MmToDot, BH*MmToDot );
    QRect barTextRect = QRect(   X0*MmToDot, (Y0+BH+3)*MmToDot, BW*MmToDot, 2 *MmToDot );

    mPainter->drawLine((X0)*MmToDot, (Y0+BH+6)*MmToDot, (X0+BW+5)*MmToDot, (Y0+BH+6)*MmToDot);

    barcode(barcodeRect, barTextRect, barcodeText, BS);

    Y = Y +BH +10;
    if (Y>PgH-BH-12) {
        Y = 0;
        mPrinter->newPage();
    }
    qDebug() << "-" << "printTcStrip" << barcodeText;
    debug << "-" << "printTcStrip" << barcodeText;
    debug << "\n";  debug.flush();

}

void BarcodePrinter::printTcStrip_end()
{
    mPainter->end();

    qDebug() << "-" << "printTcStrip_end";
    debug << "-" << "printTcStrip_end ";
    debug << "\n";  debug.flush();

}


QString min2time(int mins) {
    QString res = (mins>=60)
                ? (QString::number(mins/60)+"ч. "+QString::number(mins%60)+"м.")
                : (QString::number(mins)+"м.");
    return res;
}

QString rub2price(int rub) {
    return QString::number(rub)+"р.";
}

QString curDate() {
    return QDate::currentDate().toString("dd.MM.yyyy");
}


bool BarcodePrinter::printDocument(QString json_s, int cnt)
{
    if (!printerConfigured) {
        debug << "-" << "Printer not configured, abort.";
        debug << "\n";  debug.flush();
        return false;
    }
    debug << "-" << "json_s |" << " " << json_s << " " << "|";
    debug << "\n";  debug.flush();

    int X0 = settings.value("print/X0", "0").toInt() +5;
    int Xa = settings.value("print/Xa","20").toInt();
    int Y0 = settings.value("print/Y0", "0").toInt();
    int X1 = settings.value("print/X1","70").toInt() -5;
    int Y1 = settings.value("print/Y1","1000").toInt();
    int Xd = (X0+(X1-X0)/2)+5;
    int St = settings.value("print/St", "4").toInt();
    QString font = settings.value("print/font", "PT Sans").toString();
    int text_size = settings.value("print/size", "8").toInt();
    QString Bf = settings.value("barcode/font128", "Code 128").toString();
    int     BS = 22;   // settings.value("barcode/size","5").toInt();

    QFont textFont     = QFont(font, text_size, QFont::Normal);
    QFont textBoldFont = QFont(font, text_size, QFont::Bold);
    QFont textBol2Font = QFont(font, text_size+2, QFont::Bold);
    QFont barTextFont  = QFont(font, 5, QFont::Normal);
    QFont barcodeFont  = QFont(Bf, 22, QFont::Normal);
//    barcodeFont.setLetterSpacing(QFont::AbsoluteSpacing,297784850.0);

    mPrinter->setCopyCount(1);

    bool ok;
    // json is a QString containing the JSON data
    QtJson::JsonObject json = QtJson::parse(json_s, ok).toMap();
    if(!ok) {
        qFatal("An error occurred during parsing");
        return false;
    }

    delete mPainter;
    mPainter = new QPainter(mPrinter);

    double MmToDot = 8; //Printer DPI = 203 => 8 dots per mm
    mPainter->setFont(textFont);

    // получим данные заголовка
    QtJson::JsonObject json_order = json["order"].toMap();

    QString header = "Прокатный билет № ???";
    QString number = "дата: ???";

    QString date_s = json_order["order_date"].toString();
    date_s.replace("T", " ").replace("t", " ");
    QDateTime dt = QDateTime::fromString(date_s, "yyyy-MM-dd hh:mm:ss");
    if (dt.isNull() || !dt.isValid())
        dt = QDateTime::currentDateTime();

    header = "Прокатный билет № " + json_order["order_number"].toString() + "  (экз. " + QString::number(cnt) + ")";
    number = "на дату: " + dt.toString("dd.MM.yyyy");
    QString deposit_a = "залог:";
    QString deposit   = json_order["deposit"].toString();
    QString comment_a = "комментарий:";
    QString comment   = json_order["comment"].toString();

    int Y = Y0;
    QRect headerRect    = QRect(X0*MmToDot,  Y*MmToDot, (X1-X0)*MmToDot, St*MmToDot);
    Y = Step(Y, St, Y1);
    QRect numberRect    = QRect(X0*MmToDot,  Y*MmToDot, (X1-X0)*MmToDot, St*MmToDot);
    Y = Step(Y, St, Y1);

    mPainter->drawText(headerRect, Qt::AlignCenter, header);
    mPainter->drawText(numberRect, Qt::AlignCenter, number);
    Y = Step(Y, St, Y1);

    // получим массив строк
    QVariantList json_inventory = json["inventory"].toList();
    // debug << "-" << "json_inventory: " << " " << json_inventory;

    for (int i=0; i<json_inventory.size(); i++) {
        QVariant json_inventory_rec = json_inventory[i];
        QMap<QString, QVariant> json_rec = json_inventory_rec.toMap();

        int id_rec = -1;
        int price_rec = -1;
        int duration_rec = -1;
        QString name_rec_a = "инвентарь:";
        QString name_rec = "???";
        QString comment_rec_a = "комментарий:";
        QString comment_rec = "???";
        QString identifier;
        QString identiCode;
        int id_i=-1, id_ri=-1, id_rit=-1;
        QString dt_start_a = "начало проката:";
        QString dt_end_a   = "время возврата:";
        QDateTime dt_start, dt_end;

        id_rec = json_rec["id"].toString().toInt();
        QString price_rec_a = "стоимость:";
        price_rec = json_rec["price"].toString().toInt();

        // получим детализацию проката
        QtJson::JsonObject json_rent = json_rec["rentInventoryTariffInventory"].toMap();
        QString duration_rec_a = "тариф:";
        duration_rec = json_rent["duration"].toString().toInt();

        QtJson::JsonObject json_invent = json_rent["inventory"].toMap();

        name_rec = json_invent["name"].toString();
        comment_rec = json_invent["comment"].toString();

        // получим из БД данные инвентаря
        QString sql = "select ri.id as id_ri, rit.id as id_rit, i.id as id_i, i.identifier, ri.started_at "
                      "  from rent_inventory ri "
                      "  left join rent_inventory_tariff rit on(rit.id=ri.rent_inventory_tariff_id) "
                      "  left join inventory i on(i.id=rit.inventory_id) "
                      " where ri.id=" + QString::number(id_rec) + " ; ";
        if (m_DB) {
            map_rec rec;

            debug << "-" << sql << "\n";  debug.flush();

            m_DB->selectOne_map(sql, rec);
            id_i = rec["id_i"].toInt();
            id_ri = rec["id_ri"].toInt();
            id_rit = rec["id_rit"].toInt();
            //identifier = rec["identifier"];
            identifier = json_invent["identifier"].toString();
            // QString dt_str = rec["started_at"];
            QString dt_str = json_rec["started_at"].toString();

            debug << "-" << "identifier " << " '" << identifier << "'\n";  debug.flush();
            debug << "-" << "dt_str " << " " << dt_str << "\n";  debug.flush();

            dt_start = QDateTime::fromString(dt_str, "yyyy-MM-dd hh:mm:ss");
            dt_end = dt_start.addSecs(duration_rec*60);

            debug << "-" << "dt_start " << " " << dt_start.toString("hh:mm:ss") << "\n";  debug.flush();
            debug << "-" << "dt_end " << " " << dt_end.toString("hh:mm:ss") << "\n";  debug.flush();

        }

        // отобразим данные строки
        mPainter->drawLine(X0*MmToDot, Y*MmToDot, X1*MmToDot, Y*MmToDot);

        Y += 1;
        mPainter->setFont(textFont);
        QRect recNamRect_a  = QRect(X0*MmToDot,      Y*MmToDot, (Xa-2)*MmToDot,  St*MmToDot);
        QRect recNamRect    = QRect((X0+Xa)*MmToDot, Y*MmToDot, (X1-Xa)*MmToDot, St*MmToDot);
        mPainter->drawText(recNamRect_a, Qt::AlignRight, name_rec_a);
        mPainter->drawText(recNamRect,   Qt::AlignLeft,  name_rec);

        Y = Step(Y, St, Y1);
        mPainter->setFont(textFont);
        QRect recTStRect_a  = QRect(X0*MmToDot,      Y*MmToDot, (Xa-2)*MmToDot,  St*MmToDot);
        QRect recTStRect    = QRect((X0+Xa)*MmToDot, Y*MmToDot, (X1-Xa)*MmToDot, St*MmToDot);
        mPainter->drawText(recTStRect_a, Qt::AlignRight, dt_start_a);
        mPainter->drawText(recTStRect,   Qt::AlignLeft,  dt_start.toString("hh:mm"));

        Y = Step(Y, St, Y1);
        mPainter->setFont(textFont);
        QRect recDurRect_a  = QRect(X0*MmToDot,      Y*MmToDot,     (Xa-2)*MmToDot,  St*MmToDot);
        QRect recDurRect    = QRect((X0+Xa)*MmToDot, Y*MmToDot,     (X1-Xa)*MmToDot, St*MmToDot);
        QRect recBarcode    = QRect(Xd*MmToDot,      (Y-1)*MmToDot, (X1-Xd)*MmToDot, (St+3)*MmToDot);
        mPainter->drawText(recDurRect_a, Qt::AlignRight,   duration_rec_a);
        mPainter->drawText(recDurRect,   Qt::AlignLeft,    min2time(duration_rec));

        Y = Step(Y, St, Y1);
        mPainter->setFont(textFont);
        QRect recPrcRect_a  = QRect(X0*MmToDot,      Y*MmToDot,     (Xa-2)*MmToDot,  St*MmToDot);
        QRect recPrcRect    = QRect((X0+Xa)*MmToDot, Y*MmToDot,     (X1-Xa)*MmToDot, St*MmToDot);
        QRect recBarcode_t  = QRect(Xd*MmToDot,      (Y+2.5)*MmToDot, (X1-Xd-20)*MmToDot, (St-2)*MmToDot);
        mPainter->drawText(recPrcRect_a, Qt::AlignRight,   price_rec_a);
        mPainter->drawText(recPrcRect,   Qt::AlignLeft,    rub2price(price_rec));

        barcode(recBarcode, recBarcode_t, identifier, BS);

        Y = Step(Y, St, Y1);
        mPainter->setFont(textFont);
        QRect recTEnRect_a  = QRect(X0*MmToDot,      Y*MmToDot,       (Xa-2)*MmToDot,  St*MmToDot);
        QRect recTEnRect    = QRect((X0+Xa)*MmToDot, (Y-0.5)*MmToDot, (X1-Xa)*MmToDot, St*MmToDot);
        mPainter->setFont(textBoldFont);
        mPainter->drawText(recTEnRect_a, Qt::AlignRight, dt_end_a);
        mPainter->setFont(textBol2Font);
        mPainter->drawText(recTEnRect,   Qt::AlignLeft,  dt_end.toString("hh:mm"));

        Y = Step(Y, St, Y1);
        mPainter->setFont(textFont);
        QRect recComRect_a  = QRect(X0*MmToDot,      Y*MmToDot, (Xa-2)*MmToDot,  (St*1.5)*MmToDot);
        QRect recComRect    = QRect((X0+Xa)*MmToDot, Y*MmToDot, (X1-Xa)*MmToDot, (St*1.5)*MmToDot);
        mPainter->drawText(recComRect_a, Qt::AlignRight,   comment_rec_a);
        mPainter->drawText(recComRect,   Qt::AlignLeft,    comment_rec);

        Y = Step(Y, St*1.5, Y1);
    }
    // отобразим подвал
    mPainter->drawLine(X0*MmToDot, Y*MmToDot,       X1*MmToDot, Y*MmToDot);
    Y += 1;
    mPainter->drawLine(X0*MmToDot, (Y-0.5)*MmToDot, X1*MmToDot, (Y-0.5)*MmToDot);

    Y = Step(Y, St*0.5, Y1);

    mPainter->setFont(textFont);
    QRect depositRect_a  = QRect(X0*MmToDot,      Y*MmToDot, (Xa-2)*MmToDot,  St*MmToDot);
    QRect depositRect    = QRect((X0+Xa)*MmToDot, Y*MmToDot, (X1-Xa)*MmToDot, St*MmToDot);
    mPainter->drawText(depositRect_a, Qt::AlignRight, deposit_a);
    mPainter->drawText(depositRect,   Qt::AlignLeft,  deposit);

    Y = Step(Y, St, Y1);
    QRect commentRect_a  = QRect(X0*MmToDot,      Y*MmToDot, (Xa-2)*MmToDot,  (St*1.5)*MmToDot);
    QRect commentRect    = QRect((X0+Xa)*MmToDot, Y*MmToDot, (X1-Xa)*MmToDot, (St*1.5)*MmToDot);
    mPainter->drawText(commentRect_a, Qt::AlignRight, comment_a);
    mPainter->drawText(commentRect,   Qt::AlignLeft,  comment);

    Y = Step(Y, St*1.5, Y1);
    QRect dateTxtRect = QRect(X0*MmToDot, Y*MmToDot, (X0+Xa)*MmToDot, St*MmToDot);
    QRect signTxtRect = QRect((X0+10)*MmToDot, Y*MmToDot, (X1-X0)*MmToDot, St*MmToDot);
    mPainter->drawText(dateTxtRect, Qt::AlignHCenter, "дата:");
    mPainter->drawText(signTxtRect, Qt::AlignHCenter, "подпись:");

    Y = Step(Y, St, Y1);
    QRect dateSgnRect = QRect(X0*MmToDot, Y*MmToDot, (X0+Xa)*MmToDot, St*MmToDot);
    mPainter->drawText(dateSgnRect, Qt::AlignHCenter, QDate::currentDate().toString("dd.MM.yyyy"));

    mPainter->drawLine(((X0+(X1-X0)/2)- 0)*MmToDot, (Y+0)*MmToDot, ((X0+(X1-X0)/2)+20)*MmToDot, (Y-0)*MmToDot);
    mPainter->drawLine(((X0+(X1-X0)/2)- 0)*MmToDot, (Y+0)*MmToDot, ((X0+(X1-X0)/2)- 0)*MmToDot, (Y+8)*MmToDot);
    mPainter->drawLine(((X0+(X1-X0)/2)+20)*MmToDot, (Y+0)*MmToDot, ((X0+(X1-X0)/2)+20)*MmToDot, (Y+8)*MmToDot);
    mPainter->drawLine(((X0+(X1-X0)/2)- 0)*MmToDot, (Y+8)*MmToDot, ((X0+(X1-X0)/2)+20)*MmToDot, (Y+8)*MmToDot);

    mPainter->drawLine(X0*MmToDot, (Y+10)*MmToDot, X1*MmToDot, (Y+10)*MmToDot);

    mPainter->end();
    // debug << "-" << "Printing finished";
    debug << "\n";  debug.flush();

    if (cnt>1) {
        printDocument(json_s, cnt-1);
    }

    return true;
}


int BarcodePrinter::Step(int y, int st, int yMax) {
    y += st;
    if (y>yMax+st) {
        mPrinter->newPage();
        y = 0;
    }
    return y;
}

bool BarcodePrinter::barcode(QRect rectBar, QRect rectTxt, QString code, int BS)
{
    return barcodeEAN128(rectBar, rectTxt, code, BS);
}

bool BarcodePrinter::barcodeUPC(QRect rectBar, QRect rectTxt, QString code, int BS)
{
    return true;
}

bool BarcodePrinter::barcodeEAN8(QRect rectBar, QRect rectTxt, QString code, int BS)
{
    return true;
}

bool BarcodePrinter::barcodeEAN13(QRect rectBar, QRect rectTxt, QString code, int BS)
{
    // code
    QString bcCode = QString("000000000000" + QString::number(code.trimmed().toInt())).right(11);
//    debug << "-" << "  ->  bcCode" << " " << bcCode;
    int cn = 0;
    int S = 0;
    for (int i=0; i<11; i++) {
        int n = bcCode.mid(i,1).toInt();
        S += n*(i%2==1 ? 1 : 3);
    }
    cn = S % 10;
    bcCode = bcCode.right(1) + "!" + bcCode.left(7).right(6) + "-" + bcCode.right(5) + QString::number(cn).right(1) + "!";
//    debug << "-" << "  ->  bcCode+" << " " << bcCode;

    QString bcNorm = encodeEAN13(bcCode);
//    debug << "-" << "  ->  bcNorm" << " " << bcNorm;

    // ini
    QString font = settings.value("print/font", "PT Sans").toString();
    QString Bf = settings.value("barcode/font13", "EAN-13").toString();
//    debug << "-" << Bf;
    // fonts
    QFont textfont = QFont(font, 5, QFont::Normal);
    QFont barcodefont = QFont(Bf, BS, QFont::Normal);
//    barcodefont.setLetterSpacing(QFont::AbsoluteSpacing,297784850.0);

    // barcode
    mPainter->setFont(barcodefont);
    mPainter->drawText(rectBar, Qt::AlignLeft, bcNorm);
    // barcode text
    mPainter->setFont(textfont);
    mPainter->drawText(rectTxt, Qt::AlignHCenter, code);

    return true;
}

QString BarcodePrinter::encodeEAN13(QString code)
{
    unsigned char arr[20];
    int pos = 0;
    arr[0] = 204;  pos++;
    for (int i=0;i<=code.length(); i++) {
        if (i==1) {
            arr[pos] = 'F';
            pos++;
        }
        if (i==7) {
            arr[pos] = '|';
            pos++;
        }
        if (i==13) {
            arr[pos] = 'V';
            pos++;
            arr[pos] = 0;
        }
        if (i==0) {
            arr[pos] = 32;
        } else if (i<7) {
            if (code.at(i)=="0")       arr[pos] = 48;
            else if (code.at(i)=="1")  arr[pos] = 49;
            else if (code.at(i)=="2")  arr[pos] = 50;
            else if (code.at(i)=="3")  arr[pos] = 51;
            else if (code.at(i)=="4")  arr[pos] = 52;
            else if (code.at(i)=="5")  arr[pos] = 53;
            else if (code.at(i)=="6")  arr[pos] = 54;
            else if (code.at(i)=="7")  arr[pos] = 55;
            else if (code.at(i)=="8")  arr[pos] = 56;
            else if (code.at(i)=="9")  arr[pos] = 57;
            else arr[pos] = 49;

        } else if (i<13) {
            if (code.at(i)=="0")       arr[pos] = 80;
            else if (code.at(i)=="1")  arr[pos] = 81;
            else if (code.at(i)=="2")  arr[pos] = 82;
            else if (code.at(i)=="3")  arr[pos] = 83;
            else if (code.at(i)=="4")  arr[pos] = 84;
            else if (code.at(i)=="5")  arr[pos] = 85;
            else if (code.at(i)=="6")  arr[pos] = 86;
            else if (code.at(i)=="7")  arr[pos] = 87;
            else if (code.at(i)=="8")  arr[pos] = 88;
            else if (code.at(i)=="9")  arr[pos] = 89;
            else arr[pos] = 65;
        }
//        debug << "-" << "  arr - " << " " << arr;
//        debug << "\n";  debug.flush();
        pos++;
    }

    QString encoded = QString::fromLatin1((const char*)&arr);
//    debug << "-" << "  encoded - " << " " << encoded;
//    debug << "\n";  debug.flush();

    return encoded;
}

bool BarcodePrinter::barcodeEAN128(QRect rectBar, QRect rectTxt, QString code, int BS)
{
    // code
    QString bcNorm = encodeEAN128(code);

    // ini
    QString font = settings.value("print/font", "PT Sans").toString();
    QString Bf = settings.value("barcode/font128", "Code 128").toString();
    // fonts
    QFont textfont = QFont(font, 5, QFont::Normal);
    QFont barcodefont = QFont(Bf, BS, QFont::Normal);
//    barcodefont.setLetterSpacing(QFont::AbsoluteSpacing,297784850.0);

    // barcode
    mPainter->setFont(barcodefont);
    mPainter->drawText(rectBar, Qt::AlignLeft, bcNorm);
    // barcode text
    mPainter->setFont(textfont);
    mPainter->drawText(rectTxt, Qt::AlignRight, code);

    return true;
}

QString BarcodePrinter::encodeEAN128_old(QString code)
{
//    debug << "-" << "  code string - " << " " << code;
    QString encoded = " ";
    encoded += QChar(204);
    encoded += code;
    encoded += QChar(codeToChar(calculateCheckCharacter(code)));
    encoded += QChar(206);
//    debug << "-" << "  encoded - " << " " << encoded;

    return encoded;
}

QString BarcodePrinter::encodeEAN128(QString code)
{
//    debug << "-" << "  code string - " << " " << code;
//    debug << "\n";  debug.flush();
    unsigned char arr[16];
    int pos = 0;
   // arr[0] = 32;   pos++;
    arr[0] = 204;  pos++;
    for (int i=0;i<code.length(); i++) {
        if (code.at(i)=="0")  arr[pos] = 48;
        else if (code.at(i)=="1")  arr[pos] = 49;
        else if (code.at(i)=="2")  arr[pos] = 50;
        else if (code.at(i)=="3")  arr[pos] = 51;
        else if (code.at(i)=="4")  arr[pos] = 52;
        else if (code.at(i)=="5")  arr[pos] = 53;
        else if (code.at(i)=="6")  arr[pos] = 54;
        else if (code.at(i)=="7")  arr[pos] = 55;
        else if (code.at(i)=="8")  arr[pos] = 56;
        else if (code.at(i)=="9")  arr[pos] = 57;
        else arr[pos] = 49;
        pos++;
    }
    arr[pos] = codeToChar(calculateCheckCharacter(code));  pos++;
    arr[pos] = 206;  pos++;
    arr[pos] = 0;    pos++;

    QString encoded = QString::fromLatin1((const char*)&arr);
//    debug << "-" << "  encoded - " << " " << encoded;
//    debug << "\n";  debug.flush();

    return encoded;
}

unsigned char BarcodePrinter::calculateCheckCharacter(QString code)
{
//    debug << "-" << code;
//    debug << "\n";  debug.flush();
    QByteArray encapBarcode(code.toUtf8()); //Convert code to utf8

    //Calculate check character
    long long sum = 104; //The sum starts with the B Code start character value
    int weight = 1; //Initial weight is 1

    foreach(char ch, encapBarcode) {
        int code_char = charToCode((int)ch); //Calculate character code
        sum += code_char*weight; //add weighted code to sum
        weight++; //increment weight
    }

    int remain = sum%103; //The check character is the modulo 103 of the sum
    /*if (remain>94)  remain += 100;
    else  remain += 32;*/

//    debug << "-" << "  check - " << " " << remain;
//    debug << "\n";  debug.flush();
    return remain;
}

unsigned char BarcodePrinter::codeToChar(int code)
{
    //Calculate the font integer from the code integer
    if(code >= 95)
        code += 105;
    else
        code += 32;
    return code;
}

int BarcodePrinter::charToCode(int ch)
{
    return ch - 32;
}
