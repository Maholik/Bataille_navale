#ifndef SERVER_H
#define SERVER_H

/**
 * \file server.h
 * \brief Serveur Qt : rooms, sockets, sérialisation des événements de jeu.
 * \ingroup server-net
 */

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QMap>
#include <QString>
#include <QVector>
#include "game.h"

// Forward decl. pour éviter l'include lourd ici (AiAgent est utilisé en pointeur)
class AiAgent;

/**
 * \class Server
 * \brief Serveur TCP gérant les rooms, la phase de placement et la bataille.
 * \details Sérialise les états (\c BOARD_CREATE, \c STATUS, \c UPDATE_CASE, etc.)
 * vers les clients/joueurs et spectateurs. Gère aussi le mode solo (IA),
 * le scoring et l'inventaire de pouvoirs (scanner, missile).
 */
class Server : public QObject
{
    Q_OBJECT

public:
    /**
     * \brief Constructeur du serveur.
     * \param parent QObject parent (optionnel).
     */
    explicit Server(QObject *parent = nullptr);

    /// Destructeur.
    ~Server();

    /**
     * \brief Démarre l'écoute TCP et initialise les structures internes.
     */
    void startServer();

    /**
     * \brief Arrête proprement le serveur (ferme les sockets).
     */
    void stopServer();

    /**
     * \brief Crée une salle.
     * \param id_room Identifiant unique de la room.
     * \param name Nom affiché.
     * \param type "privée" ou "publique".
     * \param password Mot de passe si privée (sinon vide).
     */
    void createRoom(const QString& id_room, const QString& name,
                    const QString& type, const QString& password);

    /// Envoie à tous les clients la liste des salles et leurs infos.
    void sendRoomInfoToClients();

    /**
     * \brief Diffuse un snapshot des deux plateaux aux clients d'une room.
     * \param roomId Identifiant de la room.
     */
    void sendBoardsUpdateToClients(const QString& roomId);

    /**
     * \brief Diffuse le statut courant (joueur actif, fin de partie...).
     * \param roomId Identifiant de la room.
     */
    void sendStatusInfoToClients(const QString& roomId);

    /**
     * \brief Diffuse un message d'erreur à tous les clients d'une room.
     * \param roomId Room cible.
     * \param errorMessage Message à afficher côté client.
     */
    void sendErrorMessageToClients(const QString& roomId, const QString& errorMessage);

    /**
     * \brief Diffuse un message "chat" utilisateur à une room.
     * \param roomId Room cible.
     * \param message Contenu du message.
     */
    void broadcastMessageToRoom(const QString& roomId, const QString& message);

    /**
     * \brief Diffuse un message système (préfixé/formaté côté client).
     * \param roomId Room cible.
     * \param text Texte système.
     */
    void broadcastSystemMessage(const QString& roomId, const QString& text);

    /**
     * \brief Envoie une mise à jour d'une case (après tir ou effet).
     * \param _roomId Room cible.
     * \param row Ligne.
     * \param col Colonne.
     */
    void sendUpdateCaseToClients(const QString& _roomId, int row, int col);

    /**
     * \brief Retourne la liste des sockets clients présents dans une room.
     * \param roomId Identifiant de la room.
     */
    QList<QTcpSocket*> clientsInRoom(const QString &roomId);

    /**
     * \brief À appeler lorsqu'une room est entièrement déconnectée.
     * \param roomId Identifiant de la room.
     */
    void onRoomDisconnected(const QString&);

private slots:
    /// Connexion entrante au QTcpServer.
    void onNewConnection();

    /// Lecture d'un message depuis un socket client.
    void onReadyRead();

    /// Gestion d'une déconnexion de socket.
    void onDisconnected();

private:
    /**
     * \struct Room
     * \brief Métadonnées d'une salle de jeu + participants.
     */
    struct Room {
        QString id;                      ///< Identifiant unique.
        QString name;                    ///< Nom affiché.
        QString type;                    ///< "privée" ou "publique".
        QString password;                ///< Mot de passe si privée (sinon vide).
        int clientCount;                 ///< Nb de joueurs.
        QList<QTcpSocket*> clients;      ///< Sockets des joueurs.
        int spectatorCount = 0;          ///< Nb de spectateurs.
        QList<QTcpSocket*> spectators;   ///< Sockets des spectateurs.
        bool startAnnounced = false;     ///< Évite les annonces multiples.
        bool endAnnounced   = false;     ///< Idem pour la fin de partie.
    };

    /**
     * \struct PlayerState
     * \brief État gameplay par joueur (score, cadence de drop, inventaire).
     */
    struct PlayerState {
        int score = 0;           ///< Score cumulé.
        int turnsSinceDrop = 0;  ///< +1 à chaque tour ; drop d'un power à 4.
        int scanners = 0;        ///< Inventaire de scanners.
        int missiles = 0;        ///< Inventaire de missiles.
    };

    /**
     * \struct SubmittedBoat
     * \brief Spécification d'un bateau soumis lors du placement.
     */
    struct SubmittedBoat { int size; int row; int col; bool horizontal; };

    // ---------- États membres ----------
    QTcpServer *server;                           ///< Serveur TCP.
    QList<QTcpSocket*> clients;                   ///< Tous les sockets connectés.
    QMap<QTcpSocket*, QString> clientRoomMap;     ///< socket -> roomId.
    QMap<QTcpSocket*, int> clientIdMap;           ///< socket -> id unique.
    QMap<int,QString> clientToNameMap;            ///< id -> pseudo.
    QMap<QString, Game*> roomGameMap;             ///< roomId -> modèle de jeu.
    QList<Room> rooms;                            ///< Liste des rooms.
    QList<Game*> games;                           ///< Jeux alloués (lifetime).
    int clientIdCounter;                          ///< Compteur d'IDs clients.

    // ---------- Aides & utilitaires ----------
    /**
     * \brief Retourne le pointeur Room* pour un id donné, ou nullptr.
     */
    Room* findRoomById(const QString& id_room);

    /**
     * \brief Vérifie l'accès (type + mot de passe) pour rejoindre une room.
     */
    bool checkRoomAccess(const QString& id_room, const QString& password);

    /**
     * \brief Route une requête texte brute d'un client (si utilisé).
     */
    void handleClientRequest(QTcpSocket* client, const QString& request);

    /**
     * \brief Vrai si \p playerName est déjà utilisé dans \p roomId.
     */
    bool isNameTakenInRoom(const QString& roomId, const QString& playerName) const;

    /**
     * \brief Map des IA pour les rooms solo.
     */
    QMap<QString, AiAgent*> roomAiMap;

    /**
     * \brief Joue le tour de l'IA pour la room donnée.
     */
    void playAiTurn(const QString& roomId);

    /**
     * \brief Crée une partie solo (humain vs IA) et la room associée.
     */
    void createSoloGame(QTcpSocket* humanSocket, const QString& playerName);

    /**
     * \brief roomId -> (playerName -> PlayerState).
     */
    QMap<QString, QMap<QString, PlayerState>> roomState;

    /**
     * \brief Envoie une update de case sans inversion de tour.
     */
    void sendUpdateCaseNoTurnSwap(QString& _roomId, int row, int col);

    /**
     * \brief Diffuse score + inventaire d'un joueur vers la room.
     */
    void broadcastScoreAndInv(const QString& roomId, const QString& player);

    /**
     * \brief Met à jour le score d'un tir (touché/coulé).
     */
    void addScoreForHitAndSunk(const QString& roomId, const QString& attacker, bool hit, bool sunk);

    /**
     * \brief Essaye de dropper un power toutes les 4 actions d'un joueur.
     */
    void tryDropPowerEvery4Turns(const QString& roomId, const QString& attacker);

    /**
     * \brief Vérifie si un joueur peut utiliser le missile (inv/achat).
     */
    bool canUseMissile(const QString& roomId, const QString& attacker) const;

    /**
     * \brief Décrémente l'inventaire missile ou effectue l'achat si besoin.
     */
    void consumeMissileOrPay(const QString& roomId, const QString& attacker);

    /**
     * \brief Vérifie si un joueur peut utiliser le scanner (inv/achat).
     */
    bool canUseScanner(const QString& roomId, const QString& attacker) const;

    /**
     * \brief Décrémente l'inventaire scanner ou effectue l'achat si besoin.
     */
    void consumeScannerOrPay(const QString& roomId, const QString& attacker);

    /**
     * \brief Vérifie l'accès spectateur à une room.
     */
    bool checkSpectatorAccess(const QString& id_room, const QString& password);

    /**
     * \brief Room en phase de placement ?
     */
    QMap<QString, bool> roomInPlacement;

    /**
     * \brief roomId -> (playerName -> liste de bateaux soumis).
     */
    QMap<QString, QMap<QString, QVector<SubmittedBoat>>> pendingPlacements;
};

#endif // SERVER_H
