#ifndef BOARD_H
#define BOARD_H

#include <vector>
#include <stdexcept>
#include "Case.h"
#include "Boat.h"

class Board {
public:
    Board(int rows, int cols);                  // Constructeur
    ~Board();                                   // Destructeur pour libérer la mémoire

    // Accesseurs
    int getRows() const;                        // Retourne le nombre de lignes
    int getCols() const;                        // Retourne le nombre de colonnes
    Case* getCase(int row, int col) const;      // Retourne une case spécifique
    std::vector<Boat*> getAllBoats();

    // Méthodes principales
    bool attack(int row, int col);              // Tire sur une case, retourne true si touché
    bool allBoatsSunk() const;                  // Vérifie si tous les bateaux sont coulés
    void display() const; // Nouvelle méthode d'affichage
    bool placeBoat(Boat* boat, int startRow, int startCol, bool horizontal); // Place un bateau sur le plateau

private:
    int m_rows;                                 // Nombre de lignes
    int m_cols;                                 // Nombre de colonnes
    std::vector<std::vector<Case*>> m_grid;    // Grille de cases
    std::vector<Boat*> m_boats;                 // Liste des bateaux placés

    bool isValidPosition(int row, int col) const; // Vérifie si une position est valide sur la grille
    void displayBoard(const Board& board);
};

#endif // BOARD_H
