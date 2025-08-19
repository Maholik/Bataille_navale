#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

/**
 * \file gamewindow.h
 * \brief Fenêtre principale de partie (plateaux, chat, pouvoirs).
 * \ingroup client-ui
 */

#include <QMainWindow>
#include <QTcpSocket>
#include "Clickablewidget.h"

namespace Ui { class GameWindow; }

/**
 * \class GameWindow
 * \brief Gère l'interface de jeu : affichage des grilles, tours, chat, pouvoirs.
 */
class GameWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * \brief Construit la fenêtre de jeu.
     * \param roomId Identifiant de la salle.
     * \param socket Socket TCP connecté au serveur.
     * \param playerName Pseudo du joueur local.
     * \param parent Parent Qt.
     */
    explicit GameWindow(const QString &roomId, QTcpSocket *socket, const QString& playerName,QWidget *parent = nullptr);

    /**
     * \brief Met à jour une case d'un des plateaux.
     * \param player Propriétaire du plateau à mettre à jour.
     * \param row Ligne.
     * \param col Colonne.
     * \param _case Nouveau code (H/M/E/D/S/C/P/X).
     */
    void updateBoard(const QString& player, int row, int col, QString _case);

    /**
     * \brief Met à jour l'état de tour et gère la fin de partie.
     * \param currentPlayer Joueur dont c'est le tour.
     * \param isGameOver Vrai si la partie est terminée.
     * \param playerWinner Nom du vainqueur, si applicable.
     */
    void statusModification(const QString &currentPlayer, bool isGameOver, const QString& playerWinner);

    /// Vérifie la connexion (log/debug).
    void connexionSocket();

    /// Quitte l'application proprement.
    void onDisconnected();

    ~GameWindow();

private slots:
    /**
     * \brief Clic sur une case d'un plateau.
     * \param row Ligne.
     * \param col Colonne.
     * \param isOpponentBoard Vrai si la case appartient au plateau adverse.
     */
    void onElementClicked(int row, int col, bool isOpponentBoard = true);

    /// Lecture de messages serveur (événements, chat, etc.).
    void onReadyRead();

    /// Quitte la salle courante (menu).
    void onQuitRoom();

    /// Envoie un message dans le chat.
    void onSendMessage();

    /// Vide la zone de saisie de chat.
    void onClearButton();

    /// Active le pouvoir "Reconnaissance".
    void on_powerReconnaisance_clicked();

    /// Active le pouvoir "Missile".
    void on_powerMissile_clicked();

signals:
    /// Émis si la partie est interrompue (retour à l'accueil).
    void gameAborted();

private:
    Ui::GameWindow *ui; ///< UI générée par Qt Designer.
    QString roomId;     ///< Identifiant de la salle.
    QTcpSocket *socket; ///< Socket TCP vers le serveur.
    QString playerName; ///< Nom du joueur local.

    bool isWinner = false;         ///< Indique si le joueur local a gagné.
    QString winnerPlayer;          ///< Nom du vainqueur.
    bool isReconnaisancePowerActive = false; ///< État du pouvoir reconnaissance.
    std::vector<std::vector<Clickablewidget*>> myBoard; ///< Plateau du joueur local.
    std::vector<std::vector<Clickablewidget*>> opponentBoard; ///< Plateau adverse.
    bool isMissilePowerActive = false; ///< État du pouvoir missile.
};

#endif // GAMEWINDOW_H
