#ifndef PLACEMENTWINDOW_H
#define PLACEMENTWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QMap>
#include "Clickablewidget.h"
#include "gamewindow.h"

class PlacementWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit PlacementWindow(const QString& roomId,
                             const QString& playerName,
                             QTcpSocket* socket,
                             int rows, int cols,
                             const QString& p1, const QString& p2,
                             bool isSolo,
                             QWidget* parent=nullptr);
    ~PlacementWindow();

signals:
    void placementAborted();

private slots:
    void onCellClicked(int r, int c);
    void onRotate();
    void onValidate();
    void onReadyRead();
    void onQuit();

private:
    struct BoatSpec { int size; QString code; int count; };
    struct PlacedBoat { int size; int row; int col; bool horizontal; };

    QString roomId, playerName, player1, player2;
    bool isSoloMode;
    QTcpSocket* socket;

    int R, C;
    QWidget* central;
    QGridLayout* grid;
    QLabel* infoLabel;
    QPushButton* rotateBtn;
    QPushButton* validateBtn;
    QPushButton* quitBtn;

    // Grille de widgets
    std::vector<std::vector<Clickablewidget*>> cells;

    // Inventaire de bateaux à placer (D2, S3×2, C4, P5)
    QVector<BoatSpec> bag = {
        {5,"P",1}, {4,"C",1}, {3,"S",2}, {2,"D",1}
    };

    // État courant
    QString selectedCode = "P";
    int selectedSize = 5;
    bool horizontal = true; // orientation

    // Placés
    QVector<PlacedBoat> placed;

    bool canPlaceAt(int row, int col, int size, bool H) const;
    void paintBoat(int row, int col, int size, bool H, const QString& code);
    void unpaintBoat(int row, int col, int size, bool H);

    void pickNextSelectable();
    int remainingOfSize(int size) const;
};

#endif // PLACEMENTWINDOW_H
