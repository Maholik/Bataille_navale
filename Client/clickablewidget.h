#ifndef CLICKABLEWIDGET_H
#define CLICKABLEWIDGET_H

/**
 * \file clickablewidget.h
 * \brief Widget cliquable représentant une case du plateau (couleur selon l'état).
 * \ingroup client-ui
 */

#include <QWidget>
#include <QMouseEvent>
#include <Qt>

/**
 * \class Clickablewidget
 * \brief Case graphique cliquable.
 * \details Affiche une couleur selon un code de case : E (eau), H (touché),
 * M (manqué), D/S/C/P (types de bateaux), X (inconnu/masqué).
 */
class Clickablewidget : public QWidget
{
    Q_OBJECT

private:
    QString caseElement; ///< Code de la case (E,H,M,D,S,C,P,X).

public:
    /**
     * \brief Constructeur.
     * \param _element Code initial de la case.
     * \param parent Parent Qt.
     * \param f Flags de fenêtre Qt.
     */
    explicit Clickablewidget(QString _element, QWidget* parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());

    /**
     * \brief Modifie le code de la case (met à jour la couleur).
     * \param _element Nouveau code.
     */
    void setCase(const QString& _element);

    /**
     * \brief Retourne le code courant de la case.
     */
    QString getCase();

    /// Destructeur.
    ~Clickablewidget();

    /**
     * \brief Met la case en surbrillance "coulé" (doré).
     */
    void setSunkHighlight();

signals:
    /**
     * \brief Émis quand l'utilisateur clique sur la case.
     */
    void clicked();

protected:
    /// Intercepte le clic souris pour émettre \ref clicked().
    void mousePressEvent(QMouseEvent* event) override;
};

#endif // CLICKABLEWIDGET_H
