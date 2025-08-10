#include "AiAgent.h"
#include <chrono>

AiAgent::AiAgent(int r, int c)
    : rows(r), cols(c), mode(HUNT),
    tried(r, std::vector<bool>(c,false)),
    rng(static_cast<unsigned int>(std::chrono::steady_clock::now().time_since_epoch().count())) {}

bool AiAgent::inBounds(int r, int c) const { return r>=0 && r<rows && c>=0 && c<cols; }
bool AiAgent::notTried(int r, int c) const { return inBounds(r,c) && !tried[r][c]; }

void AiAgent::pushIfValid(int r, int c) {
    if (notTried(r,c)) candidates.emplace_back(r,c);
}

void AiAgent::pushNeighbors(int r, int c) {
    pushIfValid(r-1, c);
    pushIfValid(r+1, c);
    pushIfValid(r, c-1);
    pushIfValid(r, c+1);
}

std::pair<int,int> AiAgent::randomHuntCell() {
    // Choix uniforme parmi les cases non testées
    std::vector<std::pair<int,int>> pool;
    pool.reserve(rows*cols);
    for (int r=0;r<rows;++r)
        for (int c=0;c<cols;++c)
            if (!tried[r][c]) pool.emplace_back(r,c);

    if (pool.empty()) return {-1,-1};
    std::uniform_int_distribution<size_t> dist(0, pool.size()-1);
    return pool[dist(rng)];
}

std::pair<int,int> AiAgent::nextMove() {
    std::pair<int,int> shot{-1,-1};

    // Si on a des cibles adjacentes à tester, on reste en TARGET
    if (!candidates.empty()) {
        shot = candidates.back();
        candidates.pop_back();
        if (shot.first != -1) {
            tried[shot.first][shot.second] = true;
            return shot;
        }
    }

    mode = HUNT;
    // sinon on chasse au hasard
    shot = randomHuntCell();
    if (shot.first != -1) tried[shot.first][shot.second] = true;
    return shot;
}

void AiAgent::orientAndExtend() {
    if (currentHits.size() < 2) return;

    auto [r1,c1] = currentHits[currentHits.size()-2];
    auto [r2,c2] = currentHits.back();

    if (r1 == r2) {
        // horizontal → étendre à gauche/droite depuis les extrémités connues
        int minC = c1 < c2 ? c1 : c2;
        int maxC = c1 < c2 ? c2 : c1;
        pushIfValid(r1, minC-1);
        pushIfValid(r1, maxC+1);
    } else if (c1 == c2) {
        // vertical → étendre en haut/bas
        int minR = r1 < r2 ? r1 : r2;
        int maxR = r1 < r2 ? r2 : r1;
        pushIfValid(minR-1, c1);
        pushIfValid(maxR+1, c1);
    }
}

void AiAgent::onResult(int row, int col, char status, bool sunk) {
    if (status == 'H') {
        mode = TARGET;
        currentHits.emplace_back(row,col);

        if (currentHits.size() == 1) {
            // premier hit: pousser les 4 voisins
            pushNeighbors(row, col);
        } else {
            // >1 hit: on a peut-être une orientation → étendre dans l’axe
            orientAndExtend();
        }

        if (sunk) {
            // Bateau terminé → reset du mode ciblage
            currentHits.clear();
            candidates.clear();
            mode = HUNT;
        }
    }
    // en cas de 'M', on ne fait rien : on tirera la prochaine candidate ou on repassera en HUNT
}
