#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include "Clickablewidget.h"
#include <QPushButton>


namespace Ui {
class GameWindow;
}

class GameWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit GameWindow(const QString &roomId, QTcpSocket *socket, const QString& playerName,QWidget *parent = nullptr);
    void updateBoard(const QString& player, int row, int col, QString _case);
    void statusModification(const QString &currentPlayer, bool, const QString&);
    void connexionSocket();
    void onDisconnected();
    ~GameWindow();

private slots:
    void onElementClicked(int row, int col, bool isOpponentBoard = true);
    void onReadyRead();  // Ajouter cette ligne
    void onQuitRoom();
    void onSendMessage();
    void onClearButton();
    void on_powerReconnaisance_clicked();

    void on_powerMissile_clicked();

signals:
    void gameAborted();

private:
    Ui::GameWindow *ui;
    QString roomId;
    QTcpSocket *socket;
    QString playerName;
    bool isWinner;
    QString winnerPlayer;
    bool isReconnaisancePowerActive;
    std::vector<std::vector<Clickablewidget*>> myBoard; ///< Plateau de jeu graphique.
    std::vector<std::vector<Clickablewidget*>> opponentBoard;
    bool isMissilePowerActive = false;

    // --- Placement ---
    bool isPlacementPhase = false;
    QVector<int> boatsRemaining;
    int  currentPlacementSize = -1;
    bool placementHorizontal = true;
    QPushButton* rotateButton = nullptr;
    QPushButton* finishPlacementButton = nullptr;

    void setupEmptyBoards(int rows, int cols);
    void tryPlaceAt(int row, int col);
    void applyPlacementLocally(int size, int row, int col, bool horizontal);
    QString letterForSize(int size) const;


};

#endif // GAMEWINDOW_H
