#ifndef AIAGENT_H
#define AIAGENT_H

/**
 * \file AiAgent.h
 * \brief IA de tir : modes HUNT/TARGET et file de cibles.
 * \ingroup server-logic
 */

#include <vector>
#include <utility>
#include <random>

/**
 * \class AiAgent
 * \brief Choisit le prochain tir et s'adapte selon les résultats (H/M/sunk).
 */
class AiAgent {
public:
    /// Initialise l'IA pour un plateau \p rows x \p cols.
    AiAgent(int rows, int cols);

    /// \return (row,col) du prochain tir.
    std::pair<int,int> nextMove();

    /**
     * \brief Informe l'IA du résultat du tir.
     * \param row Ligne tirée.
     * \param col Colonne tirée.
     * \param status 'H' (hit) ou 'M' (miss).
     * \param sunk Vrai si un bateau vient d'être coulé.
     */
    void onResult(int row, int col, char status, bool sunk);

private:
    enum Mode { HUNT, TARGET }; ///< Stratégie courante.

    int rows, cols;
    Mode mode = HUNT;
    std::vector<std::vector<bool>> tried;        ///< Cases déjà tentées.
    std::vector<std::pair<int,int>> candidates;  ///< File de cibles adjacentes.
    std::vector<std::pair<int,int>> currentHits; ///< Série de coups au but.
    std::mt19937 rng;

    bool inBounds(int r, int c) const;
    bool notTried(int r, int c) const;

    void pushIfValid(int r, int c);
    void pushNeighbors(int r, int c);
    void orientAndExtend();
    std::pair<int,int> randomHuntCell();
};

#endif // AIAGENT_H
