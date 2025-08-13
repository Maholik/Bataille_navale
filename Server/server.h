#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QMap>
#include "game.h"

class Server : public QObject
{
    Q_OBJECT

public:
    explicit Server(QObject *parent = nullptr);
    ~Server();
    void startServer();
    void stopServer();
    void createRoom(const QString& id_room, const QString& name, const QString& type, const QString& password);
    void sendRoomInfoToClients();
    void sendBoardsUpdateToClients(const QString& roomId);
    void sendStatusInfoToClients(const QString& roomId);
    void sendErrorMessageToClients(const QString& roomId, const QString& errorMessage);
    void broadcastMessageToRoom(const QString& roomId, const QString& message);
    void broadcastSystemMessage(const QString& roomId, const QString& text);

    void sendUpdateCaseToClients(const QString& _roomId, int row, int col);
    QList<QTcpSocket*> clientsInRoom(const QString &roomId);
    void onRoomDisconnected(const QString&);


private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    struct Room {
        QString id;
        QString name;
        QString type;  // "privée" ou "publique"
        QString password;
        int clientCount;
        QList<QTcpSocket*> clients;
        int spectatorCount = 0;
        QList<QTcpSocket*> spectators;

        // NEW : pour éviter doublons d'annonces
        bool startAnnounced = false;
        bool endAnnounced   = false;
    };

    // --- ÉTAT PAR JOUEUR (score, drops, inventaire) ---
    struct PlayerState {
        int score = 0;
        int turnsSinceDrop = 0;  // +1 à chaque tour joué ; drop à 4
        int scanners = 0;
        int missiles = 0;
    };

    struct SubmittedBoat {
        int size; int row; int col; bool horizontal;
    };

    QTcpServer *server;
    QList<QTcpSocket*> clients;
    QMap<QTcpSocket*, QString> clientRoomMap;
    QMap<QTcpSocket*, int> clientIdMap;  // Mappage des sockets aux IDs des clients
    QMap<int,QString> clientToNameMap;
    QMap<QString, Game*> roomGameMap;
    QList<Room> rooms;
    QList<Game*> games;

    int clientIdCounter;  // Compteur pour les ID des clients

    Room* findRoomById(const QString& id_room);
    bool checkRoomAccess(const QString& id_room, const QString& password);
    void handleClientRequest(QTcpSocket* client, const QString& request);
    bool isNameTakenInRoom(const QString& roomId, const QString& playerName) const;
    QMap<QString, class AiAgent*> roomAiMap; // IA par room (si solo)
    void playAiTurn(const QString& roomId);  // lance un coup IA
    void createSoloGame(QTcpSocket* humanSocket, const QString& playerName);
    // roomId -> (playerName -> PlayerState)
    QMap<QString, QMap<QString, PlayerState>> roomState;

    // --- HELPERS (déclarations) ---
    void sendUpdateCaseNoTurnSwap(QString& _roomId, int row, int col);
    void broadcastScoreAndInv(const QString& roomId, const QString& player);

    // scoring d’un tir unitaire (hit/sunk)
    void addScoreForHitAndSunk(const QString& roomId, const QString& attacker, bool hit, bool sunk);

    // drop aléatoire (appelé fin de tour)
    void tryDropPowerEvery4Turns(const QString& roomId, const QString& attacker);

    // achat/usage missile
    bool canUseMissile(const QString& roomId, const QString& attacker) const;
    void consumeMissileOrPay(const QString& roomId, const QString& attacker);

    // achat/usage scanner (reconnaissance)
    bool canUseScanner(const QString& roomId, const QString& attacker) const;
    void consumeScannerOrPay(const QString& roomId, const QString& attacker);

    bool checkSpectatorAccess(const QString& id_room, const QString& password);

    // Salle en phase de placement ?
    QMap<QString, bool> roomInPlacement;

    // roomId -> (playerName -> liste de bateaux soumis)
    QMap<QString, QMap<QString, QVector<SubmittedBoat>>> pendingPlacements;






};

#endif // SERVER_H
