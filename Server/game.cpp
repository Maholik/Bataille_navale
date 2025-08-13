#include "Game.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <set>
#include <algorithm>
#include <QDebug>
#include <QRandomGenerator>



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
// Server/game.cpp
void Game::placeBoatsRandomly(Player* player) {
    Board* board = player->getBoard();

    // IMPORTANT : pas de srand() ici (tu l’as déjà fait dans le ctor du Game).
    auto* rng = QRandomGenerator::global();
    const int rows = board->getRows();
    const int cols = board->getCols();

    // Flotte alignée à ton mapping d’envoi (2=D, 3=S, 4=C, 5=P) — voir §C.
    std::vector<int> sizes = {2,3,3,4,5};

    for (int size : sizes) {
        // Allouer le bateau avant, mais prévoir le nettoyage si jamais on échoue
        Boat* boat = new Boat(size);

        // Tentatives proportionnelles à la taille de la grille
        int attempts = std::max(200, rows * cols * 5);
        bool placed = false;

        while (attempts-- > 0) {
            const int row = rng->bounded(rows);
            const int col = rng->bounded(cols);
            const bool horizontal = rng->bounded(2) == 0;

            if (board->placeBoat(boat, row, col, horizontal)) {
                placed = true;
                break;
            }
        }

        if (!placed) {
            // Échec exceptionnel : on libère le bateau pour éviter une fuite.
            delete boat;
            // Option 1 : relancer complètement la pose de TOUTE la flotte (simple).
            // Option 2 : throw/retourner une erreur que l’appelant gérera.
            // Ici on choisit de relancer une fois toute la flotte :
            // -- clear board si tu as une API pour ça (sinon laisse comme aujourd’hui).
            // -- recommencer sizes depuis le début.
            // Pour rester "drop-in", on logge et on continue.
            qWarning() << "[AI] Impossible de placer un bateau de taille" << size
                       << "après beaucoup d'essais. La flotte peut être incomplète.";
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
    const int rayon = 2;
    Board* board = this->oppositePlayer->getBoard();

    std::set<Boat*> seen; // bateaux uniques trouvés

    for (int i = row - rayon; i <= row + rayon; ++i) {
        for (int j = col - rayon; j <= col + rayon; ++j) {
            if (i < 0 || i >= board->getRows() || j < 0 || j >= board->getCols()) continue;

            Case* cell = board->getCase(i, j);

            // Cherche si cette case appartient à un bateau (Occupied ou Hit)
            for (Boat* b : board->getAllBoats()) {
                const auto& s = b->getStructure();
                if (std::find(s.begin(), s.end(), cell) != s.end()) {
                    seen.insert(b);
                    break; // pas besoin de continuer pour cette case
                }
            }
        }
    }
    qDebug() << "Reco (" << row << "," << col << ") =>" << static_cast<int>(seen.size());
    return static_cast<int>(seen.size());
}


