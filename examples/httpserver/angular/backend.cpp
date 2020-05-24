#include "backend.h"

Backend::Backend(QWebSocketServer *server): QObject(server),
	m_clientWrapper(server)
{
	m_userName = "Valerie Luna";
	m_items = QStringList() << "Milk" << "Bread" << "Cheese" << "Beer";
	//register self in javascript as "backend" object
	m_webChannel.registerObject(QStringLiteral("backend"), this);
	// setup web channel
	connect(&m_clientWrapper, &WebSocketClientWrapper::clientConnected,  &m_webChannel, &QWebChannel::connectTo);
}

void Backend::setUserName(const QString &userName)
{
	m_userName = userName;
	emit userNameChanged(userName);
}

void Backend::addItem(const QString &item)
{
	m_items.append(item);
	emit itemAdded(item);
}

void Backend::removeItem(const QString &item)
{
	m_items.removeAll(item);
	emit itemRemoved(item);
}
