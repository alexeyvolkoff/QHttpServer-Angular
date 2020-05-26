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
Most of complexities are hidden under the hood. While QHttpServer serves plain HTML pages, WebSocketClientWrapper connects WebSocket used by JavaScript to C++ class Backend (QObject descendant) running in server. Both communicate with each other in their native way: JavaScript sees Backend as a JavaScript object, invokes its methods, gets/sets properies, subscribes to events (signals). Backend, on the other hand, behaves as an ordinary QObject, client-agnostically:  it just provides slots and emits signals. Event handlers are getting fired in JavaSript without extra coding.  Angular (formely AngularJS) framework allows you to display your backend's data in DOM elements as well as modify QObject's exposed properties right away in **ng-model** directive. 

### 1. Serving plain html pages
We serve static html pages from the examples/angular/assets directory:
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
It is also possible to provide the files right from the application's resource by prepanding the relative path extracted from the URL with ":/" prefix:
```c++
httpServer.route("/<arg>", [assetsRootDir] (const QUrl &url) {
    return QHttpServerResponse::fromFile(QStringLiteral(":/assets/%1").arg(url.path()));
});
```
Do not forget to add all required HTML/css/js files to your project's *.qrc* resource list.

### 2. Angulariziation of html template
For this example we took [SB Admin 2](https://startbootstrap.com/template-overviews/sb-admin-2/) - a nice looking html template based on Bootstrap markup. We have sligtly modified *index.html* by including Angular engine and Qt's qwebchannel.js script. We also have added script section for our AngularJS app: 

```html
<head>
 ...
  <!-- AngularJS scripts -->
  <script src="https://ajax.googleapis.com/ajax/libs/angularjs/1.6.9/angular.min.js"></script>
  <!-- qwebchannel -->
  <script src="qwebchannel/qwebchannel.js"></script>
</head>

<!-- alexey: AngularJS app -->
<script>
	var app = angular.module("qtAngularDemo", []);
	app.controller("qtCtrl", function($scope) {
		$scope.products = [];
		...
	});
</script>	

<body id="page-top" ng-app="qtAngularDemo" ng-controller="qtCtrl">	
```
Referencing app and controller in HTML body directives ng-app="qtAngularDemo" and ng-controller="qtCtrl" will allow us to use $scope variables and expressions (like {{products.length}}) wherever we want them to appear in DOM. Angular will refresh the element whenever the referenced variable changes its value.

### 3. Backend class
The Backend class is pretty much self-explaining. For our example, we expose *userName* and *items* properties.  The only thing to mention in regard of read/write properties is mandatory NOTIFY member in Q_PROPERTY definition. Public slots and signals are exposed as JavaScript object's methods and events.
```c++
class Backend: public QObject
{
	Q_OBJECT
	/* WebSocket communication  */	
	QWebChannel m_webChannel;
	WebSocketClientWrapper m_clientWrapper;
	/* Data */
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

### 4. Connecting to Backend and getting data
When WebSocket is connected to QWebSocketServer, we request the backend object and make it global across our Angular $scope:
```javascript
	/* WebSocket communication */
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

### 5. Invoking mothods
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

	/* remove item request */
	$scope.removeItem = function (idx) {
		$scope.backend.removeItem($scope.products[idx]);
	};
```

### 6. Handling events
JavaScript event handlers are connected to the remote object's slots in similar way the lambda-style C++ handlers are connected to QObject by *connect()* method:  
```javascript
	/* item added on backend */
	$scope.backend.itemAdded.connect(function(item) {
		console.log('Item added: ' + item);
		$scope.products.push(item);
		$scope.notify('info', 'Item added: ' + item);
		$scope.$apply();
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
Thus, when the data is updated on the backend side, $scope variables get updated too, and Angular framework will do the rest to keep DOM elements in sync. By simply emitting C++ signals, you update your dynamic web UI with no extra effort. Cool, even for native deskop frameworks.

### 7. Displaying data
Angular's **ng-repeat** is a powerful tool to display the list of items in repeatable styled element:
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
The directive ng-click="removeItem($index)" links the displayed item with the remove handler.
User name property is displayed in the top menu as {{backend.userName}}.

### 8. Editing data
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
In our example though, we edit a temporary $scope variable *newName*, and assign it to *backend.userName* when the user confirms the modification in the Profile dialog:

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
If you replace ng-model="newName" with ng-model="backend.userName", the user name will be getting updated in the top menu (and on the backend) right away while the user types in the Profile dialog. If you are OK with it, then the temporary *newName* variable is not needed.

# Conclusion
I believe the discussed approach can be concedered an interesting alternative to classical AJAX interactions and worth trying, especially when it comes to reducing the coding complexity and effort. Besides, it is more *data-centric*, which allows you to separate the application layers properly and focus on data flow and business logic on each of them instead of coding and debugging the communication. Hope it helps someone.
