#include "placementwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTime>

PlacementWindow::PlacementWindow(const QString& rid, const QString& pname, QTcpSocket* s, QWidget* parent)
    : QMainWindow(parent), roomId(rid), playerName(pname), socket(s)
{
    setWindowTitle(QString("Placement — %1").arg(playerName));
    central = new QWidget(this);
    setCentralWidget(central);

    auto* root = new QHBoxLayout(central);

    auto* left = new QVBoxLayout();
    infoLabel = new QLabel(QString("Placez vos bateaux  (reste: %1,%2,%3,%4,%5)")
                           .arg(todo.value(0)).arg(todo.value(1)).arg(todo.value(2)).arg(todo.value(3)).arg(todo.value(4)), this);
    left->addWidget(infoLabel);

    auto* wrap = new QWidget(this);
    grid = new QGridLayout();
    grid->setSpacing(1);
    wrap->setLayout(grid);
    left->addWidget(wrap);
    root->addLayout(left);

    auto* side = new QVBoxLayout();
    orientBtn = new QPushButton("Orientation: Horizontal", this);
    finishBtn = new QPushButton("Terminer", this);
    finishBtn->setEnabled(false);
    logView = new QTextBrowser(this);

    side->addWidget(orientBtn);
    side->addWidget(finishBtn);
    side->addWidget(logView);
    root->addLayout(side);

    buildEmptyGrid(10,10);

    connect(socket, &QTcpSocket::readyRead, this, &PlacementWindow::onReadyRead);
    connect(orientBtn, &QPushButton::clicked, this, &PlacementWindow::onToggleOrientation);
    connect(finishBtn,  &QPushButton::clicked, this, &PlacementWindow::onFinish);

    log("⚓ Phase de placement ouverte.");
}

void PlacementWindow::buildEmptyGrid(int rows, int cols)
{
    myGrid.assign(rows, {});
    for (int r=0; r<rows; ++r) {
        myGrid[r].reserve(cols);
        for (int c=0; c<cols; ++c) {
            auto* w = new Clickablewidget("E", this);
            grid->addWidget(w, r, c);
            myGrid[r].push_back(w);
            connect(w, &Clickablewidget::clicked, this, [=](){ onCellClicked(r,c); });
        }
    }
}

void PlacementWindow::onToggleOrientation()
{
    horizontal = !horizontal;
    orientBtn->setText(QString("Orientation: %1").arg(horizontal ? "Horizontal" : "Vertical"));
}

void PlacementWindow::onCellClicked(int r, int c)
{
    if (todo.isEmpty()) { log("Tous les bateaux sont déjà placés."); return; }
    const int size = todo.front();
    // On demande au serveur
    socket->write(QString("PLACE_BOAT;%1;%2;%3;%4;%5\n")
                  .arg(roomId).arg(size).arg(r).arg(c).arg(horizontal ? "H":"V").toUtf8());
    socket->flush();
}

void PlacementWindow::paintBoat(int size, int row, int col, bool horiz)
{
    // couleur/lettre selon la taille
    QString code = "P";
    if (size==4) code = "C";
    else if (size==3) code = "S";
    else if (size==2) code = "D";

    for (int i=0; i<size; ++i) {
        int rr = row + (horiz ? 0 : i);
        int cc = col + (horiz ? i : 0);
        if (rr<0 || cc<0 || rr>= (int)myGrid.size() || cc >= (int)myGrid[rr].size()) continue;

        // Remplacer le widget (comme dans l’updateBoard)
        if (myGrid[rr][cc]) {
            auto* old = myGrid[rr][cc];
            grid->removeWidget(old);
            old->deleteLater();
        }
        auto* w = new Clickablewidget(code, this);
        grid->addWidget(w, rr, cc);
        myGrid[rr][cc] = w;
    }
}

void PlacementWindow::log(const QString& line)
{
    const QString hhmm = QTime::currentTime().toString("HH:mm");
    logView->append(QString("<span style='color:#888'>[%1]</span> %2").arg(hhmm, line.toHtmlEscaped()));
}

void PlacementWindow::onReadyRead()
{
    const QString raw = QString::fromUtf8(socket->readAll());

    // Injection de \n avant nos tags, comme côté jeu
    QString s = raw;
    const QStringList tags = { "PLACE_OK;", "PLACE_ERR;", "PLACE_ALL_SET", "PLACEMENT_OVER;" };
    for (const QString& t : tags) {
        s.replace(QRegularExpression("(?<!\\n)" + QRegularExpression::escape(t)), "\n" + t);
    }
    const QStringList lines = s.split('\n', Qt::SkipEmptyParts);

    for (const QString& msg : lines) {
        if (msg.startsWith("PLACE_OK;")) {
            auto p = msg.split(';', Qt::SkipEmptyParts);
            if (p.size() >= 5) {
                int size = p[1].toInt();
                int row  = p[2].toInt();
                int col  = p[3].toInt();
                bool horiz = (p[4] == "H");
                paintBoat(size, row, col, horiz);
                int idx = todo.indexOf(size);
                if (idx >= 0) todo.removeAt(idx);
                infoLabel->setText(QString("Placez vos bateaux  (reste: %1)")
                                       .arg([&](){ QStringList x; for (int v: std::as_const(todo)) x<<QString::number(v); return x.join(","); }()));
                if (todo.isEmpty()) { finishBtn->setEnabled(true); log("Tous les bateaux sont placés. Cliquez sur Terminer."); }
                }
            }
            else if (msg.startsWith("PLACE_ERR;")) {
                log("❌ Placement refusé (collision/hors grille). Essayez une autre position.");
            }
            else if (msg.startsWith("PLACE_ALL_SET")) {
                // rien, déjà géré
            }
            else if (msg.startsWith("PLACEMENT_OVER;")) {
                emit placementFinished();
                close();
            }
        }
    }


void PlacementWindow::onFinish()
{
    socket->write(QString("PLACE_DONE;%1\n").arg(roomId).toUtf8());
    socket->flush();
    finishBtn->setEnabled(false);
    log("En attente de l’autre joueur…");
}
