#ifndef BOAT_H
#define BOAT_H

#include <vector>
#include <string>
#include "Case.h"

class Boat {
public:
    explicit Boat(int size);

    // Accesseurs
    int getSize() const;
    const std::vector<Case*>& getStructure() const;
    bool isSunk() const; // Vérifie si le bateau est coulé
    const std::string& getName() const;
    void setCase(int index, Case* boardCase);

private:
    int m_size;                       // Taille du bateau
    std::vector<Case*> m_structure;   // Cases du bateau
    std::string m_name;               // Nom du bateau

    void initializeStructure();       // Initialise les cases du bateau
};

#endif // BOAT_H
