#ifndef BOAT_H
#define BOAT_H

/**
 * \file Boat.h
 * \brief Modèle de bateau : taille, nom, et liste de cases.
 * \ingroup server-model
 */

#include <vector>
#include <string>
#include "Case.h"

/**
 * \class Boat
 * \brief Représente un bateau et ses cases associées.
 */
class Boat {
public:
    /// Construit un bateau d'une taille donnée (2,3,4,5).
    explicit Boat(int size);

    /// Taille du bateau.
    int getSize() const;

    /// Référence sur les cases composant le bateau.
    const std::vector<Case*>& getStructure() const;

    /// \return Vrai si toutes les cases sont \c Hit.
    bool isSunk() const;

    /// Nom "humain" (Destroyer, Sous-marin, ...).
    const std::string& getName() const;

    /**
     * \brief Associe une case de plateau à l'index i du bateau.
     * \param index Index dans la structure.
     * \param boardCase Case du plateau.
     */
    void setCase(int index, Case* boardCase);

private:
    int m_size;                    ///< Taille du bateau.
    std::vector<Case*> m_structure;///< Cases du bateau.
    std::string m_name;            ///< Nom du bateau.

    /// Initialise la structure interne.
    void initializeStructure();
};

#endif // BOAT_H
