#ifndef GAME_H
#define GAME_H

/**
 * \file Game.h
 * \brief Moteur de jeu : joueurs, tours, attaques, pouvoirs.
 * \ingroup server-logic
 */

#include "Player.h"

/**
 * \class Game
 * \brief Contient la logique de haut niveau d'une partie 1v1.
 */
class Game {
public:
    Game();
    explicit Game(Player* player1,Player* player2);
    ~Game();
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    /// Initialise deux plateaux de manière aléatoire.
    void initializeGame();

    /// Démarre la partie (si applicable côté serveur).
    void startGame();

    /// Change le joueur courant.
    void changePlayer();

    /// \return Vrai si un plateau adverse n'a plus de bateaux.
    bool isGameOver() const;

    /// \return Nom du vainqueur ou "Aucun gagnant".
    std::string getWinner() const;

    /// Accès aux joueurs (ex. sérialisation vers client).
    Player* getPlayer1();
    Player* getPlayer2();

    /**
     * \brief Pouvoir de reconnaissance (compte de bateaux dans une zone).
     * \param row Centre ligne.
     * \param col Centre colonne.
     * \return Nombre de bateaux détectés (uniques).
     */
    int reconnaissanceZone(int row, int col);

    /// Joueur courant / opposé.
    Player* getCurrentPlayer();
    Player* getOppositePlayer();

    /**
     * \brief Applique une attaque sur l'adversaire.
     * \param row Ligne visée.
     * \param col Colonne visée.
     */
    void attackPlayer(int row, int col);

    /// Affiche les plateaux en console (debug).
    void displayBoards() const;

private:
    Player* m_player1;          ///< Joueur 1.
    Player* m_player2;          ///< Joueur 2.
    int m_currentPlayerIndex;   ///< Index du joueur courant.
    Player* currentPlayer;      ///< Pointeur joueur courant.
    Player* oppositePlayer;     ///< Pointeur joueur opposé.

    /// Placement aléatoire interne.
    void placeBoatsRandomly(Player* player);
};

#endif // GAME_H
