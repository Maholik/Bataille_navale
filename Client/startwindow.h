#ifndef STARTWINDOW_H
#define STARTWINDOW_H

/**
 * \file startwindow.h
 * \brief Écran d’accueil : liste des salles, création/join/spectate/solo.
 * \ingroup client-ui
 */

#include <QMainWindow>
#include <QTcpSocket>
#include <QMessageBox>

namespace Ui { class StartWindow; }

/**
 * \class StartWindow
 * \brief Point d’entrée du client : gère la connexion serveur et la navigation.
 */
class StartWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit StartWindow(QWidget *parent = nullptr);
    ~StartWindow();

private slots:
    /// Création d’une salle (dialogue).
    void on_btnCreate_clicked();

    /// Rejoint une salle existante (dialogue).
    void on_btnJoin_clicked();

    /// Gestion de tous les messages serveur (ROOM_LIST, erreurs, etc.).
    void onReadyRead();

    /// Quitte l’application.
    void on_btnExit_clicked();

    /// Démarre une partie solo vs IA.
    void on_iaButton_clicked();

    /// Ouvre le mode spectateur.
    void on_btnSpectate_clicked();

private:
    Ui::StartWindow *ui = nullptr; ///< UI designer.
    QTcpSocket *socket = nullptr;  ///< Socket TCP partagé avec les vues.
    QString roomId;                ///< Room courante (si join).
    QString playerName;            ///< Pseudo local.
    QMessageBox *waitingMessageBox = nullptr; ///< Popup d’attente de joueur.

    /// Met à jour la table des salles à partir du flux serveur.
    void updateRoomList(const QString &roomInfo);

    /// Affiche/masque la popup d’attente.
    void showWaitingPopup();

    /// Ouvre la vue de jeu après validation serveur.
    void showGameViewWidget(const QString &roomId);

    /// Pseudo mémorisé pour le spectateur.
    QString spectatorName;
};

#endif // STARTWINDOW_H
