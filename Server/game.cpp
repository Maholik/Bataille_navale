#include "Game.h"
#include <iostream>
#include <cstdlib>
#include <ctime>

// Constructeur
Game::Game()
    : m_player1(new Player("Joueur 1")),
    m_player2(new Player("Joueur 2")),
    m_currentPlayerIndex(0) {
    this->currentPlayer = m_currentPlayerIndex == 0 ? m_player1 : m_player2;
    this->oppositePlayer = m_currentPlayerIndex == 0 ? m_player2 : m_player1;
    std::srand(std::time(nullptr)); // Initialisation de la graine aléatoire
}

Game::Game(Player* player1,Player* player2){
    this->m_player1 = player1;
    this->m_player2 = player2;
    this->m_currentPlayerIndex = 0;
    this->currentPlayer = m_currentPlayerIndex == 0 ? m_player1 : m_player2;
    this->oppositePlayer = m_currentPlayerIndex == 0 ? m_player2 : m_player1;
    std::srand(std::time(nullptr)); // Initialisation de la graine aléatoire
}

// Destructeur
Game::~Game() {
    delete m_player1;
    delete m_player2;
}

// Initialisation du jeu
void Game::initializeGame() {
    placeBoatsRandomly(m_player1);
    placeBoatsRandomly(m_player2);
}

// Place les bateaux aléatoirement sur le plateau d'un joueur
void Game::placeBoatsRandomly(Player* player) {
    Board* board = player->getBoard();

    // Créer des bateaux
    Boat* destroyer = new Boat(2);
    Boat* submarine1 = new Boat(3);
    Boat* submarine2 = new Boat(3);
    Boat* cruiser = new Boat(4);
    Boat* carrier = new Boat(5);

    std::vector<Boat*> boats = {destroyer, submarine1, submarine2, cruiser, carrier};

    // Tenter de placer chaque bateau
    for (Boat* boat : boats) {
        int maxAttempts = 100;
        while (maxAttempts-- > 0) {
            int row = std::rand() % board->getRows();
            int col = std::rand() % board->getCols();
            bool horizontal = std::rand() % 2 == 0;

            if (board->placeBoat(boat, row, col, horizontal)) {
                break; // Bateau placé avec succès
            }
        }
    }
}

//Methode que j'ai rajoute pour pouvoir jouer via la vue
void Game::attackPlayer(int row, int col){
    this->oppositePlayer->getBoard()->attack(row, col);
}

// Passe au tour suivant
void Game::changePlayer() {
    m_currentPlayerIndex = (m_currentPlayerIndex + 1) % 2;
    this->currentPlayer = m_currentPlayerIndex == 0 ? m_player1 : m_player2;
    this->oppositePlayer = m_currentPlayerIndex == 0 ? m_player2 : m_player1;
}

// Vérifie si le jeu est terminé
bool Game::isGameOver() const {
    return m_player1->getBoard()->allBoatsSunk() || m_player2->getBoard()->allBoatsSunk();
}

// Retourne le gagnant
std::string Game::getWinner() const {
    if (m_player1->getBoard()->allBoatsSunk()) {
        return m_player2->getName();
    } else if (m_player2->getBoard()->allBoatsSunk()) {
        return m_player1->getName();
    }
    return "Aucun gagnant";
}

Player* Game::getPlayer1(){
    return this->m_player1;
}
Player* Game::getPlayer2(){
    return this->m_player2;
}

Player* Game::getCurrentPlayer(){
    return this->currentPlayer;
}

Player* Game::getOppositePlayer(){
    return this->oppositePlayer;
}

// Affiche les plateaux
void Game::displayBoards() const {
    std::cout << "\nPlateau de " << m_player1->getName() << " :\n";
    m_player1->getBoard()->display();

    std::cout << "\nPlateau de " << m_player2->getName() << " :\n";
    m_player2->getBoard()->display();
}


int Game::reconnaissanceZone(int row, int col) {
    // Rayon de recherche autour de la case donnée
    const int rayon = 2;
    int count = 0;

    // Obtenir le plateau du joueur opposé
    Board* board = this->oppositePlayer->getBoard();

    // Parcourir les cases dans le carré de côté (2 * rayon + 1)
    for (int i = row - rayon; i <= row + rayon; ++i) {
        for (int j = col - rayon; j <= col + rayon; ++j) {
            // Vérifier que les indices sont dans les limites du plateau
            if (i >= 0 && i < board->getRows() && j >= 0 && j < board->getCols()) {
                Case* currentCase = board->getCase(i, j);

                // Vérifier si la case contient un bateau (status "Occupied")
                if (currentCase->getStatus() == Case::Occupied) {
                    count++;
                }
            }
        }
    }

    return count;
}

