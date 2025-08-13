#ifndef PLACEMENTWINDOW_H
#define PLACEMENTWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QTextBrowser>
#include <vector>
#include "Clickablewidget.h"

class PlacementWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit PlacementWindow(const QString& roomId,
                             const QString& playerName,
                             QTcpSocket* socket,
                             QWidget* parent=nullptr);

signals:
    void placementFinished(); // pour enchaîner vers GameWindow

private slots:
    void onReadyRead();
    void onCellClicked(int r, int c);
    void onToggleOrientation();
    void onFinish();

private:
    QString roomId, playerName;
    QTcpSocket* socket;

    QWidget* central;
    QGridLayout* grid;
    QLabel* infoLabel;
    QPushButton* orientBtn;
    QPushButton* finishBtn;
    QTextBrowser* logView;

    bool horizontal = true;
    std::vector<std::vector<Clickablewidget*>> myGrid; // 10x10

    // flotte à poser (côté client) -> on suit l’ordre
    QList<int> todo = {5,4,3,3,2};

    void buildEmptyGrid(int rows=10, int cols=10);
    void paintBoat(int size, int row, int col, bool horizontal);
    void log(const QString& line);
};

#endif
