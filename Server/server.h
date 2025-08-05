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
    void sendBoardsUpdateToClients(QString& roomId);
    void sendStatusInfoToClients(QString& roomId);
    void sendErrorMessageToClients(QString& roomId, QString&);
    void broadcastMessageToRoom(const QString& roomId, const QString& message);
    void sendUpdateCaseToClients(QString& _roomId, int row, int col);
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
};

#endif // SERVER_H
