requires(qtHaveModule(httpserver))

TEMPLATE = app

QT = httpserver
QT += webchannel websockets


CONFIG(debug, debug|release) {
	TEMPDIR    = $$PWD/../build/debug
} else {
	TEMPDIR    = $$PWD/../build/release
}

MOC_DIR         = $$TEMPDIR/moc
OBJECTS_DIR     = $$TEMPDIR/obj
RCC_DIR         = $$TEMPDIR/rcc
DESTDIR			= $$TEMPDIR/bin

SOURCES += \
    backend.cpp \
    main.cpp


include($$PWD/websockets/websockets.pri)

RESOURCES += \
    assets.qrc


target.path = $$[QT_INSTALL_EXAMPLES]/httpserver/angular
INSTALLS += target

CONFIG += cmdline

HEADERS += \
	backend.h
