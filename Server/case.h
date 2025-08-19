#ifndef CASE_H
#define CASE_H

/**
 * \file case.h
 * \brief Représente une case d'un plateau (statut + coordonnées).
 * \ingroup server-model
 */
class Case {
public:
    /// Statut logique d'une case.
    enum Status { Empty, Occupied, Hit, Miss };

    /**
     * \brief Construit une case.
     * \param row Ligne initiale (-1 si non positionnée).
     * \param col Colonne initiale (-1 si non positionnée).
     */
    explicit Case(int row = -1, int col = -1);

    /// Retourne le statut courant.
    Status getStatus() const;

    /// Change le statut.
    void setStatus(Status status);

    /// Coordonnée ligne.
    int getRow() const;

    /// Coordonnée colonne.
    int getCol() const;

    /// Modifie la position (ligne/colonne).
    void setPosition(int row, int col);

private:
    Status m_status; ///< Statut de la case.
    int m_row;       ///< Ligne.
    int m_col;       ///< Colonne.
};

#endif // CASE_H
