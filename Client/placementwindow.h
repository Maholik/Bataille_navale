#ifndef PLACEMENTWINDOW_H
#define PLACEMENTWINDOW_H

/**
 * \file placementwindow.h
 * \brief Fenêtre de placement de la flotte avant la bataille.
 * \ingroup client-ui
 */

#include <QMainWindow>
#include <QTcpSocket>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QMap>
#include "Clickablewidget.h"
#include "gamewindow.h"

/**
 * \class PlacementWindow
 * \brief Permet au joueur de placer ses bateaux puis de valider son plateau.
 */
class PlacementWindow : public QMainWindow
{
    Q_OBJECT
public:
    /**
     * \brief Construit la fenêtre de placement.
     * \param roomId Id de room.
     * \param playerName Pseudo local.
     * \param socket Socket vers le serveur.
     * \param rows Nb de lignes.
     * \param cols Nb de colonnes.
     * \param p1 Nom du joueur 1.
     * \param p2 Nom du joueur 2.
     * \param isSolo Vrai si partie solo vs IA.
     * \param parent Parent Qt.
     */
    explicit PlacementWindow(const QString& roomId,
                             const QString& playerName,
                             QTcpSocket* socket,
                             int rows, int cols,
                             const QString& p1, const QString& p2,
                             bool isSolo,
                             QWidget* parent=nullptr);
    ~PlacementWindow();

signals:
    /// Émis si l'utilisateur quitte la phase de placement.
    void placementAborted();

private slots:
    /// Clique sur une case de la grille de placement.
    void onCellClicked(int r, int c);

    /// Bascule l'orientation (horiz/vert).
    void onRotate();

    /// Valide l'ensemble du plateau (envoi au serveur).
    void onValidate();

    /// Réception d’événements serveur (erreurs, passage en GameWindow, ...).
    void onReadyRead();

    /// Bouton quitter.
    void onQuit();

private:
    struct BoatSpec { int size; QString code; int count; };   ///< Inventaire à placer.
    struct PlacedBoat { int size; int row; int col; bool horizontal; }; ///< Bateaux posés.

    // Contexte
    QString roomId, playerName, player1, player2;
    bool isSoloMode = false;
    QTcpSocket* socket = nullptr;

    // UI
    int R = 0, C = 0;
    QWidget* central = nullptr;
    QGridLayout* grid = nullptr;
    QLabel* infoLabel = nullptr;
    QPushButton* rotateBtn = nullptr;
    QPushButton* validateBtn = nullptr;
    QPushButton* quitBtn = nullptr;

    // Grille de widgets
    std::vector<std::vector<Clickablewidget*>> cells;

    // Inventaire (D2, S3×2, C4, P5)
    QVector<BoatSpec> bag = { {5,"P",1}, {4,"C",1}, {3,"S",2}, {2,"D",1} };

    // État courant
    QString selectedCode = "P";
    int selectedSize = 5;
    bool horizontal = true;

    // Posés
    QVector<PlacedBoat> placed;

    // Helpers de placement
    bool canPlaceAt(int row, int col, int size, bool H) const;
    void paintBoat(int row, int col, int size, bool H, const QString& code);
    void unpaintBoat(int row, int col, int size, bool H);

    // Sélection automatique suivante
    void pickNextSelectable();
    int remainingOfSize(int size) const;
};

#endif // PLACEMENTWINDOW_H
