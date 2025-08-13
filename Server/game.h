#ifndef GAME_H
#define GAME_H

#include "Player.h"

class Game {
public:
    Game();
    explicit Game(Player* player1,Player* player2);
    ~Game();
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;


    // Initialisation
    void initializeGame();

    // Déroulement
    void startGame();
    void changePlayer();
    bool isGameOver() const;
    std::string getWinner() const;

    //Getter pour l'affichage
    Player* getPlayer1();
    Player* getPlayer2();
    //int getCurrentPlayerIndex();
    int reconnaissanceZone(int row, int col);
    Player* getCurrentPlayer();
    Player* getOppositePlayer();
    void attackPlayer(int row, int col);

    void displayBoards() const;

private:
    Player* m_player1;          // Joueur 1
    Player* m_player2;          // Joueur 2
    int m_currentPlayerIndex;   // Index du joueur actuel
    Player* currentPlayer; //Joueur courant
    Player* oppositePlayer;

    // Méthodes internes
    void placeBoatsRandomly(Player* player);

};

#endif // GAME_H
