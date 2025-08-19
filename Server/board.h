#ifndef BOARD_H
#define BOARD_H

/**
 * \file Board.h
 * \brief Plateau de jeu : grille de \ref Case et liste de \ref Boat.
 * \ingroup server-model
 */

#include <vector>
#include <stdexcept>
#include "Case.h"
#include "Boat.h"

/**
 * \class Board
 * \brief Gère la grille, le placement des bateaux et les tirs.
 */
class Board {
public:
    /// Construit un plateau rows x cols.
    Board(int rows, int cols);

    /// Libère les \ref Case et \ref Boat possédés.
    ~Board();

    /// \name Accesseurs
    ///@{
    int getRows() const;                 ///< Nombre de lignes.
    int getCols() const;                 ///< Nombre de colonnes.
    Case* getCase(int row, int col) const; ///< Accès à une case (exception si hors bornes).
    std::vector<Boat*> getAllBoats();    ///< Copie de la liste des bateaux.
    ///@}

    /// Tire sur une case ; \return true si touché.
    bool attack(int row, int col);

    /// \return true si tous les bateaux sont coulés.
    bool allBoatsSunk() const;

    /// Affiche la grille en console (debug).
    void display() const;

    /**
     * \brief Place un bateau si possible.
     * \param boat Pointeur du bateau (possédé après placement).
     * \param startRow Ligne de départ.
     * \param startCol Colonne de départ.
     * \param horizontal Orientation.
     * \return true si placement réussi.
     */
    bool placeBoat(Boat* boat, int startRow, int startCol, bool horizontal);

private:
    int m_rows;
    int m_cols;
    std::vector<std::vector<Case*>> m_grid;
    std::vector<Boat*> m_boats;

    /// Vérifie la validité des coordonnées dans la grille.
    bool isValidPosition(int row, int col) const;

    /// (Interne) Affichage détaillé.
    void displayBoard(const Board& board);
};

#endif // BOARD_H
