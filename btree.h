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

typedef struct Table {
    Node* root;
    int next_id;
    char name[32];
} Table;

Node* insert_into_btree(Node* root, Row new_row);
void print_btree(Node* root);

#endif //BTREE_H