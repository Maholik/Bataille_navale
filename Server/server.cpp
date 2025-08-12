#include "server.h"
#include "game.h"
#include "AiAgent.h"
#include <QTimer>
#include <algorithm>
#include <QTcpSocket>
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include <QRandomGenerator>
#include <iostream>



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

bool Server::isNameTakenInRoom(const QString& roomId, const QString& playerName) const {
    const Room* room = const_cast<Server*>(this)->findRoomById(roomId);
    if (!room) return false;
    for (QTcpSocket* c : room->clients) {
        const int otherId = clientIdMap.value(c, -1);
        if (otherId != -1 && clientToNameMap.value(otherId) == playerName)
            return true;
    }
    return false;
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

static Boat* findBoatAt(Board* board, int row, int col) {
    if (!board) return nullptr;
    Case* target = board->getCase(row, col);
    for (Boat* b : board->getAllBoats()) {
        const auto& structure = b->getStructure();
        if (std::find(structure.begin(), structure.end(), target) != structure.end()) {
            return b;
        }
    }
    return nullptr;
}


void Server::sendBoardsUpdateToClients(QString& _roomId){
    Room* roomId = findRoomById(_roomId);
    if (!roomId) return;

    QString roomInfo = "BOARD_CREATE;";
    Game* gamemodel = this->roomGameMap[_roomId];
    gamemodel->displayBoards();

    // Dimensions + joueur 1
    roomInfo += QString::number(gamemodel->getPlayer1()->getBoard()->getRows()) + ";";
    roomInfo += QString::number(gamemodel->getPlayer1()->getBoard()->getCols()) + ";";
    roomInfo += QString::fromStdString(gamemodel->getPlayer1()->getName()) + ";";

    // Plateau joueur 1
    for (int row = 0; row < gamemodel->getPlayer1()->getBoard()->getRows(); ++row) {
        for (int col = 0; col < gamemodel->getPlayer1()->getBoard()->getCols(); ++col) {
            QString _caseString;
            Case* _case = gamemodel->getPlayer1()->getBoard()->getCase(row,col);
            switch (_case->getStatus()) {
            case Case::Empty: _caseString = "E"; break;
            case Case::Hit: _caseString = "H"; break;
            case Case::Miss: _caseString = "M"; break;
            case Case::Occupied: {
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
            } break;
            }
            roomInfo += _caseString + ";";
        }
    }

    // Joueur 2 + plateau joueur 2
    roomInfo += QString::fromStdString(gamemodel->getPlayer2()->getName()) + ";";
    for (int row1 = 0; row1 < gamemodel->getPlayer2()->getBoard()->getRows(); ++row1) {
        for (int col1 = 0; col1 < gamemodel->getPlayer2()->getBoard()->getCols(); ++col1) {
            QString _caseString1;
            Case* _case1 = gamemodel->getPlayer2()->getBoard()->getCase(row1,col1);
            switch (_case1->getStatus()) {
            case Case::Empty: _caseString1 = "E"; break;
            case Case::Hit: _caseString1 = "H"; break;
            case Case::Miss: _caseString1 = "M"; break;
            case Case::Occupied: {
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
            } break;
            }
            roomInfo += _caseString1 + ";";
        }
    }
    roomInfo += "\n";

    auto sendToParticipants = [&](const QString& text) {
        for (QTcpSocket* c : roomId->clients)     c->write(text.toUtf8());
        for (QTcpSocket* s : roomId->spectators)  s->write(text.toUtf8());
    };

    sendToParticipants(roomInfo);
    qDebug() << "Boards envoyés aux participants";
}


void Server::sendUpdateCaseToClients(QString& _roomId, int row, int col){
    Room* roomId = findRoomById(_roomId);
    if (!roomId) return;

    Game* gamemodel = this->roomGameMap[_roomId];

    auto sendToParticipants = [&](const QString& text) {
        for (QTcpSocket* c : roomId->clients)     c->write(text.toUtf8());
        for (QTcpSocket* s : roomId->spectators)  s->write(text.toUtf8());
    };

    // --- UPDATE_CASE ---
    {
        QString roomInfo = "UPDATE_CASE;";
        Case* _case = gamemodel->getOppositePlayer()->getBoard()->getCase(row,col);
        roomInfo += QString::fromStdString(gamemodel->getOppositePlayer()->getName()) + ";" + QString::number(row) + ";" + QString::number(col) + ";";
        QString _caseString = "X";
        switch (_case->getStatus()) {
        case Case::Hit:  _caseString = "H"; break;
        case Case::Miss: _caseString = "M"; break;
        default: break;
        }
        roomInfo += _caseString + ";\n";
        sendToParticipants(roomInfo);
        qDebug() << "Update envoyée (participants) : " << roomInfo;
    }

    // --- Détection "coulé" -> BOAT_SUNK pour tous ---
    Board* oppBoard = gamemodel->getOppositePlayer()->getBoard();
    Boat* hitBoat = findBoatAt(oppBoard, row, col);
    if (hitBoat && hitBoat->isSunk()) {
        QString sunkMsg = "BOAT_SUNK;";
        sunkMsg += QString::fromStdString(gamemodel->getOppositePlayer()->getName()) + ";";
        const auto& cells = hitBoat->getStructure();
        sunkMsg += QString::number(static_cast<int>(cells.size())) + ";";
        for (Case* cell : cells)
            sunkMsg += QString::number(cell->getRow()) + ";" + QString::number(cell->getCol()) + ";";
        sunkMsg += "\n";
        sendToParticipants(sunkMsg);
    }

    // --- scoring/drop + swap comme avant ---
    {
        QString attacker = QString::fromStdString(gamemodel->getCurrentPlayer()->getName());
        Board* opp = gamemodel->getOppositePlayer()->getBoard();
        Case* cell = opp->getCase(row, col);
        bool hit = (cell->getStatus() == Case::Hit);
        bool sunk = (hitBoat && hitBoat->isSunk());
        addScoreForHitAndSunk(_roomId, attacker, hit, sunk);
        tryDropPowerEvery4Turns(_roomId, attacker);
    }
    gamemodel->changePlayer();
}


void Server::sendStatusInfoToClients(QString& _roomId){
    Room* roomId = findRoomById(_roomId);
    if (!roomId) return;

    QString roomInfo = "STATUS;";
    Game* gamemodel = this->roomGameMap[_roomId];
    roomInfo += QString::fromStdString(gamemodel->getCurrentPlayer()->getName()) + ";";
    roomInfo += QString::number(gamemodel->isGameOver()) + ";";
    roomInfo += QString::fromStdString(gamemodel->getWinner());
    roomInfo += "\n";

    for (QTcpSocket *_client : qAsConst(roomId->clients))    _client->write(roomInfo.toUtf8());
    for (QTcpSocket *_spec   : qAsConst(roomId->spectators)) _spec->write(roomInfo.toUtf8());

    qDebug() << "Status envoyée (participants) : " << roomInfo;
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

        // Refus si nom déjà pris dans la room
        if (isNameTakenInRoom(roomId, playerName)) {
            clientSocket->write("ERR_NAME_TAKEN\n");
            clientSocket->flush();
            return;
        }


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
                    // Init états joueurs (2 joueurs)
                    roomState[roomId][QString::fromStdString(player1->getName())] = PlayerState{};
                    roomState[roomId][QString::fromStdString(player2->getName())] = PlayerState{};

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
    else if (message.startsWith("ATTACK")) {
        QStringList parts = message.split(";");
        QString roomId = parts[1];
        int row = parts[2].toInt();
        int col = parts[3].toInt();

        Game* gamemodel = roomGameMap[roomId];

        // Le joueur HUMAIN vient d'attaquer
        gamemodel->attackPlayer(row, col);

        // Envoie la MAJ de case (cette fonction change déjà le joueur à la fin)
        sendUpdateCaseToClients(roomId, row, col);

        // Envoie l'état après le changement de joueur
        sendStatusInfoToClients(roomId);

        // >>> Déclenche le tour de l'IA si on est en mode solo et que c'est à elle <<<
        if (roomAiMap.contains(roomId)
            && QString::fromStdString(gamemodel->getCurrentPlayer()->getName()) == "AI"
            && !gamemodel->isGameOver()) {

            QTimer::singleShot(300, [this, roomId]() {
                playAiTurn(roomId);
            });
        }
    }

    else if (message.startsWith("MISSILE;")) {
        QStringList parts = message.split(';', Qt::SkipEmptyParts);
        if (parts.size() < 4) { clientSocket->write("POWER_ERR;BAD_MISSILE_FORMAT\n"); return; }
        QString roomId = parts[1];
        int row = parts[2].toInt();
        int col = parts[3].toInt();

        Game* gamemodel = roomGameMap.value(roomId, nullptr);
        if (!gamemodel) return;

        QString attacker = QString::fromStdString(gamemodel->getCurrentPlayer()->getName());

        // Vérif inventaire/score
        if (!canUseMissile(roomId, attacker)) {
            clientSocket->write("POWER_ERR;MISSILE_NOT_AVAILABLE_OR_NOT_ENOUGH_SCORE\n");
            return;
        }
        consumeMissileOrPay(roomId, attacker);

        Board* targetBoard = gamemodel->getOppositePlayer()->getBoard();

        // Stats du missile
        int hitsThisMissile = 0;
        QSet<Boat*> boatsBefore;   // bateaux rencontrés avant tirs
        QSet<Boat*> boatsAfter;    // bateaux encore vivants après tirs (pour comparer)

        // --- 1) Collecte des bateaux concernés AVANT ---
        auto findBoatAtLocal = [](Board* board, int r, int c)->Boat* {
            Case* cell = board->getCase(r,c);
            for (Boat* b : board->getAllBoats()) {
                const auto& s = b->getStructure();
                if (std::find(s.begin(), s.end(), cell) != s.end()) return b;
            }
            return nullptr;
        };

        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                int r = row + dr, c = col + dc;
                if (r < 0 || r >= targetBoard->getRows() || c < 0 || c >= targetBoard->getCols()) continue;
                Boat* b = findBoatAtLocal(targetBoard, r, c);
                if (b && !b->isSunk()) boatsBefore.insert(b);
            }
        }

        // --- 2) Appliquer les 9 tirs + push UPDATE_CASE (sans swap de tour) ---
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                int r = row + dr, c = col + dc;
                if (r < 0 || r >= targetBoard->getRows() || c < 0 || c >= targetBoard->getCols()) continue;

                // Tire (même si déjà touché) :
                gamemodel->attackPlayer(r, c);

                // Compte un "hit" si la case est Hit après tir
                if (targetBoard->getCase(r, c)->getStatus() == Case::Hit) {
                    hitsThisMissile++;
                }

                // Envoie la mise à jour de case sans changer de joueur
                QString rid = roomId;
                sendUpdateCaseNoTurnSwap(rid, r, c);
            }
        }

        // --- 3) Compter les bateaux coulés par ce missile ---
        int sunkThisMissile = 0;
        for (Boat* b : std::as_const(boatsBefore)) {
            if (b->isSunk()) sunkThisMissile++;
        }

        // --- 4) Scoring cumulé du missile ---
        for (int i = 0; i < hitsThisMissile; ++i)
            addScoreForHitAndSunk(roomId, attacker, /*hit=*/true,  /*sunk=*/false);
        for (int i = 0; i < sunkThisMissile; ++i)
            addScoreForHitAndSunk(roomId, attacker, /*hit=*/false, /*sunk=*/true);

        // --- 5) Drop auto + FIN DE TOUR (une seule fois) ---
        tryDropPowerEvery4Turns(roomId, attacker);
        gamemodel->changePlayer();

        // --- 6) Status + tour IA éventuel ---
        sendStatusInfoToClients(roomId);

        if (roomAiMap.contains(roomId)
            && QString::fromStdString(gamemodel->getCurrentPlayer()->getName()) == "AI"
            && !gamemodel->isGameOver()) {
            QTimer::singleShot(300, [this, roomId]() { playAiTurn(roomId); });
        }
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
    else if (message.startsWith("RECONNAISANCE;")) {
        QStringList parts = message.split(";", Qt::SkipEmptyParts);
        if (parts.size() < 4) { clientSocket->write("POWER_ERR;BAD_RECON_FORMAT\n"); return; }

        QString roomId = parts[1];
        int row = parts[2].toInt();
        int col = parts[3].toInt();

        Game* gamemodel = roomGameMap.value(roomId, nullptr);
        if (!gamemodel) return;

        QString attacker = QString::fromStdString(gamemodel->getCurrentPlayer()->getName());

        if (!canUseScanner(roomId, attacker)) {
            clientSocket->write("POWER_ERR;SCANNER_NOT_AVAILABLE_OR_NOT_ENOUGH_SCORE\n");
            return;
        }
        consumeScannerOrPay(roomId, attacker);

        int nbBateauInZone = gamemodel->reconnaissanceZone(row, col);
        qDebug() << "[SERVER] RECO result for" << attacker << "at" << row << col << "=>" << nbBateauInZone;

        QString reply = "RECONNAISANCE_RESULT;" + QString::number(nbBateauInZone) + "\n";
        clientSocket->write(reply.toUtf8());
        clientSocket->flush();

        tryDropPowerEvery4Turns(roomId, attacker);
        gamemodel->changePlayer();
        sendStatusInfoToClients(roomId);

        if (roomAiMap.contains(roomId)
            && QString::fromStdString(gamemodel->getCurrentPlayer()->getName()) == "AI"
            && !gamemodel->isGameOver()) {
            QTimer::singleShot(300, [this, roomId]() { playAiTurn(roomId); });
        }
    }

    else if (message.startsWith("SPECTATE_ROOM;")) {
        QStringList parts = message.split(";", Qt::SkipEmptyParts);
        // SPECTATE_ROOM;nickname;roomId;[password]
        if (parts.size() < 3) { clientSocket->write("ERR_INVALID_PASSWORD_OR_ROOM_NOT_FOUND\n"); return; }

        QString nickname = parts[1];
        QString roomId   = parts[2];
        QString password = parts.size() > 3 ? parts[3] : "";

        // mémoriser le pseudo pour ce socket (comme pour JOIN_ROOM)
        int idClient = clientIdMap[clientSocket];
        clientToNameMap[idClient] = nickname;

        if (!checkSpectatorAccess(roomId, password)) {
            clientSocket->write("ERR_INVALID_PASSWORD_OR_ROOM_NOT_FOUND\n");
            return;
        }

        Room* room = findRoomById(roomId);
        clientRoomMap[clientSocket] = roomId;
        room->spectators.append(clientSocket);

        // Accusé de réception spécifique spectateur
        clientSocket->write(QString("SPECTATE_OK;%1\n").arg(roomId).toUtf8());
        clientSocket->flush();

        // Envoyer tout de suite l'état courant
        sendBoardsUpdateToClients(roomId);  // va être étendu pour cibler aussi spectators
        sendStatusInfoToClients(roomId);
    }



    else if (message.startsWith("START_SOLO;")) {
        QStringList parts = message.split(';');
        if (parts.size() >= 2) {
            const QString playerName = parts[1].trimmed(); // <-- important
            createSoloGame(clientSocket, playerName);
        } else {
            clientSocket->write("ERR_BAD_SOLO_FORMAT\n");
        }
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
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket *>(sender()); // joueur qui a quitté via QUIT_ROOM
    Room* room = findRoomById(roomId);
    if (!room) return;

    // 1) Prévenir tout le monde que la room est fermée (autre joueur + tous les spectateurs)
    const QString leaveMessage = "CLIENT_LEFT_ROOM;\n";

    // Autres joueurs
    for (QTcpSocket *otherClient : room->clients) {
        if (otherClient != clientSocket) {
            otherClient->write(leaveMessage.toUtf8());
        }
    }
    // Spectateurs
    for (QTcpSocket *spec : room->spectators) {
        if (spec != clientSocket) { // par principe ; clientSocket n'est normalement pas spectateur
            spec->write(leaveMessage.toUtf8());
        }
    }

    // 2) Nettoyer les mappings room -> sockets
    for (QTcpSocket *c : room->clients) {
        clientRoomMap.remove(c);
    }
    for (QTcpSocket *s : room->spectators) {
        clientRoomMap.remove(s);
    }

    // 3) Vider les listes et remonter les compteurs à zéro
    room->clients.clear();
    room->clientCount = 0;

    room->spectators.clear();
    room->spectatorCount = 0;

    // 4) Détruire le Game associé (comme avant)
    if (roomGameMap.contains(roomId)) {
        this->games.removeOne(roomGameMap[roomId]);
        delete roomGameMap[roomId];
        roomGameMap.remove(roomId);
    }

    // 5) Mettre à jour la liste des rooms
    sendRoomInfoToClients();
}


void Server::onDisconnected()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket *>(sender());
    if (!clientSocket) return;

    // Retrouver la room (si ce socket en avait une)
    const QString rid = clientRoomMap.value(clientSocket, QString());
    Room* room = (!rid.isEmpty() ? findRoomById(rid) : nullptr);

    if (room) {
        // Le socket peut être un joueur OU un spectateur
        bool wasClient    = room->clients.removeOne(clientSocket);
        bool wasSpectator = room->spectators.removeOne(clientSocket);

        if (wasClient && room->clientCount > 0)
            room->clientCount--;
        if (wasSpectator && room->spectatorCount > 0)
            room->spectatorCount--;

        // Ne pas toucher clientCount si c'était un spectateur
        // Met à jour la liste des rooms pour tous
        sendRoomInfoToClients();  // affiche les rooms avec clientCount à jour
    }

    // Nettoyage des structures globales
    clientRoomMap.remove(clientSocket);
    clients.removeOne(clientSocket);
    clientSocket->deleteLater();

    qDebug() << "Un client s'est déconnecté.";
}


void Server::broadcastMessageToRoom(const QString& roomId, const QString& chatMessage)
{
    Room* room = findRoomById(roomId);
    if (!room) return;

    QTcpSocket* senderSocket = qobject_cast<QTcpSocket*>(sender());
    const QString from = clientToNameMap[clientIdMap[senderSocket]];
    const QString formatted = QString("SEND_CHAT_MESSAGE;%1;%2").arg(from, chatMessage);

    // Clients
    for (QTcpSocket* client : room->clients) {
        if (client != senderSocket) {
            client->write(formatted.toUtf8());
            client->flush();
        }
    }
    // Spectateurs
    for (QTcpSocket* spec : room->spectators) {
        if (spec != senderSocket) {
            spec->write(formatted.toUtf8());
            spec->flush();
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

void Server::createSoloGame(QTcpSocket* humanSocket, const QString& playerName)
{
    // créer une "room" dédiée
    QString roomId = QString("solo_%1").arg(clientIdMap[humanSocket]);
    createRoom(roomId, "Solo vs AI", "publique", "");
    Room* room = findRoomById(roomId);
    room->clients.append(humanSocket);
    clientRoomMap[humanSocket] = roomId;
    room->clientCount = 1;

    // Créer joueurs
    Player* human = new Player{ playerName.toStdString() };
    Player* ai    = new Player{ std::string("AI") };

    // Game
    Game* game = new Game(human, ai);
    games.append(game);
    roomGameMap[roomId] = game;
    game->initializeGame();

    // IA (dimension 10x10 aujourd'hui)
    roomAiMap[roomId] = new AiAgent(human->getBoard()->getRows(), human->getBoard()->getCols());

    // prévenir le client que la room est prête
    humanSocket->write(QString("ROOM_READY;%1\n").arg(roomId).toUtf8());
    humanSocket->flush();
    // Init états joueurs
    roomState[roomId][QString::fromStdString(human->getName())] = PlayerState{};
    roomState[roomId][QString::fromStdString(ai->getName())]    = PlayerState{};


    // envoyer les boards + status
    sendBoardsUpdateToClients(roomId);
    sendStatusInfoToClients(roomId);

    // si c'est à l'IA de jouer en premier, jouer tout de suite
    if (QString::fromStdString(game->getCurrentPlayer()->getName()) == "AI") {
        QTimer::singleShot(300, [this, roomId]() { playAiTurn(roomId); });
    }
}

void Server::playAiTurn(const QString& roomId)
{
    Game* game = roomGameMap.value(roomId, nullptr);
    Room* room = findRoomById(roomId);
    AiAgent* ai = roomAiMap.value(roomId, nullptr);
    if (!game || !room || !ai) return;

    // sécurité: ne jouer que si c'est bien le tour de l'IA
    if (QString::fromStdString(game->getCurrentPlayer()->getName()) != "AI") return;

    // Choisir un tir
    auto move = ai->nextMove();
    int r = move.first, c = move.second;
    if (r < 0 || c < 0) return;

    // On tire sur le plateau de l'humain (oppositePlayer du joueur courant)
    Board* humanBoard = game->getOppositePlayer()->getBoard();
    // Évaluer le statut avant/après
    bool wasHit = humanBoard->getCase(r,c)->getStatus() == Case::Occupied;
    game->attackPlayer(r,c); // applique Hit/Miss

    // Envoyer mise à jour case
    QString rid = roomId;
    sendUpdateCaseToClients(rid, r, c);

    // Détection coulé (on réutilise ta logique "BOAT_SUNK" si présente,
    // sinon on informe l'IA via isSunk)
    bool sunk = false;
    // On teste si la case touchée appartient à un bateau désormais coulé
    // (reprise du helper suggéré précédemment)
    auto findBoatAt = [](Board* board, int row, int col)->Boat* {
        Case* target = board->getCase(row,col);
        for (Boat* b : board->getAllBoats()) {
            const auto& s = b->getStructure();
            if (std::find(s.begin(), s.end(), target) != s.end())
                return b;
        }
        return nullptr;
    };
    Boat* b = findBoatAt(humanBoard, r, c);
    if (b && b->isSunk()) sunk = true;

    ai->onResult(r, c, wasHit ? 'H' : 'M', sunk);

    // Envoyer status et, si maintenant c'est encore à l'IA (selon ta règle de tour),
    // tu peux rejouer au prochain tick. Dans ton code actuel, le tour change à chaque tir,
    // donc ça repassera au joueur humain.
    sendStatusInfoToClients(rid);

    // Si le jeu est fini, on ne relance rien
    if (game->isGameOver()) return;

    // Si ta règle de tour change (ex: rejouer en cas de Hit), tu peux faire :
    // if (QString::fromStdString(game->getCurrentPlayer()->getName()) == "AI")
    //     QTimer::singleShot(300, [this, roomId]() { playAiTurn(roomId); });
}

void Server::sendUpdateCaseNoTurnSwap(QString& _roomId, int row, int col)
{
    Room* roomId = findRoomById(_roomId);
    if (!roomId) return;

    Game* gamemodel = this->roomGameMap[_roomId];

    auto sendToParticipants = [&](const QString& text) {
        for (QTcpSocket* c : roomId->clients)     c->write(text.toUtf8());
        for (QTcpSocket* s : roomId->spectators)  s->write(text.toUtf8());
    };

    // UPDATE_CASE sans swap
    {
        QString roomInfo = "UPDATE_CASE;";
        Case* _case = gamemodel->getOppositePlayer()->getBoard()->getCase(row,col);
        roomInfo += QString::fromStdString(gamemodel->getOppositePlayer()->getName()) + ";" + QString::number(row) + ";" + QString::number(col) + ";";
        QString _caseString = "X";
        switch (_case->getStatus()) {
        case Case::Hit:  _caseString = "H"; break;
        case Case::Miss: _caseString = "M"; break;
        default: break;
        }
        roomInfo += _caseString + ";\n";
        sendToParticipants(roomInfo);
    }

    // Détection "coulé" → BOAT_SUNK (sans swap)
    Board* oppBoard = gamemodel->getOppositePlayer()->getBoard();
    Boat* hitBoat = findBoatAt(oppBoard, row, col);
    if (hitBoat && hitBoat->isSunk()) {
        QString sunkMsg = "BOAT_SUNK;";
        sunkMsg += QString::fromStdString(gamemodel->getOppositePlayer()->getName()) + ";";
        const auto& cells = hitBoat->getStructure();
        sunkMsg += QString::number(static_cast<int>(cells.size())) + ";";
        for (Case* cell : cells)
            sunkMsg += QString::number(cell->getRow()) + ";" + QString::number(cell->getCol()) + ";";
        sunkMsg += "\n";
        sendToParticipants(sunkMsg);
    }
}


void Server::broadcastScoreAndInv(const QString& roomId, const QString& player) {
    Room* room = findRoomById(roomId);
    if (!room) return;
    const auto &ps = roomState[roomId][player];
    QString msg = QString("SCORE_UPDATE;%1;%2;%3;%4\n")
                      .arg(player)
                      .arg(ps.score)
                      .arg(ps.scanners)
                      .arg(ps.missiles);
    for (QTcpSocket* c : room->clients) c->write(msg.toUtf8());
    //Envoi score et inventaire for (QTcpSocket* s : room->spectators) s->write(msg.toUtf8());
}

// règles de score (ajuste si tu veux)
static constexpr int SCORE_HIT  = 10;
static constexpr int SCORE_SUNK = 30;

void Server::addScoreForHitAndSunk(const QString& roomId, const QString& attacker, bool hit, bool sunk) {
    auto &ps = roomState[roomId][attacker];
    if (hit)  ps.score += SCORE_HIT;
    if (sunk) ps.score += SCORE_SUNK;
    broadcastScoreAndInv(roomId, attacker);
}

void Server::tryDropPowerEvery4Turns(const QString& roomId, const QString& attacker) {
    auto &ps = roomState[roomId][attacker];
    ps.turnsSinceDrop++;
    if (ps.turnsSinceDrop >= 4) {
        ps.turnsSinceDrop = 0;
        bool dropScanner = (QRandomGenerator::global()->bounded(2) == 0);
        if (dropScanner) ps.scanners++;
        else             ps.missiles++;

        Room* room = findRoomById(roomId);
        if (room) {
            QString msg = "POWER_AVAILABLE;" + attacker + ";" + QString(dropScanner ? "SCANNER" : "MISSILE") + "\n";
            for (QTcpSocket* c : room->clients) c->write(msg.toUtf8());
        }
        broadcastScoreAndInv(roomId, attacker);
    }
}

// Achat / usage missile
static constexpr int MISSILE_COST = 80;

bool Server::canUseMissile(const QString& roomId, const QString& attacker) const {
    const auto &ps = const_cast<Server*>(this)->roomState[roomId][attacker];
    return ps.missiles > 0 || ps.score >= MISSILE_COST;
}

void Server::consumeMissileOrPay(const QString& roomId, const QString& attacker) {
    auto &ps = roomState[roomId][attacker];
    if (ps.missiles > 0) {
        ps.missiles--;
    } else {
        ps.score -= MISSILE_COST; // paye en score
    }
    broadcastScoreAndInv(roomId, attacker);

}

// Achat / usage missile
// Achat / usage scanner (reconnaissance)
static constexpr int SCANNER_COST = 30;



bool Server::canUseScanner(const QString& roomId, const QString& attacker) const {
    const auto &ps = const_cast<Server*>(this)->roomState[roomId][attacker];
    return ps.scanners > 0 || ps.score >= SCANNER_COST;
}
void Server::consumeScannerOrPay(const QString& roomId, const QString& attacker) {
    auto &ps = roomState[roomId][attacker];
    if (ps.scanners > 0) ps.scanners--;
    else                 ps.score -= SCANNER_COST;
    broadcastScoreAndInv(roomId, attacker);
}

bool Server::checkSpectatorAccess(const QString &id_room, const QString &password)
{
    Room* room = findRoomById(id_room);
    if (!room) return false;
    if (room->type == "privée" && room->password != password) return false;

               // Option : n’autoriser que si une partie est en cours (2 joueurs déjà prêts)
               const bool gameRunning = roomGameMap.contains(id_room);
    if (!gameRunning) return false;

    room->spectatorCount++;
    return true;
}









