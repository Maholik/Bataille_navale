#ifndef CLICKABLEWIDGET_H
#define CLICKABLEWIDGET_H

#include <QWidget>
#include <QMouseEvent>
#include <Qt>


class Clickablewidget : public QWidget
{
    Q_OBJECT

private:
    QString caseElement;

public:
    explicit Clickablewidget(QString _element, QWidget* parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
    void setCase(const QString& _element);
    QString getCase();
    ~Clickablewidget();

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event);
};

#endif // CLICKABLEWIDGET_H
