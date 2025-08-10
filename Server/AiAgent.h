#ifndef AIAGENT_H
#define AIAGENT_H

#include <vector>
#include <utility>
#include <random>

class AiAgent {
public:
    AiAgent(int rows, int cols);

    // Choisit le prochain tir (row,col)
    std::pair<int,int> nextMove();

    // Informe l'IA du résultat du tir
    // status: 'H' (Hit) ou 'M' (Miss)
    // sunk: true si le bateau ciblé est désormais coulé
    void onResult(int row, int col, char status, bool sunk);

private:
    enum Mode { HUNT, TARGET };

    int rows, cols;
    Mode mode;
    std::vector<std::vector<bool>> tried; // cases déjà tirées
    std::vector<std::pair<int,int>> candidates; // pile/queue de cibles adjacentes
    std::vector<std::pair<int,int>> currentHits; // hits en cours (pour orientation)
    std::mt19937 rng;

    bool inBounds(int r, int c) const;
    bool notTried(int r, int c) const;

    void pushIfValid(int r, int c);
    void pushNeighbors(int r, int c);
    void orientAndExtend();
    std::pair<int,int> randomHuntCell();
};

#endif // AIAGENT_H
