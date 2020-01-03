
/*
https://github.com/nodejitsu

https://github.com/nodejitsu/node-http-proxy/issues/576

https://gist.github.com/richardkazuomiller/9087254

https://www.diycode.cc/projects/nodejitsu/node-http-proxy

*/


var WebSocketServer = require('websocket').server;
var http = require('http');
var port = 82;

var server = http.createServer(function(request, response) {
    // Qui possiamo processare la richiesta HTTP
    // Dal momento che ci interessano solo le WebSocket, non dobbiamo implementare nulla
});



console.log('Websoket Running  on port: ' + port);


server.listen(port, function() { });
// Creazione del server
wsServer = new WebSocketServer({
    httpServer: server
});


// Gestione degli eventi
wsServer.on('request', function(request) {
    var connection = request.accept(null, request.origin);
    connection.on('message', function(message) {
        // Metodo eseguito alla ricezione di un messaggio
        if (message.type === 'utf8') {
            // Se il messaggio è una stringa, possiamo leggerlo come segue:
            console.log('Il messaggio ricevuto è: ' + message.utf8Data);
        }
    });
    connection.on('close', function(connection) {
        // Metodo eseguito alla chiusura della connessione
    });
});




