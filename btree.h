#ifndef BTREE_H
#define BTREE_H

// Représente une colonne dans une table
typedef struct Column {
    char name[32];    // Nom de la colonne
} Column;

// Représente une ligne dans la table (données)
typedef struct {
    int id;
    char name[32];
} Row;

// Représente un nœud dans l'arbre binaire
typedef struct Node {
    Row data;
    struct Node* left;
    struct Node* right;
} Node;

// Représente une table
typedef struct Table {
    Node* root;          // Arbre binaire pour les données
    int next_id;         // ID pour les nouvelles lignes
    char name[32];       // Nom de la table
    Column columns[100]; // Liste des colonnes
    int num_columns;     // Nombre de colonnes
} Table;

// Déclarations des fonctions liées à l'arbre binaire
Node* insert_into_btree(Node* root, Row new_row);
void print_btree(Node* root);


#endif //BTREE_H