#ifndef PLAYER_H
#define PLAYER_H

/**
 * \file Player.h
 * \brief Joueur : nom + plateau associé.
 * \ingroup server-model
 */

#include <string>
#include "Board.h"

/**
 * \class Player
 * \brief Modèle simple d'un joueur possédant un plateau.
 */
class Player {
public:
    /// Construit un joueur nommé (possède un Board 10x10 par défaut).
    explicit Player(const std::string& name);
    ~Player();
    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    /// Nom du joueur.
    const std::string& getName() const;

    /// Plateau du joueur.
    Board* getBoard() const;

private:
    std::string m_name; ///< Nom.
    Board* m_board;     ///< Plateau possédé.
};

#endif // PLAYER_H
