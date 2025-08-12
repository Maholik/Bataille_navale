#include "clickablewidget.h"

Clickablewidget::Clickablewidget(QString _element,QWidget* parent, Qt::WindowFlags f) :  QWidget(parent,f) {
    this->caseElement = _element;

    // Table de correspondance entre les chaînes et les couleurs
    static const QMap<QString, QColor> colorMap = {
        {"E", QColor(30, 144, 255)},   // Bleu clair
        {"H", Qt::black},              // Noir
        {"M", Qt::darkBlue},           // Bleu foncé
        {"D", QColor(95, 158, 160)},   // Bleu sarcelle
        {"S", QColor(255, 165, 0)},   // Orange
        {"C", QColor(218, 112, 214)},  // Violet
        {"P", QColor(255, 0, 0)},     // Rouge
        {"X", Qt::darkGray}
    };

    QPalette pal = this->palette();

    // Cherche la couleur dans la table, ou prend une couleur par défaut si non trouvée
    QColor color = colorMap.value(caseElement, Qt::white);  // Blanc par défaut
    pal.setColor(QPalette::Window, color);

    this->setAutoFillBackground(true);  // Indique à Qt de remplir l'arrière-plan
    this->setPalette(pal);

    this->setMinimumSize(20,20);
}

void Clickablewidget::setCase(const QString& _element){
    this->caseElement = _element;

    // Table de correspondance entre les chaînes et les couleurs
    static const QMap<QString, QColor> colorMap = {
        {"H", Qt::black},              // Noir
        {"M", Qt::darkBlue},           // Bleu foncé
    };

    QPalette pal = this->palette();

    // Cherche la couleur dans la table, ou prend une couleur par défaut si non trouvée
    QColor color = colorMap.value(caseElement, Qt::white);  // Blanc par défaut
    //qDebug() << "MILIEU SET CASE : " << color;
    pal.setColor(QPalette::Window, color);

    this->setAutoFillBackground(true);  // Indique à Qt de remplir l'arrière-plan
    this->setPalette(pal);

    this->setMinimumSize(20,20);
}

QString Clickablewidget::getCase(){
    return this->caseElement;
}

Clickablewidget::~Clickablewidget() {}

void Clickablewidget::mousePressEvent(QMouseEvent* event) {
    emit clicked();
}

void Clickablewidget::setSunkHighlight() {
    QPalette pal = this->palette();
    pal.setColor(QPalette::Window, QColor("gold")); // Or
    this->setAutoFillBackground(true);
    this->setPalette(pal);
}
