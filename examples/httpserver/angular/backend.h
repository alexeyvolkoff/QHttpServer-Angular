#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include <QWebChannel>
#include <QWebSocketServer>

#include "websocketclientwrapper.h"
#include "websockettransport.h"

class Backend: public QObject
{
	Q_OBJECT
	QWebChannel m_webChannel;
	WebSocketClientWrapper m_clientWrapper;
	QString m_userName;
	QStringList m_items;
public:
	Backend(QWebSocketServer *server);
	void setUserName(const QString& userName);
	QString getUserName() { return m_userName; }
	QStringList getItems() { return  m_items; }

	/* Javascript interface - properties */
	Q_PROPERTY(QStringList items READ getItems CONSTANT)
	Q_PROPERTY(QString userName READ getUserName WRITE setUserName NOTIFY userNameChanged)

	/* Javascript interface - methods */
public slots:
	void addItem(const QString& item);
	void removeItem(const QString& item);

	/* Javascript interface - events */
signals:
    void itemAdded(const QString& item);
    void itemRemoved(const QString& item);
    void userNameChanged(const QString& userName);
};

#endif // BACKEND_H
