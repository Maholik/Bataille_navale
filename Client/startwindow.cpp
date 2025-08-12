#include "startwindow.h"
#include "ui_startwindow.h"
#include "gamewindow.h"
#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDebug>
#include <QLabel>
#include <QInputDialog>
#include "spectatorwindow.h"


StartWindow::StartWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::StartWindow)
{
    ui->setupUi(this);

    ui->tableRooms->setColumnCount(4);
    ui->tableRooms->setHorizontalHeaderLabels(QStringList() << "Id" << "Nom" << "Privé ou publique" << "Nombre de clients");

    // Créer un nouveau QMessageBox et le stocker dans waitingMessageBox
    waitingMessageBox = new QMessageBox(this);
    waitingMessageBox->setWindowTitle("WAITING...");
    waitingMessageBox->setText("Waiting for 2 player...");
    waitingMessageBox->setStandardButtons(QMessageBox::Cancel);
    waitingMessageBox->setDefaultButton(QMessageBox::Cancel);

    // Désactiver la croix de fermeture
    waitingMessageBox->setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    // Connecter le bouton "Annuler" pour quitter la salle
    connect(waitingMessageBox, &QMessageBox::buttonClicked, this, [this](QAbstractButton *button) {
        if (waitingMessageBox->button(QMessageBox::Cancel) == button) {
            socket->write(QString("QUIT_ROOM;%1").arg(roomId).toUtf8());
            socket->flush();
            waitingMessageBox->close();  // Ferme le pop-up
        }
    });

    // Initialiser le QTcpSocket pour se connecter au serveur
    socket = new QTcpSocket(this);

    // Connexion au serveur sur localhost et port 8081
    socket->connectToHost("127.0.0.1", 12344);

    // Vérifier si la connexion est établie
    if (!socket->waitForConnected()) {
        QMessageBox::critical(this, "Erreur de connexion", "Impossible de se connecter au serveur : " + socket->errorString());
    } else {
        qDebug() << "Connexion au serveur réussie!";
    }

    // Connexion des signaux et slots
    connect(socket, &QTcpSocket::readyRead, this, &StartWindow::onReadyRead);
    connect(socket, &QTcpSocket::errorOccurred, this, [](QAbstractSocket::SocketError socketError){
        qDebug() << "Erreur de socket: " << socketError;
    });

    // Demander la liste des salles existantes
    socket->write("GET_ROOMS");
    socket->flush();
}

StartWindow::~StartWindow()
{
    delete ui;
    delete socket;
}

void StartWindow::on_btnCreate_clicked()
{
    // Créer un QDialog pour créer une salle
    QDialog dialog(this);
    dialog.setWindowTitle("Créer une salle");

    // Créer les widgets pour le dialogue
    QLineEdit *lineEditRoomName = new QLineEdit(&dialog);
    QCheckBox *checkBoxIsPublic = new QCheckBox("Room Privé", &dialog);
    QPushButton *btnSubmit = new QPushButton("Créer", &dialog);

    // Créer le champ mot de passe, il est initialement caché
    QLineEdit *lineEditPassword = new QLineEdit(&dialog);
    lineEditPassword->setPlaceholderText("Mot de passe");
    lineEditPassword->setEchoMode(QLineEdit::Password);  // Cacher les caractères
    lineEditPassword->setEnabled(false);  // Désactiver par défaut

    // Disposer les widgets dans un layout vertical
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(lineEditRoomName);
    layout->addWidget(checkBoxIsPublic);
    layout->addWidget(lineEditPassword);
    layout->addWidget(btnSubmit);
    dialog.setLayout(layout);

    // Connecter le bouton "Créer" pour fermer le dialogue
    connect(btnSubmit, &QPushButton::clicked, &dialog, &QDialog::accept);

    // Lorsque l'état du checkbox change, activer/désactiver le champ de mot de passe
    connect(checkBoxIsPublic, &QCheckBox::toggled, [lineEditPassword](bool checked){
        if (checked) {
            lineEditPassword->setEnabled(true);  // Si public, désactiver mot de passe
        } else {
            lineEditPassword->setEnabled(false);   // Si privé, activer mot de passe
        }
    });

    // Afficher le dialogue
    if (dialog.exec() == QDialog::Accepted) {
        // Récupérer les informations saisies
        QString roomName = lineEditRoomName->text();
        bool isPrivate = checkBoxIsPublic->isChecked();
        QString password = isPrivate ? lineEditPassword->text() : "";  // Si la salle est privée, récupérer le mot de passe

        // Vérifier que le nom de la salle n'est pas vide
        if (roomName.isEmpty()) {
            qDebug() << "Erreur: Le nom de la salle ne peut pas être vide!";
            return;
        }

        // Logique pour envoyer les informations au serveur
        QString data = "CREATE_ROOM;" + roomName + ";" + (isPrivate ? "privée" : "publique");

        if (!password.isEmpty()) {
            data += ";" + password;  // Ajouter le mot de passe si la salle est privée
        }

        if (socket->state() == QAbstractSocket::ConnectedState) {
            socket->write(data.toUtf8());
            socket->flush();
            qDebug() << "Données envoyées au serveur: " << data;
        } else {
            qDebug() << "Erreur: Connexion au serveur non établie.";
        }
    }
}

void StartWindow::on_btnJoin_clicked()
{
    // Création de la boîte de dialogue
    QDialog dialog(this);
    dialog.setWindowTitle("Rejoindre une salle");
    QLineEdit *lineEditPlayerName = new QLineEdit(&dialog);
    lineEditPlayerName->setPlaceholderText("Nom Joueur");
    QLineEdit *lineEditRoomId = new QLineEdit(&dialog);
    lineEditRoomId->setPlaceholderText("ID de la salle");
    QLineEdit *lineEditPassword = new QLineEdit(&dialog);
    lineEditPassword->setPlaceholderText("Mot de passe");
    lineEditPassword->setEchoMode(QLineEdit::Password);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(lineEditPlayerName);
    layout->addWidget(lineEditRoomId);
    layout->addWidget(lineEditPassword);
    QPushButton *btnJoin = new QPushButton("Rejoindre", &dialog);
    layout->addWidget(btnJoin);
    dialog.setLayout(layout);

    connect(btnJoin, &QPushButton::clicked, &dialog, &QDialog::accept);

    // Affichage de la boîte de dialogue et récupération des données
    if (dialog.exec() == QDialog::Accepted) {
        QString roomId = lineEditRoomId->text();
        QString password = lineEditPassword->text();
        QString _playerName = lineEditPlayerName->text();
        this->playerName = _playerName;

        if (roomId.isEmpty()) {
            QMessageBox::warning(this, "Erreur", "L'identifiant de la salle est requis.");
            return;
        }
        if(_playerName.isEmpty()){
            QMessageBox::warning(this, "Erreur", "Le nom du joueur est requis.");
        }

        // Envoi de la requête au serveur
        QString request = QString("JOIN_ROOM;%1;%2;%3").arg(_playerName,roomId, password);
        socket->write(request.toUtf8());
        qDebug() << "Requête envoyée au serveur : " << request;
    }
}

void StartWindow::onReadyRead()
{
    QByteArray response = socket->readAll();
    QString all = QString::fromUtf8(response);
    qDebug() << "Message(s) reçu(s) :" << all;

        const QStringList lines = all.split('\n', Qt::SkipEmptyParts);
    for (QString message : lines) {
        message = message.trimmed();

        if (message.startsWith("ROOM_LIST:")) {
            updateRoomList(message);
        }
        else if (message.startsWith("ROOM_JOINED")) {
            showWaitingPopup();
        }
        else if (message.startsWith("ROOM_READY;")) {
            QString roomId = message.split(';', Qt::SkipEmptyParts).value(1);
            if (waitingMessageBox) waitingMessageBox->close();
            showGameViewWidget(roomId);
        }
        else if (message.startsWith("ERR_NAME_TAKEN")) {
            QMessageBox::warning(this, "Nom déjà pris", "Ce nom est déjà utilisé dans cette salle.");
        }
        else if (message.startsWith("ERR_INVALID_PASSWORD_OR_ROOM_NOT_FOUND")) {
            QMessageBox::warning(this, "Erreur", "Salle introuvable ou mot de passe invalide.");
        }

        else if (message.startsWith("SPECTATE_OK;")) {
            QString roomId = message.split(';', Qt::SkipEmptyParts).value(1);
            // ouvrir la fenêtre spectateur
            disconnect(socket, &QTcpSocket::readyRead, this, &StartWindow::onReadyRead);
            // NEW: on passe spectatorName
            SpectatorWindow* w = new SpectatorWindow(roomId, socket, this->spectatorName, this);
            connect(w, &SpectatorWindow::spectateAborted, this, [this](){
                this->show();
                connect(socket, &QTcpSocket::readyRead, this, &StartWindow::onReadyRead);
            });
            w->show();
            this->hide();
        }

        else {
            qDebug() << "Message non reconnu :" << message;
        }
    }

}


void StartWindow::updateRoomList(const QString &roomInfo)
{
    qDebug() << "Mise à jour de la liste des rooms avec: " << roomInfo;

    // Vider la table avant de la remplir avec les nouvelles données
    ui->tableRooms->setRowCount(0);

    // Extraire les données après "ROOM_LIST:" (on retire ce préfixe)
    QStringList roomsData = roomInfo.mid(10).split("\n");

    // Vérifier qu'il y a suffisamment d'éléments pour traiter les salles
    for (const QString &roomData : roomsData) {
        QStringList roomFields = roomData.split(";");

        // Vérifier que nous avons les 4 éléments nécessaires pour remplir la table
        if (roomFields.size() == 4) {

            QString roomId = roomFields[0].trimmed();         // ID de la salle
            QString roomName = roomFields[1].trimmed();       // Nom de la salle
            QString roomType = roomFields[2].trimmed();       // Type de la salle (privée/publique)
            QString clientCount = roomFields[3].trimmed();    // Nombre de clients dans la salle

            // Ajouter une nouvelle ligne à la table
            int row = ui->tableRooms->rowCount();
            ui->tableRooms->insertRow(row);

            // Remplir la table avec les données de la salle
            ui->tableRooms->setItem(row, 0, new QTableWidgetItem(roomId));       // ID de la salle
            ui->tableRooms->setItem(row, 1, new QTableWidgetItem(roomName));     // Nom de la salle
            ui->tableRooms->setItem(row, 2, new QTableWidgetItem(roomType));     // Type de la salle (privée/publique)
            ui->tableRooms->setItem(row, 3, new QTableWidgetItem(clientCount));  // Nombre de clients dans la salle
        }
    }

    ui->tableRooms->repaint();  // Forcer la mise à jour de l'affichage
}

void StartWindow::showGameViewWidget(const QString &roomId)
{
    // Déconnecte temporairement readyRead dans StartWindow
    disconnect(socket, &QTcpSocket::readyRead, this, &StartWindow::onReadyRead);

    // Ouvre la fenêtre GameWindow
    GameWindow* gameWindow = new GameWindow(roomId, socket, playerName, this);

    // Lorsque GameWindow est fermé, reconnecte readyRead à StartWindow
    connect(gameWindow, &GameWindow::gameAborted, this, [this]() {
        this->show();
        connect(socket, &QTcpSocket::readyRead, this, &StartWindow::onReadyRead);
    });

    gameWindow->show();
    this->hide();
}

void StartWindow::showWaitingPopup() {
    waitingMessageBox->show();
}


void StartWindow::on_btnExit_clicked()
{
    QApplication::quit();
}



void StartWindow::on_iaButton_clicked()
{
    if (playerName.trimmed().isEmpty()) playerName = "Joueur";
    socket->write(QString("START_SOLO;%1\n").arg(playerName).toUtf8());
    socket->flush();
}



void StartWindow::on_btnSpectate_clicked()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Spectate une salle");

    QLineEdit *lineEditName = new QLineEdit(&dialog);
    lineEditName->setPlaceholderText("Votre pseudo (spectateur)");

    QLineEdit *lineEditRoomId = new QLineEdit(&dialog);
    lineEditRoomId->setPlaceholderText("ID de la salle");

    QLineEdit *lineEditPassword = new QLineEdit(&dialog);
    lineEditPassword->setPlaceholderText("Mot de passe (si privé)");
        lineEditPassword->setEchoMode(QLineEdit::Password);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(lineEditName);
    layout->addWidget(lineEditRoomId);
    layout->addWidget(lineEditPassword);
    QPushButton *btnGo = new QPushButton("Spectate", &dialog);
    layout->addWidget(btnGo);
    dialog.setLayout(layout);
    connect(btnGo, &QPushButton::clicked, &dialog, &QDialog::accept);

    if (dialog.exec() == QDialog::Accepted) {
        QString nick = lineEditName->text().trimmed();
        QString rid  = lineEditRoomId->text().trimmed();
        QString pwd  = lineEditPassword->text();

        if (nick.isEmpty() || rid.isEmpty()) {
            QMessageBox::warning(this, "Erreur", "Pseudo et ID Room requis.");
            return;
        }

        // NEW: mémoriser le pseudo du spectateur
        this->spectatorName = nick;

        socket->write(QString("SPECTATE_ROOM;%1;%2;%3\n").arg(nick, rid, pwd).toUtf8());
        socket->flush();
    }

}

