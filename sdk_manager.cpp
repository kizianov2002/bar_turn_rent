#include "sdk_manager.h"
#include <QDateTime>

sdk_manager::sdk_manager(QSettings &settings, QTextStream &debug) :
    settings(settings), debug(debug)
{
    debug << "@@ " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "sdk_manager::sdk_manager()";
    debug << "\n";  debug.flush();

    xml_dom = new QAxObject(this);
    xml_dom->setControl("MSXML2.DOMDocument.3.0");
    if ( xml_dom->isNull() ) {
        debug << "-" << "xml_dom - reject COM";
        debug << "\n";  debug.flush();
//        return;
    } else {
        debug << "-" << "xml_dom - OK";
        debug << "\n";  debug.flush();
        // Control loaded OK; connecting to catch exceptions from control
        connect(
            xml_dom,
            SIGNAL(exception(int, const QString &, const QString &, const QString &)),
            this,
            SLOT(on_xml_dom_exception(int, const QString &, const QString &, const QString &)));
    }

    xml_dom_err = new QAxObject(this);
    xml_dom_err->setControl("MSXML2.DOMDocument.3.0");
    if ( xml_dom_err->isNull() ) {
        debug << "-" << "xml_dom_err - reject COM";
        debug << "\n";  debug.flush();
//        return;
    } else {
        debug << "-" << "xml_dom_err - OK";
        debug << "\n";  debug.flush();
        // Control loaded OK; connecting to catch exceptions from control
        connect(
            xml_dom_err,
            SIGNAL(exception(int, const QString &, const QString &, const QString &)),
            this,
            SLOT(on_xml_dom_err_exception(int, const QString &, const QString &, const QString &)));
    }

    // подключимся к PERCo SDK
    comX = new QAxObject(this);
    comX->setControl("PERCo_S20_SDK.ExchangeMain");
    if ( comX->isNull() ) {
        debug << "-" << "comX - reject COM";
        debug << "\n";  debug.flush();
        return;
    } else {
        debug << "-" << "comX - OK";
        debug << "\n";  debug.flush();
        // Control loaded OK; connecting to catch exceptions from control
        connect(
            comX,
            SIGNAL(exception(int, const QString &, const QString &, const QString &)),
            this,
            SLOT(on_comX_exception(int, const QString &, const QString &, const QString &)));
    }

    QString sdkUser = settings.value("SDK/user", "ADMIN").toString();
    QString sdkPass = settings.value("SDK/pass", "").toString();
    /*debug << "-" << "SetConnect PERCo" <<*/ comX->dynamicCall("SetConnect(QString, QString, QString, QString)", "127.0.0.1", "211", sdkUser, sdkPass).toString();

    debug << "OK " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "sdk_manager::sdk_manager()";
    debug << "\n";  debug.flush();
}

void sdk_manager::disconnect() {
    debug << "@@ " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "sdk_manager::disconnect()";
    debug << "\n";  debug.flush();

    /*debug << "-" << "SetConnect PERCo" <<*/ comX->dynamicCall("DisConnect()").toString();

    debug << "OK " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "sdk_manager::disconnect()";
    debug << "\n";  debug.flush();
}


void sdk_manager::addStaff(QString lastName, QString firstName, QString middleName, int tabel_id, int subdiv_id, qint64 cardNum, int shablon_id, QDate dBegin, QDate dEnd)
{
    debug << "@@ " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "sdk_manager::addStaff()";
    debug << "\n";  debug.flush();

    QString xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>"
                  "<documentrequest type=\"staff\">"
                  " <login loginname=\"login name\">"
                  "  <workmode mode=\"append\">"
                  "   <staff id_external=\"\" last_name=\"#LAST#\" first_name=\"#FIRST#\" middle_name=\"#MIDDLE#\""
                  "          tabel_id=\"#TABEL#\" subdiv_id_external=\"\""
                  "          appoint_id_external=\"\" graph_id_external=\"\" subdiv_id_internal=\"#SUBDIV#\""
                  "          appoint_id_internal=\"1\" graph_id_internal=\"0\" date_begin=\"#DBEGIN#\""
                  "          photo=\"#PHOTO#\">"
                  "    <identifiers>"
                  "     <identifier type_identifier=\"number\" identifier=\"#CARDNUM#\""
                  "                 date_begin_action=\"#DBEGIN#\" date_end_action=\"#DEND#\" prohibit=\"#PROHIBIT#\""
                  "                 shablon_id_external=\"\" shablon_id_internal=\"#SHABLON#\" />"
                  "    </identifiers>"
                  "   </staff>"
                  "  </workmode>"
                  " </login>"
                  "</documentrequest>";

    xml.replace("#LAST#",    lastName)
       .replace("#FIRST#",   firstName)
       .replace("#MIDDLE#",  middleName)
       .replace("#TABEL#",   QString::number(tabel_id))
       .replace("#SUBDIV#",  QString::number(subdiv_id))
       .replace("#PHOTO#",   "")
       .replace("#DBEGIN#",  dBegin.toString("dd.MM.yyyy"))
       .replace("#DEND#",    dEnd.toString("dd.MM.yyyy"))
       .replace("#CARDNUM#", QString::number(saveCardNum(cardNum)))
       .replace("#PROHIBIT#","true")
       .replace("#SHABLON#", QString::number(shablon_id));

    //debug << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "  -  xml_dom->dynamicCall(loadXML(QString), xml); \n";
    xml_dom->dynamicCall("loadXML(QString)", xml);
    //debug << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "  -  xml_dom->dynamicCall() - OK \n";

    QString dir_s = settings.value("main/data_folder", "c:").toString();

    debug << "@@ " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "comX->dynamicCall(SendData(QVariant), xml_dom->asVariant());";
    debug << "\n";  debug.flush();

    comX->dynamicCall("SendData(QVariant)", xml_dom->asVariant());

    debug << "OK " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "comX->dynamicCall(SendData(QVariant), xml_dom->asVariant());";
    debug << "\n";  debug.flush();

   /* debug << "-" << "save: " << " " << */
    xml_dom->dynamicCall("save(QString)", (dir_s+"/AddStaff.xml"));
    comX->dynamicCall("GetErrorDescription(QVariant)", xml_dom_err->asVariant());
   /* debug << "-" << "save: " << " " << */
    xml_dom_err->dynamicCall("save(QString)", (dir_s+"/AddStaff_error.xml"));
    //debug << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n";  debug.flush();

    debug << "OK " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "sdk_manager::addStaff()";
    debug << "\n";  debug.flush();
}

void sdk_manager::addStaffMass(int count, QString lastName, QString middleName, int tabelId_base, int subdiv_id, qint64 cardNum_base, int shablon_id, QDate dBegin, QDate dEnd)
{
    debug << "@@ " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "sdk_manager::addStaffMass()";
    debug << "\n";  debug.flush();

    QString xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>"
                  " <documentrequest type=\"staff\">"
                  "  <login loginname=\"login name\">"
                  "   <workmode mode=\"append\">"
                  "    #LINE#"
                  "   </workmode>"
                  "  </login>"
                  " </documentrequest>'";

    int tabelId = tabelId_base;
    int cardNum = cardNum_base;

    for (int i=0; i<count; i++) {
        tabelId++;
        cardNum++;
        debug << "-" << "new tabelId" << " " << tabelId;
        debug << "-" << "new cardNum" << " " << cardNum;
        debug << "\n";  debug.flush();
        QString line = "<staff id_external=\"\" last_name=\"#LAST#\" first_name=\"FIRST##\" middle_name=\"#MIDDLE#\""
                       "       tabel_id=\"#TABEL#\" subdiv_id_external=\"\""
                       "       appoint_id_external=\"\" graph_id_external=\"\" subdiv_id_internal=\"#SUBDIV#\""
                       "       appoint_id_internal=\"1\" graph_id_internal=\"0\" date_begin=\"#DBEGIN#\""
                       "       photo=\"#PHOTO#\">"
                       " <identifiers>"
                       "  <identifier type_identifier=\"number\" identifier=\"#CARDNUM#\""
                       "              date_begin_action=\"#DBEGIN\" date_end_action=\"#DEND#\" prohibit=\"#PROHIBIT#\""
                       "              shablon_id_external=\"\" shablon_id_internal=\"#SHABLON#\" />"
                       "  </identifiers>"
                       " </staff>"
                       " #LINE#";
        line.replace("#LAST#",    lastName)
            .replace("#FIRST#",   QString::number(tabelId))
            .replace("#MIDDLE#",  middleName)
            .replace("#TABEL#",   QString::number(tabelId))
            .replace("#SUBDIV#",  QString::number(subdiv_id))
            .replace("#DBEGIN#",  dBegin.toString("dd.MM.yyyy"))
            .replace("#DEND#",    dEnd.toString("dd.MM.yyyy"))
            .replace("#CARDNUM#", QString::number(saveCardNum(cardNum)))
            .replace("#SHABLON#", QString::number(shablon_id))
            .replace("#PROHIBIT#","false")
            .replace("#PHOTO#",   "");

        xml.replace("#LINE#", line);
    }
    xml.replace("#LINE#", "");
    xml_dom->dynamicCall("loadXML(QString)", xml);

    QString dir_s = settings.value("main/data_folder", "c:").toString();

    debug << "@@ " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "comX->dynamicCall(SendData(QVariant), xml_dom->asVariant());";
    debug << "\n";  debug.flush();

    comX->dynamicCall("SendData(QVariant)", xml_dom->asVariant());

    debug << "OK " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "comX->dynamicCall(SendData(QVariant), xml_dom->asVariant());";
    debug << "\n";  debug.flush();

   /* debug << "-" << "save: " << " " << */ xml_dom->dynamicCall("save(QString)", (dir_s+"/AddStaff.xml"));
    comX->dynamicCall("GetErrorDescription(QVariant)", xml_dom_err->asVariant());
   /* debug << "-" << "save: " << " " << */ xml_dom_err->dynamicCall("save(QString)", (dir_s+"/AddStaff_error.xml"));
    debug << "\n";  debug.flush();

    debug << "OK " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "sdk_manager::addStaffMass()";
    debug << "\n";  debug.flush();
}


void sdk_manager::passCard(int StaffID, qint64 cardNum, int CardID)
{
    debug << "@@ " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "sdk_manager::passCard()";
    debug << "\n";  debug.flush();

    QString xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>"
                  " <documentrequest type=\"sendcards\" mode=\"resolve_selected_card\"> "
                  "  <login loginname=\"login name\"> "
                  "   <employ id_external=\"\" "
                  "           id_internal=\"#STAFF_ID#\"> "
                  "    <identifier type_identifier=\"family_number\" identifier=\"#CARDNUM#\" id_card=\"#CARD_ID#\" /> "
                  "   </employ> "
                  "  </login> "
                  " </documentrequest> ";
    xml.replace("#STAFF_ID#", QString::number(StaffID))
       .replace("#CARDNUM#",  QString::number(saveCardNum(cardNum)))
       .replace("#CARD_ID#",  QString::number(CardID));
    xml_dom->dynamicCall("loadXML(QString)", xml);
//    debug << "-" << xml;

    QString dir_s = settings.value("main/data_folder", "c:").toString();
//    debug << "-" << dir_s;

    debug << "@@ " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "comX->dynamicCall(ExecuteAccessCardsAction(QVariant), xml_dom->asVariant());";
    debug << "\n";  debug.flush();

    comX->dynamicCall("ExecuteAccessCardsAction(QVariant)", xml_dom->asVariant());

    debug << "OK " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "comX->dynamicCall(ExecuteAccessCardsAction(QVariant), xml_dom->asVariant());";
    debug << "\n";  debug.flush();

   /* debug << "-" << "save: " << " " << */ xml_dom->dynamicCall("save(QString)", (dir_s+"/PassCardsAction.xml"));
//    comX->dynamicCall("GetErrorDescription(QVariant)", xml_dom_err->asVariant());
//   /* debug << "-" << "save: " << " " << */ xml_dom_err->dynamicCall("save(QString)", (dir_s+"/PassCardsAction_error.xml"));
    debug << "\n";  debug.flush();

    debug << "OK " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "sdk_manager::passCard()";
    debug << "\n";  debug.flush();
}

void sdk_manager::denyCard(int StaffID, qint64 cardNum, int CardID)
{
    debug << "@@ " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "sdk_manager::denyCard()";
    debug << "\n";  debug.flush();

    QString xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>"
                  " <documentrequest type=\"sendcards\""
                  "                  mode=\"prohibit_selected_card\">"
                  "  <login loginname=\"login name\">"
                  "   <employ id_external=\"\""
                  "           id_internal=\"#STAFF_ID#\">"
                  "    <identifier type_identifier=\"family_number\""
                  "                identifier=\"#CARDNUM#\""
                  "                id_card=\"#CARD_ID#\" />"
                  "   </employ>"
                  "  </login>"
                  " </documentrequest>";
    xml.replace("#STAFF_ID#", QString::number(StaffID))
       .replace("#CARDNUM#",  QString::number(saveCardNum(cardNum)))
       .replace("#CARD_ID#",  QString::number(CardID));
    xml_dom->dynamicCall("loadXML(QString)", xml);

    QString dir_s = settings.value("main/data_folder", "c:").toString();

    debug << "@@ " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "comX->dynamicCall(ExecuteAccessCardsAction(QVariant), xml_dom->asVariant());";
    debug << "\n";  debug.flush();

    comX->dynamicCall("ExecuteAccessCardsAction(QVariant)", xml_dom->asVariant());

    debug << "OK " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "comX->dynamicCall(ExecuteAccessCardsAction(QVariant), xml_dom->asVariant());";
    debug << "\n";  debug.flush();

   /* debug << "-" << "save: " << " " << */ xml_dom->dynamicCall("save(QString)", (dir_s+"/ExecuteAccessCardsAction.xml"));
//    comX->dynamicCall("GetErrorDescription(QVariant)", xml_dom_err->asVariant());
//   /* debug << "-" << "save: " << " " << */ xml_dom_err->dynamicCall("save(QString)", (dir_s+"/ExecuteAccessCardsAction_error.xml"));
    debug << "\n";  debug.flush();

    debug << "OK " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "sdk_manager::denyCard()";
    debug << "\n";  debug.flush();
}

void sdk_manager::denyAllCards(QString reason)
{
    debug << "@@ " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "sdk_manager::denyAllCards()";
    debug << "\n";  debug.flush();

    QString xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>"
                  " <documentrequest type=\"sendcards\" mode=\"prohibit_card_selected_employs\">"
                  "  <login loginname=\"login name\">"
                  "   <employs>"
                  "    <employ id_external=\"\""
                  "            id_internal=\"#ID#\""
                  "            reason=\"#REASON#\">"
                  "     <identifier type_identifier=\"family_number\""
                  "                 identifier=\"\""
                  "                 id_card=\"\" />"
                  "    </employ>"
                  "   </employs>"
                  "  </login>"
                  " </documentrequest>";
    xml.replace("#REASON#", reason);
    xml_dom->dynamicCall("loadXML(QString)", xml);

    QString dir_s = settings.value("main/data_folder", "c:").toString();
    comX->dynamicCall("ExecuteAccessCardsAction(QVariant)", xml_dom->asVariant());
   /* debug << "-" << "save: " << " " << */ xml_dom->dynamicCall("save(QString)", (dir_s+"/ExecuteAccessCardsAction.xml"));
    comX->dynamicCall("GetErrorDescription(QVariant)", xml_dom_err->asVariant());
   /* debug << "-" << "save: " << " " << */ xml_dom_err->dynamicCall("save(QString)", (dir_s+"/ExecuteAccessCardsAction_error.xml"));
    debug << "\n";  debug.flush();

    debug << "OK " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "sdk_manager::denyAllCards()";
    debug << "\n";  debug.flush();
}

void sdk_manager::saveAllCards()
{
    debug << "@@ " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "sdk_manager::saveAllCards()";
    debug << "\n";  debug.flush();

    QString xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>"
                  " <documentrequest type=\"sendcards\" mode=\"all_employs\">"
                  "  <login loginname=\"login name\" />"
                  " </documentrequest>";
    xml_dom->dynamicCall("loadXML(QString)", xml);

    QString dir_s = settings.value("main/data_folder", "c:").toString();
    debug << dir_s+"/FetchCardsAction.xml\n";

    debug << "@@ " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "comX->dynamicCall(ExecuteAccessCardsAction(QVariant), xml_dom->asVariant());";
    debug << "\n";  debug.flush();

    comX->dynamicCall("ExecuteAccessCardsAction(QVariant)", xml_dom->asVariant());

    debug << "OK " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "comX->dynamicCall(ExecuteAccessCardsAction(QVariant), xml_dom->asVariant());";
    debug << "\n";  debug.flush();

    xml_dom->dynamicCall("save(QString)", (dir_s+"/FetchCardsAction.xml"));
//    comX->dynamicCall("GetErrorDescription(QVariant)", xml_dom_err->asVariant());
//    xml_dom_err->dynamicCall("save(QString)", (dir_s+"/FetchCardsAction_error.xml"));
    debug << "\n";  debug.flush();

    debug << "OK " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "sdk_manager::saveAllCards()";
    debug << "\n";  debug.flush();
}


QString sdk_manager::getEvents(QDate dateFrom, QDate dateTo) {
    debug << "@@ " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "sdk_manager::getEvents()";
    debug << "\n";  debug.flush();

    while (true) {
        try {
            QString xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>"
                          "    <documentrequest type=\"events\" typeemploys=\"staff\">"
                          "    <typereport beginperiod=\"#DBEGIN#\""
                          "                endperiod=\"#DEND#\""
                          "                id_subdiv_external=\"\""
                          "                id_subdiv_internal=\"1\""
                          "                typehierarchy=\"hierarchy\""
                          "                order=\"subdiv;fio;appoint;tab_no\""
                          "                get_all_events=\"true\""
                          "                type_identifier=\"family_number\" />"
                          "        <eventsreport />"
                          "    </documentrequest>";
            xml.replace("#DBEGIN#", dateFrom.toString("dd.MM.yyyy"))
               .replace("#DEND#",   dateTo.addDays(1).toString("dd.MM.yyyy"));
            xml_dom->dynamicCall("loadXML(QString)", xml);

            debug << "@@ " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "comX->dynamicCall(GetEvents(QVariant), xml_dom->asVariant());";
            debug << "\n";  debug.flush();

            QString dir_s = settings.value("main/data_folder", "c:").toString();
            comX->dynamicCall("GetEvents(QVariant)", xml_dom->asVariant());
           /* debug << "-" << "save: " << " " << */ xml_dom->dynamicCall("save(QString)", (dir_s+"/events.xml"));
//            comX->dynamicCall("GetErrorDescription(QVariant)", xml_dom_err->asVariant());
//         /* debug << "-" << "save: " << " " << */ xml_dom_err->dynamicCall("save(QString)", (dir_s+"/events_errors.xml"));
//            debug << "\n";  debug.flush();

            debug << "OK " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "comX->dynamicCall(GetEvents(QVariant), xml_dom->asVariant());";
            debug << "\n";  debug.flush();

            debug << "OK " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "sdk_manager::getEvents()";
            debug << "\n";  debug.flush();

            return (dir_s+"/events.xml");
        } catch (...) {
            debug << "-" << "не смог получить список событий с турникета. Жду 5 сек.";
            debug << "\n";  debug.flush();
            Sleep(5000);
        }
    }

    debug << "OK " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "sdk_manager::getEvents()";
    debug << "\n";  debug.flush();

    return (QString(".\\events.xml"));
}

qint64 sdk_manager::saveCardNum(qint64 cardNum)
{
    debug << "@@ " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "sdk_manager::saveCardNum()";
    debug << "\n";  debug.flush();

    bool ch_SaveHex = settings.value("turnside/save_Hex", "1").toString().toInt()>0;

    if (ch_SaveHex) {
        // перед сохранением 10-чное число интерпретируется словно 16-ричное
        bool ok = true;
        qint64 cardNum_cor = QString::number(cardNum, 10).toInt(&ok, 16);
        if (ok)
            return cardNum_cor;
    }

    debug << "OK " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "sdk_manager::saveCardNum()";
    debug << "\n";  debug.flush();

    return cardNum;
}

qint64 sdk_manager::readCardNum(qint64 cardNum_сor)
{
    debug << "@@ " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "sdk_manager::readCardNum()";
    debug << "\n";  debug.flush();

    bool ch_SaveHex = settings.value("turnside/save_Hex", "1").toString().toInt()>0;

    if (ch_SaveHex) {
        // перед прочтением 16-ричное число интерпретируется словно 10-чное
        bool ok = true;
        qint64 cardNum = QString::number(cardNum, 16).toInt(&ok, 10);
        if (ok)
            return cardNum;
    }

    debug << "OK " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "   -   " << "sdk_manager::readCardNum()";
    debug << "\n";  debug.flush();

    return cardNum_сor;
}
