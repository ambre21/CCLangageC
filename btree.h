
#ifndef BTREE_H
#define BTREE_H

typedef struct {
    int id;
    char name[32];
} Row;

typedef struct Node {
    Row data;
    struct Node* left;
    struct Node* right;
} Node;

typedef struct {
    Node* root;
    int next_id;
    char name[32]; // Ajouter un champ pour le nom de la table
} Table;

#endif //BTREE_H
