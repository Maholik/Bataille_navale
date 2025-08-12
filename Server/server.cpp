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
    // --- Détection "coulé" ---
    Board* oppBoard = gamemodel->getOppositePlayer()->getBoard();
    Boat* hitBoat = findBoatAt(oppBoard, row, col);

    if (hitBoat && hitBoat->isSunk()) {
        // Construire le message: BOAT_SUNK;ownerName;N;row1;col1;...;rowN;colN
        QString sunkMsg = "BOAT_SUNK;";
        sunkMsg += QString::fromStdString(gamemodel->getOppositePlayer()->getName()) + ";";

        const auto& cells = hitBoat->getStructure();
        sunkMsg += QString::number(static_cast<int>(cells.size())) + ";";
        for (Case* cell : cells) {
            sunkMsg += QString::number(cell->getRow()) + ";" + QString::number(cell->getCol()) + ";";
        }
        sunkMsg += "\n";

        for (QTcpSocket *client : roomId->clients) {
            client->write(sunkMsg.toUtf8());
        }
    }
    // --- scoring & drop pour TIR SIMPLE (1 case) ---
    {
        QString attacker = QString::fromStdString(gamemodel->getCurrentPlayer()->getName());

        // évalue hit/sunk depuis l'état déjà appliqué
        Board* oppBoard = gamemodel->getOppositePlayer()->getBoard();
        Case* cell = oppBoard->getCase(row, col);
        bool hit = (cell->getStatus() == Case::Hit);

        // on a déjà le test sunk ci-dessus (hitBoat)
        bool sunk = (hitBoat && hitBoat->isSunk());

        addScoreForHitAndSunk(_roomId, attacker, hit, sunk);

        // drop aléatoire tous les 4 tours
        tryDropPowerEvery4Turns(_roomId, attacker);
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

        // Joueur qui demande la reco = joueur courant
        QString attacker = QString::fromStdString(gamemodel->getCurrentPlayer()->getName());

        // Vérif inventaire/score
        if (!canUseScanner(roomId, attacker)) {
            clientSocket->write("POWER_ERR;SCANNER_NOT_AVAILABLE_OR_NOT_ENOUGH_SCORE\n");
            return;
        }
        // Déduction (inventaire ou score)
        consumeScannerOrPay(roomId, attacker);

        // Calcul du résultat
        int nbBateauInZone = gamemodel->reconnaissanceZone(row, col);

        // Retourner le résultat au SEUL demandeur
        QString reply = "RECONNAISANCE_RESULT;" + QString::number(nbBateauInZone) + "\n";
        clientSocket->write(reply.toUtf8());

        // Drop auto (tous les 4 tours) + fin de tour
        tryDropPowerEvery4Turns(roomId, attacker);
        gamemodel->changePlayer();

        // Statut & IA éventuelle
        sendStatusInfoToClients(roomId);
        if (roomAiMap.contains(roomId)
            && QString::fromStdString(gamemodel->getCurrentPlayer()->getName()) == "AI"
            && !gamemodel->isGameOver()) {
            QTimer::singleShot(300, [this, roomId]() { playAiTurn(roomId); });
        }
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
    Game* gamemodel = this->roomGameMap[_roomId];

    // envoi de l'état de case (H/M) côté joueur opposé actuel
    for (QTcpSocket *client : roomId->clients) {
        QString roomInfo = "UPDATE_CASE;";
        Case* _case = gamemodel->getOppositePlayer()->getBoard()->getCase(row,col);
        roomInfo += QString::fromStdString(gamemodel->getOppositePlayer()->getName()) + ";" + QString::number(row) + ";" + QString::number(col) + ";";
        QString _caseString = "X";
        switch (_case->getStatus()) {
        case Case::Hit: _caseString = "H"; break;
        case Case::Miss: _caseString = "M"; break;
        default: break;
        }
        roomInfo += _caseString + ";\n";
        client->write(roomInfo.toUtf8());
    }

    // détection "coulé" → envoi BOAT_SUNK si besoin (mais PAS de changePlayer ici)
    Board* oppBoard = gamemodel->getOppositePlayer()->getBoard();
    Boat* hitBoat = findBoatAt(oppBoard, row, col);
    if (hitBoat && hitBoat->isSunk()) {
        QString sunkMsg = "BOAT_SUNK;";
        sunkMsg += QString::fromStdString(gamemodel->getOppositePlayer()->getName()) + ";";

        const auto& cells = hitBoat->getStructure();
        sunkMsg += QString::number(static_cast<int>(cells.size())) + ";";
        for (Case* cell : cells) {
            sunkMsg += QString::number(cell->getRow()) + ";" + QString::number(cell->getCol()) + ";";
        }
        sunkMsg += "\n";

        for (QTcpSocket *client : roomId->clients) {
            client->write(sunkMsg.toUtf8());
        }
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







