#include "placementwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRegularExpression>
#include <QMessageBox>
#include <algorithm>


static QStringList splitMessagesPlacement(const QString& chunk) {
    QString s = chunk;
    const QStringList tags = {
        "BOARD_CREATE;", "STATUS;", "SEND_CHAT_MESSAGE;", "ERROR;"
    };
    for (const QString& t : tags)
        s.replace(QRegularExpression("(?<!\\n)"+QRegularExpression::escape(t)), "\n"+t);
    return s.split('\n', Qt::SkipEmptyParts);
}

PlacementWindow::PlacementWindow(const QString& _roomId,
                                 const QString& _playerName,
                                 QTcpSocket* _socket,
                                 int rows, int cols,
                                 const QString& _p1, const QString& _p2,
                                 bool _isSolo,
                                 QWidget* parent)
    : QMainWindow(parent),
      roomId(_roomId), playerName(_playerName),
      player1(_p1), player2(_p2),
      isSoloMode(_isSolo),
      socket(_socket), R(rows), C(cols)
{
    setWindowTitle(QString("Placement — %1").arg(roomId));
    central = new QWidget(this);
    setCentralWidget(central);

    auto* root = new QHBoxLayout(central);

    // Colonne gauche: grille
    auto* left = new QVBoxLayout();
    infoLabel = new QLabel("Place tes bateaux (P=5, C=4, S=3×2, D=2) — clic sur une case pour poser la proue.", this);
    left->addWidget(infoLabel);

    grid = new QGridLayout();
    grid->setSpacing(1);
    left->addLayout(grid);

    // init matrice
    cells.resize(R);
    for (int r=0; r<R; ++r) {
        cells[r].resize(C, nullptr);
        for (int c=0; c<C; ++c) {
            auto *w = new Clickablewidget("E", this);
            grid->addWidget(w, r, c);
            cells[r][c] = w;
            // capture r,c
            connect(w, &Clickablewidget::clicked, this, [this, r, c]() { onCellClicked(r, c); });
        }
    }
    root->addLayout(left, 1);

    // Colonne droite: palette & actions
    auto* right = new QVBoxLayout();

    auto addPick = [&](const QString& label, int size, const QString& code){
        auto* b = new QPushButton(QString("%1 (%2)").arg(label).arg(size), this);
        connect(b, &QPushButton::clicked, this, [this, label, size, code]() {
            selectedSize = size; selectedCode = code;
            infoLabel->setText(QString("Sélection: %1 (%2) — %3 restant(s)").arg(label).arg(size).arg(remainingOfSize(size)));
        });

        right->addWidget(b);
    };
    addPick("Porte-avions",5,"P");
    addPick("Croiseur",4,"C");
    addPick("Sous-marin",3,"S");
    addPick("Destroyer",2,"D");

    rotateBtn = new QPushButton("Rotation (Horiz ↔ Vert)", this);
    connect(rotateBtn, &QPushButton::clicked, this, &PlacementWindow::onRotate);
    right->addWidget(rotateBtn);

    validateBtn = new QPushButton("Valider mon plateau", this);
    connect(validateBtn, &QPushButton::clicked, this, &PlacementWindow::onValidate);
    right->addWidget(validateBtn);

    quitBtn = new QPushButton("Quitter", this);
    connect(quitBtn, &QPushButton::clicked, this, &PlacementWindow::onQuit);
    right->addWidget(quitBtn);

    right->addStretch(1);
    root->addLayout(right);

    connect(socket, &QTcpSocket::readyRead, this, &PlacementWindow::onReadyRead);

    pickNextSelectable();
}

PlacementWindow::~PlacementWindow(){}

int PlacementWindow::remainingOfSize(int size) const {
    auto it = std::find_if(bag.begin(), bag.end(),
                           [size](const BoatSpec& s){ return s.size == size; });
    return (it != bag.end()) ? it->count : 0;
}


void PlacementWindow::pickNextSelectable() {
    auto it = std::find_if(bag.begin(), bag.end(),
                           [](const BoatSpec& s){ return s.count > 0; });
    if (it != bag.end()) {
        selectedSize = it->size;
        selectedCode = it->code;
        infoLabel->setText(QString("Sélection auto: %1 (%2) — %3 restant(s)")
                               .arg(selectedCode).arg(selectedSize).arg(it->count));
    } else {
        infoLabel->setText("Tous les bateaux sont placés. Clique sur Valider.");
    }
}


bool PlacementWindow::canPlaceAt(int row, int col, int size, bool H) const {
    if (H) {
        if (col+size-1 >= C) return false;
        for (int i=0;i<size;++i) if (cells[row][col+i]->getCase() != "E") return false;
    } else {
        if (row+size-1 >= R) return false;
        for (int i=0;i<size;++i) if (cells[row+i][col]->getCase() != "E") return false;
    }
    return true;
}
void PlacementWindow::paintBoat(int row, int col, int size, bool H, const QString& code) {
    if (H) {
        for (int i = 0; i < size; ++i) {
        delete cells[row][col+i];
        cells[row][col+i] = new Clickablewidget(code, this);
        grid->addWidget(cells[row][col+i], row, col+i);
        const int rr = row, cc = col + i;
        connect(cells[rr][cc], &Clickablewidget::clicked, this, [this, rr, cc]() { onCellClicked(rr, cc); });
        }
    } else {
        for (int i = 0; i < size; ++i) {
        delete cells[row+i][col];
        cells[row+i][col] = new Clickablewidget(code, this);
        grid->addWidget(cells[row+i][col], row+i, col);
        const int rr = row + i, cc = col;
        connect(cells[rr][cc], &Clickablewidget::clicked, this, [this, rr, cc]() { onCellClicked(rr, cc); });
        }
    }
}
void PlacementWindow::unpaintBoat(int row, int col, int size, bool H) {
    if (H) {
        for (int i = 0; i < size; ++i) {
        delete cells[row][col+i];
        cells[row][col+i] = new Clickablewidget("E", this);
        grid->addWidget(cells[row][col+i], row, col+i);
        const int rr = row, cc = col + i;
        connect(cells[rr][cc], &Clickablewidget::clicked, this, [this, rr, cc]() { onCellClicked(rr, cc); });
        }
    } else {
        for (int i = 0; i < size; ++i) {
        delete cells[row+i][col];
        cells[row+i][col] = new Clickablewidget("E", this);
        grid->addWidget(cells[row+i][col], row+i, col);
        const int rr = row + i, cc = col;
        connect(cells[rr][cc], &Clickablewidget::clicked, this, [this, rr, cc]() { onCellClicked(rr, cc); });
        }
    }
}
void PlacementWindow::onCellClicked(int r, int c) {
    // Si clic sur une case déjà occupée par le bateau sélectionné -> retrait (toggle simple)
    // Sinon place si possible.
    if (remainingOfSize(selectedSize) <= 0) { pickNextSelectable(); return; }

    if (!canPlaceAt(r,c,selectedSize,horizontal)) { QMessageBox::warning(this,"Placement","Placement invalide."); return; }
    // Appliquer
    paintBoat(r, c, selectedSize, horizontal, selectedCode);
    placed.push_back({selectedSize, r, c, horizontal});

    // décrémenter bag
    if (auto it = std::find_if(bag.begin(), bag.end(),
                               [this](const BoatSpec& s){ return s.size == selectedSize; });
        it != bag.end())
    {
        it->count--;
    }

    pickNextSelectable();

}

void PlacementWindow::onRotate() {
    horizontal = !horizontal;
    infoLabel->setText(QString("Orientation: %1").arg(horizontal?"Horizontale":"Verticale"));
}

void PlacementWindow::onValidate() {
    // Vérifier inventaire épuisé
    if (std::any_of(bag.begin(), bag.end(),
                    [](const BoatSpec& s){ return s.count > 0; })) {
        QMessageBox::warning(this, "Placement", "Tu dois placer tous les bateaux.");
        return;
    }

    // Construire SUBMIT_BOARD;roomId;playerName;W;H;N;size;row;col;H|V;...
    QString msg = QString("SUBMIT_BOARD;%1;%2;%3;%4;%5;")
                    .arg(roomId).arg(playerName).arg(R).arg(C).arg(placed.size());
    for (const auto& b : placed) {
        msg += QString::number(b.size) + ";" + QString::number(b.row) + ";" + QString::number(b.col) + ";" + (b.horizontal?"H;":"V;");
    }
    msg += "\n";
    socket->write(msg.toUtf8());
    socket->flush();
    validateBtn->setEnabled(false);
    infoLabel->setText("Plateau envoyé. En attente de l'adversaire…");
}

void PlacementWindow::onReadyRead() {
    QByteArray data = socket->readAll();
    const QStringList msgs = splitMessagesPlacement(QString::fromUtf8(data));
    for (const QString& m : msgs) {
        if (m.startsWith("ERROR;")) {
            QMessageBox::warning(this, "Erreur serveur", m.mid(6));
        }
        else if (m.startsWith("BOARD_CREATE;")) {
            // Passer à la GameWindow existante
            disconnect(socket, &QTcpSocket::readyRead, this, &PlacementWindow::onReadyRead);
            auto* gw = new GameWindow(roomId, socket, playerName, this);
            connect(gw, &GameWindow::gameAborted, this, [this](){ emit placementAborted(); this->close(); });
            gw->show();
            this->hide();
        }
    }
}

void PlacementWindow::onQuit() {
    emit placementAborted();
    close();
}
