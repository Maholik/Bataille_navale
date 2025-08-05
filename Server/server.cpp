#include "server.h"
#include "game.h"
#include <QTcpSocket>
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>

Server::Server(QObject *parent) :
    QObject(parent),
    server(new QTcpServer(this)),
    clientIdCounter(1) // Initialiser le compteur pour les ID des clients
{
    connect(server, &QTcpServer::newConnection, this, &Server::onNewConnection);
}

Server::~Server()
{
    server->close();
}

void Server::startServer()
{
    if (!server->listen(QHostAddress::Any, 12344)) {
        qCritical() << "Le serveur n'a pas pu démarrer.";
    } else {
        qDebug() << "Serveur démarré sur le port" << server->serverPort();
    }
}

void Server::stopServer()
{
    for (QTcpSocket *client : qAsConst(clients)) {
        client->disconnectFromHost();
    }
    server->close();
    qDebug() << "Serveur arrêté";
}

void Server::createRoom(const QString& id_room, const QString& name, const QString& type, const QString& password)
{
    Room room;
    room.id = id_room;
    room.name = name;
    room.type = type;
    room.password = password;
    room.clientCount = 0;

    rooms.append(room);
    sendRoomInfoToClients();
}

void Server::sendRoomInfoToClients()
{
    for (QTcpSocket *client : qAsConst(clients)) {
        QString roomInfo = "ROOM_LIST:";
        for (const Room &room : qAsConst(rooms)) {
            roomInfo += QString("%1;%2;%3;%4\n")
            .arg(room.id)
                .arg(room.name)
                .arg(room.type)
                .arg(room.clientCount);
        }

        client->write(roomInfo.toUtf8());
        qDebug() << "Liste des rooms envoyée : " << roomInfo;
    }
}

void Server::sendBoardsUpdateToClients(QString& _roomId){
    Room* roomId = findRoomById(_roomId);
    for (QTcpSocket *client : roomId->clients) {
        QString roomInfo = "BOARD_CREATE;";
        Game* gamemodel = this->roomGameMap[_roomId];
        gamemodel->displayBoards();
        roomInfo += QString::number(gamemodel->getPlayer1()->getBoard()->getRows()) + ";";
        roomInfo += QString::number(gamemodel->getPlayer1()->getBoard()->getCols()) + ";";
        roomInfo += QString::fromStdString(gamemodel->getPlayer1()->getName()) + ";";
        //Creation du plateau
        for (int row = 0; row < gamemodel->getPlayer1()->getBoard()->getRows(); ++row) {
            for (int col = 0; col < gamemodel->getPlayer1()->getBoard()->getCols(); ++col) {
                QString _caseString;
                Case* _case = gamemodel->getPlayer1()->getBoard()->getCase(row,col);
                switch (_case->getStatus()) {
                case Case::Empty: _caseString = "E"; break;    // Case vide
                case Case::Hit: _caseString = "H"; break;      // Touché
                case Case::Miss: _caseString = "M"; break;     // Manqué
                case Case::Occupied:
                    std::vector<Boat*> allBoats = gamemodel->getPlayer1()->getBoard()->getAllBoats();
                    for (Boat* _element: allBoats) {
                        if (std::find(_element->getStructure().begin(), _element->getStructure().end(), _case) != _element->getStructure().end()) {
                            switch (_element->getSize()) {
                            case 2 : _caseString = "D"; break;
                            case 3 : _caseString = "S"; break;
                            case 4 : _caseString = "C"; break;
                            case 5 : _caseString = "P"; break;
                            }
                        }
                    }
                }
                roomInfo += _caseString + ";";
            }
        }

        roomInfo += QString::fromStdString(gamemodel->getPlayer2()->getName()) + ";";
        //Creation du plateau
        for (int row1 = 0; row1 < gamemodel->getPlayer2()->getBoard()->getRows(); ++row1) {
            for (int col1 = 0; col1 < gamemodel->getPlayer2()->getBoard()->getCols(); ++col1) {
                QString _caseString1;
                Case* _case1 = gamemodel->getPlayer2()->getBoard()->getCase(row1,col1);
                switch (_case1->getStatus()) {
                case Case::Empty: _caseString1 = "E"; break;    // Case vide
                case Case::Hit: _caseString1 = "H"; break;      // Touché
                case Case::Miss: _caseString1 = "M"; break;     // Manqué
                case Case::Occupied:
                    std::vector<Boat*> allBoats1 = gamemodel->getPlayer2()->getBoard()->getAllBoats();
                    for (Boat* _element1: allBoats1) {
                        if (std::find(_element1->getStructure().begin(), _element1->getStructure().end(), _case1) != _element1->getStructure().end()) {
                            switch (_element1->getSize()) {
                            case 2 : _caseString1 = "D"; break;
                            case 3 : _caseString1 = "S"; break;
                            case 4 : _caseString1 = "C"; break;
                            case 5 : _caseString1 = "P"; break;
                            }
                        }
                    }
                }
                roomInfo += _caseString1 + ";";
            }
        }
        roomInfo += "\n";
        qDebug() <<"Je suis a la fin, le roomInfo est: " << roomInfo;
        client->write(roomInfo.toUtf8());
        qDebug() << "Boards sont envoyes aux clients";
    }
}

void Server::sendUpdateCaseToClients(QString& _roomId, int row, int col){
    Room* roomId = findRoomById(_roomId);
    Game* gamemodel = this->roomGameMap[_roomId];
    for (QTcpSocket *client : roomId->clients) {
        QString roomInfo = "UPDATE_CASE;";
        Case* _case = gamemodel->getOppositePlayer()->getBoard()->getCase(row,col);
        roomInfo += QString::fromStdString(gamemodel->getOppositePlayer()->getName()) + ";" + QString::number(row) + ";" + QString::number(col) + ";";
        QString _caseString = "X";
        switch (_case->getStatus()) {
        case Case::Hit: _caseString = "H"; break;      // Touché
        case Case::Miss: _caseString = "M"; break;     // Manqué
        }
        roomInfo += _caseString + ";\n";
        client->write(roomInfo.toUtf8());
        qDebug() << "Update envoyée : " << roomInfo;
    }
    gamemodel->changePlayer();
}

void Server::sendStatusInfoToClients(QString& _roomId){
    Room* roomId = findRoomById(_roomId);
    for (QTcpSocket *_client : qAsConst(roomId->clients)) {
        QString roomInfo = "STATUS;";
        Game* gamemodel = this->roomGameMap[_roomId];
        roomInfo += QString::fromStdString(gamemodel->getCurrentPlayer()->getName()) + ";";
        roomInfo += QString::number(gamemodel->isGameOver()) + ";";
        roomInfo += QString::fromStdString(gamemodel->getWinner());
        roomInfo += "\n";
        _client->write(roomInfo.toUtf8());
        qDebug() << "Status envoyée : " << roomInfo;
    }
}

void Server::sendErrorMessageToClients(QString& _roomId, QString& errorMessage){
    Room* roomId = findRoomById(_roomId);
    for (QTcpSocket *client : qAsConst(roomId->clients)) {
        QString roomInfo = "ERROR;" + errorMessage;
        client->write(roomInfo.toUtf8());
    }
}

void Server::onNewConnection()
{
    QTcpSocket *clientSocket = server->nextPendingConnection();
    clients.append(clientSocket);

    // Attribuer un ID unique au client
    int clientId = clientIdCounter++;
    clientIdMap[clientSocket] = clientId;

    connect(clientSocket, &QTcpSocket::readyRead, this, &Server::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &Server::onDisconnected);

    qDebug() << "Un nouveau client s'est connecté avec l'ID:" << clientId;
}

void Server::onReadyRead()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket *>(sender());
    if (!clientSocket)
        return;

    QByteArray data = clientSocket->readAll();
    QString message = QString::fromUtf8(data);

    qDebug() << "Message reçu du client:" << message;

    if (message.startsWith("GET_ROOMS")) {
        sendRoomInfoToClients();
    }
    else if (message.startsWith("CREATE_ROOM;")) {
        QStringList parts = message.split(";");
        if (parts.size() >= 3) {
            QString roomId = QString::number(rooms.size() + 1);
            QString roomName = parts[1];
            QString roomType = parts[2];
            QString password = (parts.size() > 3) ? parts[3] : "";
            createRoom(roomId, roomName, roomType, password);
        }
    }
    else if (message.startsWith("JOIN_ROOM;")) {
        QStringList parts = message.split(";");
        QString playerName = parts[1];
        int idClient = clientIdMap[clientSocket];
        clientToNameMap[idClient] = playerName;
        QString roomId = parts[2];
        QString password = (parts.size() == 4) ? parts[3] : "";

        if (checkRoomAccess(roomId, password)) {
            clientRoomMap[clientSocket] = roomId;  // Mettez à jour le mapping des rooms
            Room* room = findRoomById(roomId);
            room->clients.append(clientSocket);
            if(room){
                if(room->clientCount == 2){
                    QList<QTcpSocket*> _clientsInRoom = this->clientsInRoom(roomId);

                    Player* player1 = new Player{clientToNameMap[clientIdMap[_clientsInRoom[0]]].toStdString()};
                    Player* player2 = new Player{clientToNameMap[clientIdMap[_clientsInRoom[1]]].toStdString()};

                    // Créez une instance de jeu
                    Game* gamemodel = new Game(player1,player2);
                    this->games.append(gamemodel);
                    this->roomGameMap[roomId] = gamemodel;
                    gamemodel->initializeGame();
                    gamemodel->displayBoards();

                    for (QTcpSocket* client : _clientsInRoom) {
                        client->write("ROOM_READY;" + roomId.toUtf8());
                    }
                }
                else{
                    clientSocket->write("ROOM_JOINED;" + roomId.toUtf8() + "\n");
                    sendRoomInfoToClients();
                }
            }
            qDebug() << "Client a rejoint la salle: " << roomId;
        } else {
            clientSocket->write("ERR_INVALID_PASSWORD_OR_ROOM_NOT_FOUND\n");
        }
    }
    else if(message.startsWith("CREATE_GAME;")){
        QStringList parts = message.split(";");
        QString roomId = parts[1];
        sendBoardsUpdateToClients(roomId);
        sendStatusInfoToClients(roomId);
    }
    else if(message.startsWith("ATTACK")){
        QStringList parts = message.split(";");
        QString roomId = parts[1];
        int row = parts[2].toInt();
        int col = parts[3].toInt();
        Game* gamemodel = roomGameMap[roomId];
        gamemodel->attackPlayer(row,col);
        sendUpdateCaseToClients(roomId,row,col);
        sendStatusInfoToClients(roomId);
    }
    else if(message.startsWith("QUIT_ROOM;")){
        QStringList parts = message.split(";");
        QString roomId = parts[1];
        onRoomDisconnected(roomId);
    }
    else if (message.startsWith("CHAT_MESSAGE;")) {
        QStringList parts = message.split(";");
        QString roomId = parts[1];       // Nom du joueur qui a envoyé le message
        QString chatMessage = parts[2];  // Contenu du message
        broadcastMessageToRoom(roomId,chatMessage);
    }
    else if(message.startsWith("RECONNAISANCE;")) {
        QStringList parts = message.split(";");
        QString roomId = parts[1];       // Nom du joueur qui a envoyé le message
        int row = parts[2].toInt();  // Contenu du message
        int col = parts[3].toInt();
        Game* gamemodel = roomGameMap[roomId];
        int nbBateauInZone = gamemodel->reconnaissanceZone(row,col);
        QString message = "RECONNAISANCE_RESULT;" + QString::number(nbBateauInZone) + "\n";
        clientSocket->write(message.toUtf8());
    }
}

Server::Room* Server::findRoomById(const QString& id_room)
{
    for (Room& room : rooms) {
        if (room.id == id_room) {
            return &room;
        }
    }
    return nullptr;
}

bool Server::checkRoomAccess(const QString &id_room, const QString &password)
{
    Room* room = findRoomById(id_room);
    if (!room) {
        qDebug() << "Room introuvable : " << id_room;
        return false;
    }

    if (room->type == "privée" && room->password != password) {
        qDebug() << "Mot de passe incorrect pour la salle : " << id_room;
        return false;
    }

    room->clientCount++;
    return true;
}



void Server::onRoomDisconnected(const QString& roomId)
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket *>(sender());
    if (clientSocket) {
        // Trouver la room du client
        Room* room = findRoomById(roomId);
        if (room) {
            // Si la room contient d'autres clients, informer qu'elle est fermée
            if (room->clients.size() > 1) {
                QString leaveMessage = "CLIENT_LEFT_ROOM;\n";
                for (QTcpSocket *otherClient : room->clients) {
                    if (otherClient != clientSocket) {
                        otherClient->write(leaveMessage.toUtf8());
                    }
                }
            }

            // Supprimer tous les clients de la room
            for (QTcpSocket *client : room->clients) {
                clientRoomMap.remove(client);  // Retirer chaque client de la map
            }
            room->clients.clear();  // Vider la liste des clients
            room->clientCount=0;

            // Supprimer l'objet Game associé à la room
            if (roomGameMap.contains(roomId)) {
                this->games.removeOne(roomGameMap[roomId]);
                delete roomGameMap[roomId];
                roomGameMap.remove(roomId);
            }

            // Mettre à jour la liste des rooms pour tous les autres clients
            sendRoomInfoToClients();
        }
    }


}

void Server::onDisconnected()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket *>(sender());
    if (clientSocket) {
        // Trouver la room du client et décrémenter le nombre de clients
        Room* room = findRoomById(clientRoomMap[clientSocket]);
        if (room) {
            room->clientCount--;
            sendRoomInfoToClients();  // Met à jour la liste des rooms
        }

        clients.removeOne(clientSocket);
        clientSocket->deleteLater();
        qDebug() << "Un client s'est déconnecté.";
    }
}

void Server::broadcastMessageToRoom(const QString& roomId, const QString& chatMessage)
{
    Room* room = findRoomById(roomId);
    if (room) {
        QTcpSocket* senderSocket = qobject_cast<QTcpSocket*>(sender());

        // Diffuser le message à tous les clients sauf l'envoyeur
        for (QTcpSocket* client : room->clients) {
            if (client != senderSocket) {  // Ne pas renvoyer au client qui a envoyé le message
                QString formattedMessage = QString("SEND_CHAT_MESSAGE;%1;%2").arg(clientToNameMap[clientIdMap[senderSocket]], chatMessage);
                client->write(formattedMessage.toUtf8());
                client->flush();
            }
        }
    }
}

QList<QTcpSocket*> Server::clientsInRoom(const QString &_roomId) {
    Room* roomId = findRoomById(_roomId);
    QList<QTcpSocket*> roomClients;
    for (QTcpSocket *client : roomId->clients) {
        roomClients.append(client);
    }
    return roomClients;
}

