#include "gamewindow.h"
#include "ui_gamewindow.h"
#include "QMessageBox"

GameWindow::GameWindow(const QString &_roomId, QTcpSocket *_socket, const QString& _playerName,QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::GameWindow),
    roomId(_roomId),
    socket(_socket),
    playerName(_playerName)
{
    ui->setupUi(this);

    this->isWinner= false;
    this->isReconnaisancePowerActive = false;
    ui->namePlayer->setText(_playerName);

    connect(ui->sendButton, &QPushButton::clicked, this, &GameWindow::onSendMessage);

    connect(ui->clearButton, &QPushButton::clicked, this, &GameWindow::onClearButton);

    connect(socket, &QTcpSocket::readyRead, this, &GameWindow::onReadyRead);

    connect(ui->actionRetour_2, &QAction::triggered, this, &GameWindow::onQuitRoom);

    connect(ui->actionExit, &QAction::triggered, this, &GameWindow::onDisconnected);

    socket->write("CREATE_GAME;" + roomId.toUtf8());
    socket->flush();
}

void GameWindow::onReadyRead() {

    //qDebug() << "Je suis dans onREQDYREAD";
    QByteArray data = socket->readAll();
    QString message = QString::fromUtf8(data);

    qDebug() << "Message reçu du serveur:" << message;

    QStringList messages = message.split('\n', Qt::SkipEmptyParts);

    for (const QString &msg : messages) {
        if(msg.startsWith("BOARD_CREATE;")){
            QStringList parts = msg.split(";");
            int width = parts[1].toInt();
            int height = parts[2].toInt();
            QString player1 = parts[3];
            QString player2 = parts[104];

            int i = 4;

            for (int row = 0; row < width; ++row) {
                std::vector<Clickablewidget*> line;
                for (int col = 0; col < height; ++col) {
                    QString caseElement = parts[i];
                    i++;
                    if (player1 == playerName) {
                        // MON plateau - NON attaquable
                        Clickablewidget *cardLabel = new Clickablewidget (caseElement,this);//
                        connect(cardLabel, &Clickablewidget::clicked, this, [this, row, col]() {
                            onElementClicked(row, col,false);
                        });
                        ui->gridCurrentBoard->addWidget(cardLabel, row, col);
                        ui->opponentLabel->setText(player2);
                        line.push_back(cardLabel);//
                    } else {
                        // PLATEAU ADVERSE - Attaquable
                        Clickablewidget *cardLabel1 = new Clickablewidget ("X",this);//
                        connect(cardLabel1, &Clickablewidget::clicked, this, [this, row, col]() {
                            onElementClicked(row, col,true);
                        });
                        ui->opponentLabel->setText(player1);
                        ui->gridOppositeBoard->addWidget(cardLabel1, row, col);
                        line.push_back(cardLabel1);//
                    }
                }
                // Ajout de la ligne complète à opponentBoard ou myBoard
                if (player1 == playerName) {
                    this->myBoard.push_back(line);
                } else {
                    this->opponentBoard.push_back(line);
                }
            }

            i++;

            for (int row1 = 0; row1 < width; ++row1) {
                std::vector<Clickablewidget*> line1;
                for (int col1 = 0; col1 < height; ++col1) {
                    QString caseElement1 = parts[i];
                    i++;
                    if (player1 == playerName) {
                        // PLATEAU ADVERSE - Attaquable
                        Clickablewidget *cardLabel2 = new Clickablewidget ("X",this);//
                        connect(cardLabel2, &Clickablewidget::clicked, this, [this, row1, col1]() {
                            onElementClicked(row1, col1,true);
                        });
                        ui->gridOppositeBoard->addWidget(cardLabel2, row1, col1);
                        line1.push_back(cardLabel2);//
                    } else {
                        // MON plateau - NON attaquable
                        Clickablewidget *cardLabel3 = new Clickablewidget (caseElement1,this);//
                        connect(cardLabel3, &Clickablewidget::clicked, this, [this, row1, col1]() {
                            onElementClicked(row1, col1,false);
                        });
                        ui->gridCurrentBoard->addWidget(cardLabel3, row1, col1);
                        line1.push_back(cardLabel3);
                    }
                }
                // Ajout de la ligne complète à opponentBoard ou myBoard
                if (player1 == playerName) {
                    this->opponentBoard.push_back(line1);
                } else {
                    this->myBoard.push_back(line1);
                }
            }
        }
        else if(msg.startsWith("STATUS;")){
            QStringList parts = msg.split(";");
            QString currentPlayer = parts[1];
            this->isWinner = parts[2].toInt();
            this->winnerPlayer = parts[3];
            statusModification(currentPlayer, this->isWinner, this->winnerPlayer);
        }
        else if(msg.startsWith("UPDATE_CASE")){
            QStringList parts = msg.split(";");
            QString oppositePlayer = parts[1];
            int row = parts[2].toInt();
            int col = parts[3].toInt();
            QString _case = parts[4];
            updateBoard(oppositePlayer,row,col,_case);
        }
        else if(msg.startsWith("CLIENT_LEFT_ROOM;")){
            QMessageBox::information(this, "Retour à l'accueil", "Un joueur a quitté la salle. Vous allez être redirigé.");
            emit gameAborted();  // Émet un signal pour informer StartWindow
            this->close();       // Ferme la GameWindow
        }
        else if (msg.startsWith("SEND_CHAT_MESSAGE;")) {
            QStringList parts = message.split(";");
            QString playerMessage = parts[1];        // Nom du joueur qui a envoyé le message
            QString chatMessage = parts[2];   // Contenu du message

            // Afficher le message dans le QTextBrowser
            QString formattedMessage = playerMessage + ": " + chatMessage;
            ui->messageIncomeBox->append(formattedMessage);
        }
        else if(msg.startsWith("RECONNAISANCE_RESULT;")){
            QStringList parts = message.split(";");
            int nbBateaux = parts[1].toInt();
            QString formattedMessage = "Resultat Reconnaisance : " + QString::number(nbBateaux) + " Bateaux sont presents dans la zone";
            ui->messageIncomeBox->append(formattedMessage);
        }
    }
}

void GameWindow::connexionSocket() {
    qDebug() << "Tentative de connexion...";
    if (socket->state() == QAbstractSocket::ConnectedState) {
        qDebug() << "Connexion réussie";
    } else {
        qDebug() << "Échec de la connexion";
    }
}

void GameWindow::onDisconnected() {
    QApplication::quit();
    qDebug() << "Déconnexion du client";
}

void GameWindow::onQuitRoom() {
    QString quitMessage = QString("QUIT_ROOM;%1").arg(roomId);
    socket->write(quitMessage.toUtf8());  // Notifie le serveur
    emit gameAborted();
    this->close();  // Ferme l'interface
}

void GameWindow::updateBoard(const QString& oppositePlayer, int row, int col, QString _case)
{
    if(oppositePlayer != this->playerName){
        Clickablewidget *cardLabel = new Clickablewidget (_case,this);
        ui->gridOppositeBoard->addWidget(cardLabel, row, col);
        this->opponentBoard[row][col] = cardLabel;
    }
    else{
        Clickablewidget *cardLabel1 = new Clickablewidget (_case,this);
        ui->gridCurrentBoard->addWidget(cardLabel1, row, col);
        this->myBoard[row][col] = cardLabel1;
    }
}

void GameWindow::statusModification(const QString& currentPlayer, bool isGameOver, const QString& playerWinner){
    ui->currentPlayerLabel->setText(currentPlayer);
    bool isMyTurn = (currentPlayer == this->playerName);
    if(!isGameOver){
        ui->powerReconnaisance->setEnabled(isMyTurn);
        ui->powerMissile->setEnabled(isMyTurn);
        if(isMyTurn){
            ui->opponentLabel->setStyleSheet("background-color: gold;");
            for (int row = 0; row < this->opponentBoard.size(); ++row) {
                for (int col = 0; col < this->opponentBoard[row].size(); ++col) {
                    this->opponentBoard[row][col]->blockSignals(false);
                }
            }
        }
        else{
            ui->opponentLabel->setStyleSheet("background-color: none;");
            for (int row = 0; row < this->opponentBoard.size(); ++row) {
                for (int col = 0; col < this->opponentBoard[row].size(); ++col) {
                    this->opponentBoard[row][col]->blockSignals(true);
                }
            }
        }
    }
    else{
        for (std::vector<Clickablewidget*> row : this->myBoard) {
            for (Clickablewidget*  _element: row) {
                _element->blockSignals(true);
            }
        }
        if(playerWinner == this->playerName){
            for (std::vector<Clickablewidget*> row : this->opponentBoard) {
                for (Clickablewidget*  _element: row) {
                    _element->blockSignals(true);
                }
            }

            QMessageBox::information(this,"Victorire","Vous avez Gagné");
        }
        else{
            QMessageBox::information(this,"Defaite","Vous avez perdu");
        }
    }
}

void GameWindow::onElementClicked(int row, int col, bool isOpponentBoard)
{
    qDebug("Clicked");

    // Vérifier qu'on peut jouer
    if (this->playerName != ui->currentPlayerLabel->text()) {
        qDebug() << "Ce n'est pas votre tour !";
        return;
    }

    // Empêcher l'attaque de son propre plateau
    if (!isOpponentBoard) {
        qDebug() << "Vous ne pouvez pas attaquer votre propre plateau !";
        ui->messageIncomeBox->append("Vous ne pouvez pas attaquer votre propre plateau !");
        return;
    }

    // Vérifier que la case n'a pas déjà été attaquée
    if (row < this->opponentBoard.size() && col < this->opponentBoard[row].size()) {
        Clickablewidget* targetWidget = this->opponentBoard[row][col];
        if (targetWidget && (targetWidget->getCase() == "H" || targetWidget->getCase() == "M")) {
            qDebug() << "Cette case a déjà été attaquée !";
            ui->messageIncomeBox->append("Cette case a déjà été attaquée !");
            return;
        }
    }

    // Votre code existant (inchangé)
    if(this->isReconnaisancePowerActive){
        QString message = "RECONNAISANCE;" + this->roomId + ";" + QString::number(row) + ";" + QString::number(col) + "\n";
        socket->write(message.toUtf8());
        this->isReconnaisancePowerActive = false;
    }
    else{
        QString message = "ATTACK;" + this->roomId + ";" + QString::number(row) + ";" + QString::number(col) ;
        socket->write(message.toUtf8());
    }
}

void GameWindow::onSendMessage(){
    QString message = ui->boxText->toPlainText().trimmed();  // Récupérer le texte saisi

    if (message.isEmpty()) {
        return;  // Ne rien faire si le message est vide
    }

    QString formattedMessage = playerName + ": " + message;  // Ajouter le nom du joueur

    // Envoyer le message au serveur via le socket
    socket->write(QString("CHAT_MESSAGE;%1;%2").arg(roomId, message).toUtf8());
    socket->flush();

    // Afficher le message localement dans le QTextBrowser
    ui->messageIncomeBox->append(formattedMessage);

    // Vider le QTextEdit après l'envoi
    ui->boxText->clear();
}

void GameWindow::onClearButton(){
    ui->boxText->clear();
}

GameWindow::~GameWindow()
{
    delete ui;
}

void GameWindow::on_powerReconnaisance_clicked()
{
    this->isReconnaisancePowerActive = true;
}

