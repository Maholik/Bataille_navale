#include "Player.h"

// Constructeur
Player::Player(const std::string& name)
    : m_name(name), m_board(new Board(10, 10)) {}

// Destructeur
Player::~Player() {
    delete m_board;
}

// Retourne le nom du joueur
const std::string& Player::getName() const {
    return m_name;
}

// Retourne le plateau du joueur
Board* Player::getBoard() const {
    return m_board;
}
