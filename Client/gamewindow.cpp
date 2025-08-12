#include "gamewindow.h"
#include "ui_gamewindow.h"
#include "QMessageBox"
#include <QRegularExpression> // add this at the top


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
        if (msg.startsWith("BOARD_CREATE;")) {
            // Split on ';' or '\n'
            QStringList t = msg.split(QRegularExpression("[;\n]"), Qt::SkipEmptyParts);

            // Expected layout:
            // [0]="BOARD_CREATE", [1]=W, [2]=H, [3]=player1,
            // [4.. 4+W*H-1]=board1, [4+W*H]=player2, then [..]=board2 (W*H cells)
            if (t.size() < 4) { qWarning() << "BOARD_CREATE truncated"; continue; }

            const int W = t[1].toInt();
            const int H = t[2].toInt();
            const QString p1 = t[3].trimmed();

            const int cells = W * H;
            const int idxBoard1Start = 4;
            const int idxBoard1End   = idxBoard1Start + cells - 1;
            const int idxPlayer2     = idxBoard1End + 1;
            const int idxBoard2Start = idxPlayer2 + 1;
            const int idxBoard2End   = idxBoard2Start + cells - 1;

            if (t.size() <= idxBoard2End) {
                qWarning() << "BOARD_CREATE wrong count, got" << t.size() << "need" << (idxBoard2End+1);
                continue;
            }

            const QString p2 = t[idxPlayer2].trimmed();

            // Build grids
            auto buildGrid = [&](int startIdx, bool mine) {
                int k = startIdx;
                for (int r = 0; r < W; ++r) {
                    std::vector<Clickablewidget*> line;
                    for (int c = 0; c < H; ++c, ++k) {
                        const QString cell = t[k];
                        Clickablewidget* w = nullptr;
                        if (mine) {
                            w = new Clickablewidget(cell, this);
                            connect(w, &Clickablewidget::clicked, this, [this,r,c](){ onElementClicked(r,c,false); });
                            ui->gridCurrentBoard->addWidget(w, r, c);
                        } else {
                            w = new Clickablewidget("X", this);
                            connect(w, &Clickablewidget::clicked, this, [this,r,c](){ onElementClicked(r,c,true); });
                            ui->gridOppositeBoard->addWidget(w, r, c);
                        }
                        line.push_back(w);
                    }
                    if (mine) myBoard.push_back(line); else opponentBoard.push_back(line);
                }
            };

            // Decide who is me
            if (p1 == playerName) {
                ui->opponentLabel->setText(p2);
                buildGrid(idxBoard1Start, /*mine=*/true);
                buildGrid(idxBoard2Start, /*mine=*/false);
            } else {
                ui->opponentLabel->setText(p1);
                buildGrid(idxBoard1Start, /*mine=*/false);
                buildGrid(idxBoard2Start, /*mine=*/true);
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

        else if (msg.startsWith("POWER_AVAILABLE;")) {
            auto parts = msg.split(';', Qt::SkipEmptyParts);
            if (parts.size() >= 3) {
                QString who = parts[1];
                QString what = parts[2];
                if (who == this->playerName) {
                    ui->messageIncomeBox->append("Nouveau pouvoir: " + what);
                }
            }
        }
        else if (msg.startsWith("SCORE_UPDATE;")) {
            auto parts = msg.split(';', Qt::SkipEmptyParts);
            if (parts.size() >= 5) {
                QString who = parts[1];
                int score = parts[2].toInt();
                int scanners = parts[3].toInt();
                int missiles = parts[4].toInt();
                if (who == this->playerName) {
                    ui->messageIncomeBox->append(
                        QString("Score: %1 | Scanners: %2 | Missiles: %3").arg(score).arg(scanners).arg(missiles));
                }
            }
        }


        else if (msg.startsWith("BOAT_SUNK;")) {
            // Format: BOAT_SUNK;ownerName;N;row1;col1;row2;col2; ... ;rowN;colN
            QStringList parts = msg.split(';', Qt::SkipEmptyParts);
            if (parts.size() < 3) {
                qWarning() << "BOAT_SUNK invalide:" << msg;
                continue;
            }

            const QString ownerName = parts[1];
            const int n = parts[2].toInt();
            int idx = 3;

            for (int k = 0; k < n; ++k) {
                if (idx + 1 >= parts.size()) {
                    qWarning() << "BOAT_SUNK tronqué:" << msg;
                    break;
                }
                const int r = parts[idx++].toInt();
                const int c = parts[idx++].toInt();

                // Si le bateau appartient à l'adversaire, on colore sur opponentBoard,
                // sinon sur mon board.
                if (ownerName != this->playerName) {
                    if (r < opponentBoard.size() && c < opponentBoard[r].size() && opponentBoard[r][c]) {
                        opponentBoard[r][c]->setSunkHighlight();
                    }
                } else {
                    if (r < myBoard.size() && c < myBoard[r].size() && myBoard[r][c]) {
                        myBoard[r][c]->setSunkHighlight();
                    }
                }
            }

            // Optionnel: message d’info chat
            ui->messageIncomeBox->append(QString("🚢 Bateau coulé (%1) !").arg(ownerName != this->playerName ? "adverse" : "chez vous"));
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
    if (oppositePlayer != this->playerName) {
        // plateau adverse
        Clickablewidget *w = new Clickablewidget(_case, this);
        connect(w, &Clickablewidget::clicked, this, [this,row,col](){
            onElementClicked(row, col, /*isOpponentBoard=*/true);
        });
        ui->gridOppositeBoard->addWidget(w, row, col);
        this->opponentBoard[row][col] = w;
    } else {
        // mon plateau
        Clickablewidget *w = new Clickablewidget(_case, this);
        // (pas besoin de clics sur mon plateau, mais on peut ignorer)
        ui->gridCurrentBoard->addWidget(w, row, col);
        this->myBoard[row][col] = w;
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

    // --- IMPORTANT : si un pouvoir est actif (reco ou missile), on AUTORISE le clic
    // même sur une case déjà attaquée (H/M ou dorée).
    const bool skipAlreadyAttackedCheck = (this->isReconnaisancePowerActive || this->isMissilePowerActive);

    if (!skipAlreadyAttackedCheck) {
        // Vérifier que la case n'a pas déjà été attaquée (H ou M)
        if (row < this->opponentBoard.size() && col < this->opponentBoard[row].size()) {
            Clickablewidget* targetWidget = this->opponentBoard[row][col];
            if (targetWidget && (targetWidget->getCase() == "H" || targetWidget->getCase() == "M")) {
                qDebug() << "Cette case a déjà été attaquée !";
                ui->messageIncomeBox->append("Cette case a déjà été attaquée !");
                return;
            }
        }
    }

    // --- Pouvoirs ---
    if (this->isReconnaisancePowerActive) {
        QString message = "RECONNAISANCE;" + this->roomId + ";" + QString::number(row) + ";" + QString::number(col) + "\n";
        socket->write(message.toUtf8());
        this->isReconnaisancePowerActive = false;  // désarmé côté client (le serveur gère coût/fin de tour)
        return;
    }

    if (this->isMissilePowerActive) {
        QString message = "MISSILE;" + this->roomId + ";" + QString::number(row) + ";" + QString::number(col) + "\n";
        socket->write(message.toUtf8());
        this->isMissilePowerActive = false;        // désarmé côté client (le serveur gère coût/fin de tour)
        return;
    }

    // --- Tir simple si aucun pouvoir actif ---
    QString message = "ATTACK;" + this->roomId + ";" + QString::number(row) + ";" + QString::number(col);
    socket->write(message.toUtf8());
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

void GameWindow::on_powerMissile_clicked()
{
    // Active le mode "prochain clic = MISSILE"
    this->isMissilePowerActive = true;
    ui->messageIncomeBox->append("Missile sélectionné : cliquez une case cible (centre du 3x3).");
}


