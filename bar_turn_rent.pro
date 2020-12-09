QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

QT  +=  core axcontainer sql xml network printsupport

CONFIG += axcontainer

QT_DEBUG_PLUGINS=1

#TYPELIBS = C:\libs\perco_s20_sdk.tlb

QMAKE_CXX_FLAGS += /EHa

#LIBS += -lqjson

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
        main.cpp \
    core_object.cpp \
    db_manager.cpp \
    server.cpp \
    sdk_manager.cpp \
    tcpserver.cpp \
    barcodeprinter.cpp \
    json.cpp \
    waiter.cpp \
    db_ticket.cpp

TYPELIBS = $$system(dumpcpp -getfile {E74FA501-350F-43CF-8C15-D831778FD465})
#TYPELIBS = $$system(dumpcpp -getfile {00062FFF-0000-0000-C000-000000000046})

#isEmpty(TYPELIBS) {
#     message("PERCo type library not found!")
#     REQUIRES += PERCo
# } else {
#     HEADERS  += perco.h
#     SOURCES  += perco.cpp
# }


# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    core_object.h \
    db_manager.h \
    server.h \
    db_rec.h \
    sdk_manager.h \
    tcpserver.h \
    barcodeprinter.h \
    json.h \
    waiter.h \
    db_ticket.h

DISTFILES += \
    bar_turn_rent.ini \
    fonts/CarolinaBarUPC_Normal.ttf \
    fonts/EAN-13.ttf \
    start.vbs \
    fonts/code128.ttf
