#include "spectatorwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QAction>
#include <QRegularExpression>
#include <QMessageBox>

SpectatorWindow::SpectatorWindow(const QString& _roomId, QTcpSocket* _socket, QWidget* parent)
    : QMainWindow(parent), roomId(_roomId), socket(_socket)
{
    setWindowTitle(QString("Bataille Navale — Spectateur (%1)").arg(roomId));

    central = new QWidget(this);
    setCentralWidget(central);

    auto *root = new QHBoxLayout(central);

    // Colonne gauche
    auto *leftCol = new QVBoxLayout();
    leftLabel = new QLabel("Joueur 1", this);
    leftLabel->setAlignment(Qt::AlignCenter);
    leftGrid = new QGridLayout();
    leftGrid->setSpacing(1);
    leftCol->addWidget(leftLabel);
    auto *leftWrap = new QWidget(this);
    leftWrap->setLayout(leftGrid);
    leftCol->addWidget(leftWrap);
    root->addLayout(leftCol);

    // Colonne droite
    auto *rightCol = new QVBoxLayout();
    rightLabel = new QLabel("Joueur 2", this);
    rightLabel->setAlignment(Qt::AlignCenter);
    rightGrid = new QGridLayout();
    rightGrid->setSpacing(1);
    rightCol->addWidget(rightLabel);
    auto *rightWrap = new QWidget(this);
    rightWrap->setLayout(rightGrid);
    rightCol->addWidget(rightWrap);
    root->addLayout(rightCol);

    // Panneau latéral chat / status
    auto *side = new QVBoxLayout();
    auto *statusBox = new QHBoxLayout();
    auto *lbl = new QLabel("CurrentPlayer :", this);
    currentPlayerLabel = new QLabel("-", this);
    statusBox->addWidget(lbl);
    statusBox->addWidget(currentPlayerLabel);
    side->addLayout(statusBox);

    chatView = new QTextBrowser(this);
    chatEdit = new QTextEdit(this);
    chatEdit->setMaximumHeight(70);
    sendBtn = new QPushButton("Send", this);
    quitBtn = new QPushButton("Quitter", this);

    auto *btnRow = new QHBoxLayout();
    btnRow->addWidget(sendBtn);
    btnRow->addWidget(quitBtn);

    side->addWidget(chatView);
    side->addWidget(chatEdit);
    side->addLayout(btnRow);
    root->addLayout(side);

    connect(sendBtn, &QPushButton::clicked, this, &SpectatorWindow::onSendMessage);
    connect(quitBtn, &QPushButton::clicked, this, &SpectatorWindow::onQuit);
    connect(socket, &QTcpSocket::readyRead, this, &SpectatorWindow::onReadyRead);

    // Demande l'état courant
    socket->write("CREATE_GAME;" + roomId.toUtf8());
    socket->flush();
}

SpectatorWindow::~SpectatorWindow() {}

QString SpectatorWindow::maskForSpectator(const QString& cell) const {
    // Spectateur voit uniquement H/M, le reste est masqué
    if (cell == "H" || cell == "M") return cell;
    return "X";
}

QGridLayout* SpectatorWindow::gridFor(const QString& who) const {
    return (who == player1) ? leftGrid : rightGrid;
}

void SpectatorWindow::buildGrid(const QString& who, int rows, int cols, const QStringList& cells, bool onLeft)
{
    std::vector<std::vector<Clickablewidget*>> matrix;
    matrix.resize(rows);

    QGridLayout* grid = onLeft ? leftGrid : rightGrid;

    int k = 0;
    for (int r=0; r<rows; ++r) {
        matrix[r].reserve(cols);
        for (int c=0; c<cols; ++c, ++k) {
            QString display = maskForSpectator(cells[k]);
            auto *w = new Clickablewidget(display, this); // pas de connexion de clics
            grid->addWidget(w, r, c);
            matrix[r].push_back(w);
        }
    }
    boards[who] = std::move(matrix);
}

void SpectatorWindow::updateCellFor(const QString& owner, int r, int c, const QString& symbol)
{
    auto it = boards.find(owner);
    if (it == boards.end()) return;
    if (r < 0 || r >= (int)it.value().size()) return;
    if (c < 0 || c >= (int)it.value()[r].size()) return;

    Clickablewidget* old = it.value()[r][c];
    auto *w = new Clickablewidget(symbol, this);
    gridFor(owner)->addWidget(w, r, c);
    it.value()[r][c] = w;
}

void SpectatorWindow::onReadyRead()
{
    QByteArray data = socket->readAll();
    const QString all = QString::fromUtf8(data);
    const QStringList msgs = all.split('\n', Qt::SkipEmptyParts);

    for (const QString &msg : msgs) {

        if (msg.startsWith("BOARD_CREATE;")) {
            // même layout que GameWindow: [0]tag [1]W [2]H [3]player1 [4..]board1 [..]player2 [..]board2
            QStringList t = msg.split(QRegularExpression("[;\n]"), Qt::SkipEmptyParts);
            if (t.size() < 4) continue;

            const int W = t[1].toInt();
            const int H = t[2].toInt();
            player1 = t[3].trimmed();
            leftLabel->setText(player1);

            const int cells = W*H;
            const int idxBoard1Start = 4;
            const int idxBoard1End   = idxBoard1Start + cells - 1;
            const int idxPlayer2     = idxBoard1End + 1;
            const int idxBoard2Start = idxPlayer2 + 1;
            const int idxBoard2End   = idxBoard2Start + cells - 1;
            if (t.size() <= idxBoard2End) continue;

            player2 = t[idxPlayer2].trimmed();
            rightLabel->setText(player2);

            QStringList b1 = t.mid(idxBoard1Start, cells);
            QStringList b2 = t.mid(idxBoard2Start, cells);

            buildGrid(player1, W, H, b1, /*onLeft=*/true);
            buildGrid(player2, W, H, b2, /*onLeft=*/false);
        }

        else if (msg.startsWith("STATUS;")) {
            QStringList parts = msg.split(';', Qt::SkipEmptyParts);
            if (parts.size() >= 4) {
                currentPlayerLabel->setText(parts[1]); // nom du joueur courant
                bool isOver = parts[2].toInt();
                if (isOver) {
                    QMessageBox::information(this, "Fin de partie",
                        QString("Vainqueur: %1").arg(parts[3]));
                }
            }
        }

        else if (msg.startsWith("UPDATE_CASE;")) {
            // UPDATE_CASE;owner;row;col;H|M;
            QStringList parts = msg.split(';', Qt::SkipEmptyParts);
            if (parts.size() >= 5) {
                QString owner = parts[1];
                int row = parts[2].toInt();
                int col = parts[3].toInt();
                QString sym = parts[4];
                updateCellFor(owner, row, col, sym);
            }
        }

        else if (msg.startsWith("BOAT_SUNK;")) {
            // BOAT_SUNK;owner;N;r;c;...
            QStringList parts = msg.split(';', Qt::SkipEmptyParts);
            if (parts.size() >= 3) {
                QString owner = parts[1];
                int n = parts[2].toInt();
                int idx = 3;
                for (int k=0; k<n && (idx+1)<parts.size(); ++k) {
                    int r = parts[idx++].toInt();
                    int c = parts[idx++].toInt();
                    if (boards.contains(owner)
                        && r < (int)boards[owner].size()
                        && c < (int)boards[owner][r].size()
                        && boards[owner][r][c]) {
                        boards[owner][r][c]->setSunkHighlight();
                    }
                }
                chatView->append("🚢 Un bateau a été coulé !");
            }
        }

        else if (msg.startsWith("SEND_CHAT_MESSAGE;")) {
            QStringList parts = msg.split(';', Qt::SkipEmptyParts);
            if (parts.size() >= 3) {
                chatView->append(parts[1] + ": " + parts[2]);
            }
        }

        else if (msg.startsWith("CLIENT_LEFT_ROOM;")) {
            QMessageBox::information(this, "Room fermée", "Un joueur a quitté la salle.");
            emit spectateAborted();
            close();
        }
    }
}

void SpectatorWindow::onSendMessage()
{
    const QString text = chatEdit->toPlainText().trimmed();
    if (text.isEmpty()) return;
    socket->write(QString("CHAT_MESSAGE;%1;%2\n").arg(roomId, text).toUtf8());
    socket->flush();
    chatEdit->clear();
}

void SpectatorWindow::onQuit()
{
    // pas de QUIT_ROOM côté spectateur: juste fermer localement
    emit spectateAborted();
    close();
}
