#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include "Board.h"

class Player {
public:
    explicit Player(const std::string& name);
    ~Player();

    // Accesseurs
    const std::string& getName() const;
    Board* getBoard() const;

private:
    std::string m_name; // Nom du joueur
    Board* m_board;     // Plateau associé au joueur
};

#endif // PLAYER_H
