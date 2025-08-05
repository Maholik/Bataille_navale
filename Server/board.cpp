#include "Board.h"
#include <iostream>

// Constructeur
Board::Board(int rows, int cols) : m_rows(rows), m_cols(cols) {
    if (rows <= 0 || cols <= 0) {
        throw std::invalid_argument("Le plateau doit avoir un nombre positif de lignes et de colonnes.");
    }

    // Initialisation de la grille avec des cases
    m_grid.resize(rows);
    for (int i = 0; i < rows; ++i) {
        m_grid[i].resize(cols);
        for (int j = 0; j < cols; ++j) {
            m_grid[i][j] = new Case(i, j);
            m_grid[i][j]->setStatus(Case::Empty);
        }
    }
}

// Destructeur
Board::~Board() {
    // Libération des cases
    for (int i = 0; i < m_rows; ++i) {
        for (int j = 0; j < m_cols; ++j) {
            delete m_grid[i][j];
        }
    }

    // Libération des bateaux
    for (Boat* boat : m_boats) {
        delete boat;
    }
}

std::vector<Boat*> Board::getAllBoats(){
    return this->m_boats;
}

// Retourne le nombre de lignes
int Board::getRows() const {
    return m_rows;
}

// Retourne le nombre de colonnes
int Board::getCols() const {
    return m_cols;
}

// Retourne une case spécifique
Case* Board::getCase(int row, int col) const {
    if (!isValidPosition(row, col)) {
        throw std::out_of_range("Position invalide !");
    }
    return m_grid[row][col];
}


// Tire sur une case
bool Board::attack(int row, int col) {
    if (!isValidPosition(row, col)) {
        throw std::out_of_range("Position invalide !");
    }

    Case* target = m_grid[row][col];
    if (target->getStatus() == Case::Occupied) {
        target->setStatus(Case::Hit);
        return true;  // Touché
    } else if (target->getStatus() == Case::Empty) {
        target->setStatus(Case::Miss);
        return false;  // Manqué
    }
    return false;  // Aucune action si la case est déjà touchée ou manquée
}

// Vérifie si tous les bateaux sont coulés
bool Board::allBoatsSunk() const {
    for (const Boat* boat : m_boats) {
        if (!boat->isSunk()) {
            return false; // Au moins un bateau n'est pas encore coulé
        }
    }
    return true; // Tous les bateaux sont coulés
}

// Vérifie si une position est valide
bool Board::isValidPosition(int row, int col) const {
    return row >= 0 && row < m_rows && col >= 0 && col < m_cols;
}

// Place un bateau sur le plateau
bool Board::placeBoat(Boat* boat, int startRow, int startCol, bool horizontal) {

    // Vérifiez si le placement est valide
    for (int i = 0; i < boat->getSize(); ++i) {
        int row = startRow + (horizontal ? 0 : i);
        int col = startCol + (horizontal ? i : 0);

        if (!isValidPosition(row, col) || m_grid[row][col]->getStatus() != Case::Empty) {
            return false; // Placement invalide
        }
    }

    // Place le bateau sur le plateau
    for (int i = 0; i < boat->getSize(); ++i) {
        int row = startRow + (horizontal ? 0 : i);
        int col = startCol + (horizontal ? i : 0);

        //m_grid[row][col]->setStatus(Case::Occupied);

        Case* boardCase = m_grid[row][col];
        boardCase->setStatus(Case::Occupied);
        boat->setCase(i, boardCase);  // Associe la case du bateau à la case du plateau

    }

    // Ajouter le bateau à la liste des bateaux
    m_boats.push_back(boat);
    return true;
}

void Board::display() const {
    for (int row = 0; row < m_rows; ++row) {
        for (int col = 0; col < m_cols; ++col) {
            Case* c = m_grid[row][col];
            switch (c->getStatus()) {
            case Case::Empty: std::cout << "O "; break;    // Case vide
            case Case::Occupied: std::cout << "B "; break; // Bateau
            case Case::Hit: std::cout << "H "; break;      // Touché
            case Case::Miss: std::cout << "M "; break;     // Manqué
            }
        }
        std::cout << std::endl;
    }
}

