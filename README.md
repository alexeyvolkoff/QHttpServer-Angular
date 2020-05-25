# AngularJS Demo App for QHttpServer
The purpose of this app is to demonstrate how to run modern frontend atop of C++/Qt backend making advantage of Qt's signals/slots facility for implementing the frontend-to-backend communication.

## Components
- [SB Admin 2](https://startbootstrap.com/template-overviews/sb-admin-2/) - free Bootstrap 4 admin theme;
- [QHttpServer](https://github.com/qt-labs/qthttpserver) - Qt-native http server by Qt Labs;
- [WebSocket Client Wrapper](https://github.com/ynonp/qt-webchannel-demo/blob/master/shared/websocketclientwrapper.cpp) - WebSocket transport implementation for QWebChannel by KDAB Group company;
- [Angular framework](https://angular.io/).

## Preview
![QHttpServer-Angular Preview](https://github.com/alexeyvolkoff/QHttpServer-Angular/raw/master/screenshot.png)

## Building and running
- Open qhttpserver.pro;
- Build and run **examples/angular** sub-project;
- Navigate to http://127.0.0.1:8000/

You can add/remove items to the Shopping list and modify your user name in the Profile dialog. As user adds/removes data or changes the name, notifications from backend get displayed in the Notifications widget. 

# How it works
Most of complexities are hidden under the hood. While QHttpServer serves plain HTML pages, WebSocketClientWrapper connects WebSocket used by JavaScript to C++ class Backend (QObject descendant) running in server. Both communicate with each other in their native way: JavaScript sees Backend as a JavaScript object, invokes its methods, gets/sets properies, subscribes to events (signals). Backend, on the other hand, behaves as a normal QObject, client-agnostically:  it just provides slots and emits signals as usual, event handlers are getting fired in JavaSript without extra coding.  Angular (formely AngularJS) framework allows you to display your backend's data in DOM elements as well as edit QObject's properties right away in **ng-model** directive, and yes, with no coding. 

### 1. Serving static html
We serve html pages from examples/angular/assets:
```c++
QDir assetsDir = QDir(QCoreApplication::applicationDirPath() + "../../../../angular/assets");
const QString assetsRootDir = assetsDir.absolutePath();

httpServer.route("/", [assetsRootDir]() {
    return QHttpServerResponse::fromFile(assetsRootDir + QStringLiteral("/index.html"));
});

httpServer.route("/<arg>", [assetsRootDir] (const QUrl &url) {
    return QHttpServerResponse::fromFile(assetsRootDir + QStringLiteral("/%1").arg(url.path()));
});
```
It is also possible to provide the files right from the application's resource by prepanding the relative path extracted from url with ":/" prefix:
```c++
httpServer.route("/<arg>", [assetsRootDir] (const QUrl &url) {
    return QHttpServerResponse::fromFile(QStringLiteral(":/assets/%1").arg(url.path()));
});
```
Do not forget to add your files to project's *.qrc* resource list.

### 2. Backend class
The Backend class is much self-explaining. For our example, we expose userName and items properties.  The only thing to mention in regard of read/write properies is mandatory NOTIFY member in Q_PROPERTY definition. Public slots and signals are also exposed.
```c++
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

	/* JavaScript interface - properties */
	Q_PROPERTY(QStringList items READ getItems CONSTANT)
	Q_PROPERTY(QString userName READ getUserName WRITE setUserName NOTIFY userNameChanged)

	/* JavaScript interface - methods */
public slots:
	void addItem(const QString& item);
	void removeItem(const QString& item);

	/* JavaScript interface - events */
signals:
    void itemAdded(const QString& item);
    void itemRemoved(const QString& item);
    void userNameChanged(const QString& userName);
};
```
This is how the Backend is created and exposed to JavaScript:
```c++
/* Websocket server for communication */
QWebSocketServer wsServer("Angular Demo",  QWebSocketServer::NonSecureMode);
/* Backend for QWebSocketServer */
Backend backend(&wsServer);
...
Backend::Backend(QWebSocketServer *server): QObject(server),
    m_clientWrapper(server)
{
	m_userName = "Valerie Luna";
	m_items = QStringList() << "Milk" << "Bread" << "Cheese" << "Beer";
	/* Register backend instance in JavaScript as "backend" object */
	m_webChannel.registerObject(QStringLiteral("backend"), this);
	/* Setup Web Channel */
	connect(&m_clientWrapper, &WebSocketClientWrapper::clientConnected,  &m_webChannel, &QWebChannel::connectTo);
}
```
Events get fired from the property setters and public slots:
```c++
void Backend::setUserName(const QString &userName)
{
	m_userName = userName;
  	/* firing JavaScript event */
	emit userNameChanged(userName);
}

void Backend::addItem(const QString &item)
{
	m_items.append(item);
  	/* firing JavaScript event */
	emit itemAdded(item);
}

void Backend::removeItem(const QString &item)
{
	m_items.removeAll(item);
  	/* firing JavaScript event */
	emit itemRemoved(item);
}

```
 
### 3. Connecting to Backend and getting data
As soon as websocket is connected to QWebSocketServer, we request the backend object and make it global across our Angular $scope:
```javascript
    	/* Websocket communication */
	$scope.wsUrl = (window.location.protocol === 'https:' ? 'wss://' : 'ws://') +  window.location.hostname + ':8001';
		$scope.notificationSocket = new WebSocket($scope.wsUrl);

		if ($scope.notificationSocket)
			$scope.notificationSocket.onopen = function() {
				new QWebChannel($scope.notificationSocket, function(channel) {
					console.log('web channel connected');
					/* make backend object accessible globally in scope */
					$scope.backend = channel.objects.backend;

					/* get item list from backend */
					$scope.products = $scope.backend.items;
		};
```
We also get backend's items property to display it as list with **ng-repeat** directive. 

### 4. Invoking mothods
C++ methods declared as **public slots** are becoming JavaScript object's methods that can be invoked transparently:
```javascript
		/* add new item request  */
		$scope.addItem = function () {

			if ($scope.products.indexOf($scope.addMe) == -1) {
				$scope.backend.addItem($scope.addMe);
			} else {
				$scope.notify ('warning', $scope.addMe + ' - already added');
			}

			$scope.addMe = '';
		};

		/* remove item request from frontend */
		$scope.removeItem = function (idx) {
			$scope.backend.removeItem($scope.products[idx]);
		};
```

### 4. Handling events
JavaScript event handlers are connected to the object's slots in similar way as *lambda-style* *QObject::connect()* does:  
```javascript
	/* item added on backend */
	$scope.backend.itemAdded.connect(function(item) {
		console.log('Item added: ' + item);
		$scope.products.push(item);
		$scope.notify('info', 'Item added: ' + item);
		scope.$apply();
	});

	/* item removed on backend */
	$scope.backend.itemRemoved.connect(function(item) {
	var idx = $scope.products.indexOf(item);
		if (idx !== -1) {
			console.log('Item removed: ' + item);
			$scope.products.splice(idx, 1);
			$scope.notify('info', 'Item removed: ' + item);
			$scope.$apply();
		}
	});

	/* user renamed on backend */
	$scope.backend.userNameChanged.connect(function(newUserName) {
		console.log('User renamed: ' + newUserName);
		$scope.notify('info', 'User renamed: ' + newUserName);
		$scope.$apply();
	});

```
Thus, we update $scope as soon as data is updated on the backend side.

### 5. Displaying data
Angular's **ng-repeat** is a powerful tool to display the list of data in repeatable element:
```html
<div class="card-body">
	<div class="flex-row">
		<div class="mb-4 small d-flex flex-row justify-content-between" ng-repeat="item in products">
				<span class="mr-2 pull-left"><i class="fas fa-check text-primary"></i> {{item}}</span>
				<span class="mr-2 pull-right" style="cursor: pointer;" ng-click="removeItem($index)"><i class="fas fa-times-circle text-secondary"></i></span>
		</div>
		<div class="mb-4 small"></div>
			<div class="input-group">
				<input type="text" class="form-control bg-light border-0 small" ng-model="addMe" placeholder="Add item" aria-label="Add" aria-describedby="basic-addon2">
				<div class="input-group-append">
				<button class="btn btn-primary" type="button" ng-click="addItem()">
					<i class="fas fa-plus fa-sm"></i>
				</button>
			</div>
		</div>
	</div>
</div>
```
User name property is displayed in top menu in Angular markup as {{backend.userName}}.

### 5. Editing data
Remote object's properties can be edited with Angular's **ng-model** directive as usual $sope variables. 
```html
  <!-- Profile modal -->
  <div class="modal animated fade" id="profile">
	<div class="modal-dialog">
	  <div class="modal-content">
		<form ng-submit="updateProfile()">
			<div class="modal-header">
				<h5 class="modal-title" id="exampleModalLabel">Profile - {{backend.userName}}</h5>
					<button class="close" type="button" data-dismiss="modal" aria-label="Close">
					 <span aria-hidden="true">×</span>
					</button>
			</div>
			<div class="modal-body">
			  <label class="radio">User name:</label>
			  <input class="form-control" ng-model="newName" autofocus="autofocus">
			</div>
			<div class="modal-footer">
			  <button type="button" class="btn btn-secondary" data-dismiss="modal">Cancel</button>
			  <button type="submit" class="btn btn-primary">OK</button>
			</div>
		</form>
	  </div>
	</div>
  </div>
```
In our example though, we edit a temporary $scope variable newName, and assign it to Backend's userName when the user confirms modification in the Profile dialog:

```javascript
	/* show profile dialog */
	$scope.showProfileDialog = function() {
			$scope.newName = $scope.backend.userName;
			$( "#profile" ).modal('show');
	};

	/* change user name on backend */
	$scope.updateProfile = function(){
			$scope.backend.userName = $scope.newName;
			$( "#profile" ).modal('hide');
	}
```
# Conclusion
We beleave that WebSockets in combination with Qt's signals/slots facility can be concedered an interesting alternative to classical approach like AJAX, especially when it comes to reducing the coding effort. Hope it helps someone.
