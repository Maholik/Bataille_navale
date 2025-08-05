#include "Case.h"

// Constructeur
Case::Case(int row, int col) : m_status(Empty), m_row(row), m_col(col) {}

// Retourne le statut actuel de la case
Case::Status Case::getStatus() const {
    return m_status;
}

// Modifie le statut de la case
void Case::setStatus(Case::Status status) {
    m_status = status;
}

// Retourne la ligne de la case
int Case::getRow() const {
    return m_row;
}

// Retourne la colonne de la case
int Case::getCol() const {
    return m_col;
}

// Définit la position de la case
void Case::setPosition(int row, int col) {
    m_row = row;
    m_col = col;
}


