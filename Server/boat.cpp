#include "Boat.h"
#include <stdexcept>
#include <algorithm>

// Constructeur
Boat::Boat(int size) : m_size(size) {
    // Définir un nom en fonction de la taille
    switch (size) {
    case 2: m_name = "Destroyer"; break;
    case 3: m_name = "Sous-marin"; break;
    case 4: m_name = "Croiseur"; break;
    case 5: m_name = "Porte-avions"; break;
    default: throw std::invalid_argument("Taille de bateau invalide");
    }
    initializeStructure(); // Initialise les cases du bateau
}

// Initialise les cases du bateau
void Boat::initializeStructure() {
    m_structure.clear();
    for (int i = 0; i < m_size; ++i) {
        Case* newCase = new Case(); // Crée une case sans position au départ
        m_structure.push_back(newCase);
        newCase->setStatus(Case::Occupied);
    }
}

// Retourne la taille du bateau
int Boat::getSize() const {
    return m_size;
}

// Retourne les cases qui forment le bateau
const std::vector<Case*>& Boat::getStructure() const {
    return m_structure;
}

// Vérifie si le bateau est coulé
bool Boat::isSunk() const {
    return std::all_of(m_structure.begin(), m_structure.end(),
                       [](const Case* c){ return c && c->getStatus() == Case::Hit; });
}


// Retourne le nom du bateau
const std::string& Boat::getName() const {
    return m_name;
}

void Boat::setCase(int index, Case* boardCase){
    if (index >= 0 && index < m_size) {
        m_structure[index] = boardCase;  // Associe une Case du plateau à une Case du bateau
    }
}

