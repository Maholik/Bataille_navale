#ifndef CASE_H
#define CASE_H

class Case {
public:
    enum Status {
        Empty,    // Case vide
        Occupied, // Contient une partie d'un bateau
        Hit,      // Touché par un tir
        Miss      // Tir manqué
    };

    Case(int row = -1, int col = -1);  // Ajout des coordonnées dans le constructeur
    Status getStatus() const;         // Retourne le statut de la case
    void setStatus(Status status);    // Modifie le statut de la case

    int getRow() const;               // Retourne la ligne de la case
    int getCol() const;               // Retourne la colonne de la case
    void setPosition(int row, int col); // Définit la position de la case


private:
    Status m_status; // Statut de la case
    int m_row;       // Ligne de la case
    int m_col;       // Colonne de la case
};

#endif // CASE_H
