#ifndef STARTWINDOW_H
#define STARTWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QMessageBox>

namespace Ui {
class StartWindow;
}

class StartWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit StartWindow(QWidget *parent = nullptr);
    ~StartWindow();

private slots:
    void on_btnCreate_clicked();
    void on_btnJoin_clicked();
    void onReadyRead();
    void on_btnExit_clicked();

    void on_iaButton_clicked();

private:
    Ui::StartWindow *ui;
    QTcpSocket *socket;
    QString roomId;
    QString playerName;
    QMessageBox *waitingMessageBox = nullptr;

    void updateRoomList(const QString &roomInfo);
    void showWaitingPopup();
    void showGameViewWidget(const QString &roomId); // Garde seulement cette version
};

#endif // STARTWINDOW_H
