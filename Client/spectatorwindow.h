#ifndef SPECTATORWINDOW_H
#define SPECTATORWINDOW_H

/**
 * \file spectatorwindow.h
 * \brief Fenêtre de spectateur : observe deux grilles, chat et statut.
 * \ingroup client-ui
 */

#include <QMainWindow>
#include <QTcpSocket>
#include <QGridLayout>
#include <QLabel>
#include <QTextBrowser>
#include <QTextEdit>
#include <QPushButton>
#include <QMap>
#include "Clickablewidget.h"

/**
 * \class SpectatorWindow
 * \brief Permet de suivre une partie en lecture seule (avec chat).
 */
class SpectatorWindow : public QMainWindow
{
    Q_OBJECT
public:
    /**
     * \brief Construit la fenêtre spectateur.
     * \param roomId Id de room à observer.
     * \param socket Socket TCP vers le serveur.
     * \param spectatorName Pseudo du spectateur (pour le chat local).
     * \param parent Parent Qt.
     */
    explicit SpectatorWindow(const QString& roomId, QTcpSocket* socket, const QString& spectatorName, QWidget* parent=nullptr);
    ~SpectatorWindow();

signals:
    /// Émis quand la fenêtre est fermée/abandonnée.
    void spectateAborted();

private slots:
    /// Réception d’événements serveur.
    void onReadyRead();

    /// Fermeture locale de la fenêtre de spectateur.
    void onQuit();

    /// Envoi d’un message dans le chat.
    void onSendMessage();

private:
    QString roomId;            ///< Id de room suivi.
    QTcpSocket* socket = nullptr;

    QWidget* central = nullptr;
    QGridLayout *leftGrid = nullptr, *rightGrid = nullptr;
    QLabel *leftLabel = nullptr, *rightLabel = nullptr, *currentPlayerLabel = nullptr;
    QTextBrowser *chatView = nullptr;
    QTextEdit *chatEdit = nullptr;
    QPushButton *sendBtn = nullptr, *quitBtn = nullptr;

    QString player1, player2; ///< Noms des deux joueurs.
    /// Mapping joueur -> matrice de cases affichées.
    QMap<QString, std::vector<std::vector<Clickablewidget*>>> boards;

    /// Masque côté spectateur : seuls H/M sont visibles, sinon "X".
    QString maskForSpectator(const QString& cell) const;

    /// Construit une grille masquée pour un joueur.
    void buildGrid(const QString& who, int rows, int cols, const QStringList& cells, bool onLeft);

    /// Met à jour une cellule (H/M/…).
    void updateCellFor(const QString& owner, int r, int c, const QString& symbol);

    /// Retourne le layout de grille selon le joueur.
    QGridLayout* gridFor(const QString& who) const;

    QString spectatorName; ///< Pseudo local à afficher dans le chat.
};

#endif
