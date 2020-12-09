#pragma once

#include <QObject>
#include <QtPrintSupport/QtPrintSupport>
#include <QtPrintSupport/QPrinter>
#include <QTextStream>

#include "db_manager.h"

#define CODE128_B_START 204
#define CODE128_STOP 206

class BarcodePrinter : public QObject
{
    Q_OBJECT
    QString m_printerName;
    QSettings &settings;

    QTextStream &debug;

    QString name;
    int PgW;
    int PgH;
    int MgT;
    int MgB;
    int MgL;
    int MgR;
    int Resolution;

    QPrinter *mPrinter;
    QPainter *mPainter;

    db_manager *m_DB;

public:
    explicit BarcodePrinter(QString name,
                            QSettings &settings,
                            QTextStream &debug,
                            db_manager *DB = nullptr,
                            QObject *parent = nullptr);

    /** Lists all available printers */
    QStringList getAvailablePrinters();
    /** Configures the printer, if none selected defaults to pdf printer */
    bool configurePrinter(QString printerName = QString());
    /** Prints the text as a barcode */

    bool printDocument(QString json, int cnt=2);
    void printTicket(QString barcodeText, QString type);
    void printTicket(QString barcodeText);

    bool printTcStack_start();
    void printTcStack(QString barcodeText, QString type);
    void printTcStack(QString barcodeText);
    void printTcStack_end();
    
    int  Y;
    bool printTcStrip_start();
    void printTcStrip(QString barcodeText, QString type);
    void printTcStrip(QString barcodeText);
    void printTcStrip_end();

    /** Calculates the checksum character from the code */
    unsigned char calculateCheckCharacter(QString code);

private:
    /** Configures the page */
    void configurePage();

    unsigned char codeToChar(int code);
    int charToCode(int ch);

    bool printerConfigured;

    int Step(int y, int st, int yMax);

    bool barcode(QRect rectBar, QRect rectTxt, QString barcode, int BS);        // общая обёртка печати штрих-кода
    bool barcodeUPC(QRect rectBar, QRect rectTxt, QString barcode, int BS);     // UPC
    bool barcodeEAN8(QRect rectBar, QRect rectTxt, QString barcode, int BS);    // EAN8
    bool barcodeEAN13(QRect rectBar, QRect rectTxt, QString barcode, int BS);   // EAN13
    QString encodeEAN13(QString code);
    bool barcodeEAN128(QRect rectBar, QRect rectTxt, QString barcode, int BS);  // EAN128
    QString encodeEAN128(QString code);
    QString encodeEAN128_old(QString code);

    /** Adds start/check/stop characters to the code */
};



