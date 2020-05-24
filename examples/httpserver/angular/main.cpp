/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtHttpServer module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore>
#include <QtHttpServer>
#include <QWebSocketServer>
#include <QDir>
#include "backend.h"


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    /* HTTP server for providing content */
    QHttpServer httpServer;
    /* Websocket server for communication */
    QWebSocketServer wsServer("Angular Demo",  QWebSocketServer::NonSecureMode);
    /* Backend for Websocket server */
    Backend backend(&wsServer);


    if (! wsServer.listen(QHostAddress::AnyIPv4, 8001))
    {
	qWarning() << QCoreApplication::translate(
		"QHttpServerExample", "Failed to start WebSocket server on port 8001");
	return 0;
    }

    qInfo() << QCoreApplication::translate(
		"QHttpServerExample", "Starting WebSocket notifications server on ws://127.0.0.1:%1/ ").arg(wsServer.serverPort());

    QDir assetsDir = QDir(QCoreApplication::applicationDirPath() + "../../../../angular/assets");
    const QString assetsRootDir = assetsDir.absolutePath();


    httpServer.route("/", [assetsRootDir]() {
	return QHttpServerResponse::fromFile(assetsRootDir + QStringLiteral("/index.html"));
    });

    httpServer.route("/<arg>", [assetsRootDir] (const QUrl &url) {
	return QHttpServerResponse::fromFile(assetsRootDir + QStringLiteral("/%1").arg(url.path()));
    });


    const auto port = httpServer.listen(QHostAddress::Any, 8000);
    if (!port) {
        qDebug() << QCoreApplication::translate(
		"QHttpServerExample", "Server failed to listen on a port 8000");
        return 0;
    }

    qDebug() << QCoreApplication::translate(
	    "QHttpServerExample", "Running WebServer on http://127.0.0.1:%1/ (Press CTRL+C to quit)").arg(port);

    return app.exec();
}
