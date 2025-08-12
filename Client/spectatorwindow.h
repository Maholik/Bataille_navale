#ifndef SPECTATORWINDOW_H
#define SPECTATORWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QGridLayout>
#include <QLabel>
#include <QTextBrowser>
#include <QTextEdit>
#include <QPushButton>
#include <QMap>
#include "Clickablewidget.h"

class SpectatorWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit SpectatorWindow(const QString& roomId, QTcpSocket* socket, QWidget* parent=nullptr);
    ~SpectatorWindow();

signals:
    void spectateAborted();

private slots:
    void onReadyRead();
    void onQuit();
    void onSendMessage();

private:
    QString roomId;
    QTcpSocket* socket;

    QWidget* central;
    QGridLayout *leftGrid, *rightGrid;
    QLabel *leftLabel, *rightLabel, *currentPlayerLabel;
    QTextBrowser *chatView;
    QTextEdit *chatEdit;
    QPushButton *sendBtn, *quitBtn;

    QString player1, player2; // noms
    // mapping par nom -> matrice de widgets
    QMap<QString, std::vector<std::vector<Clickablewidget*>>> boards;

    QString maskForSpectator(const QString& cell) const; // H/M visibles, le reste "X"
    void buildGrid(const QString& who, int rows, int cols, const QStringList& cells, bool onLeft);
    void updateCellFor(const QString& owner, int r, int c, const QString& symbol);
    QGridLayout* gridFor(const QString& who) const;
};

#endif
