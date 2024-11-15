// Gestion des arbres binaires / listes chainées pour les bdd et les tables

#include <stdio.h>
#include <stdlib.h>
#include "btree.h" // pour le lien avec le fichier entête

Node* insert_into_btree(Node* root, Row data, int num_columns) {
    if (root == NULL) {
        printf("Création d'un nouveau nœud avec ID = %d\n", data.id);
        Node* new_node = (Node*)malloc(sizeof(Node));
        if (new_node == NULL) {
            printf("Erreur d'allocation mémoire pour le nouveau nœud.\n");
            exit(EXIT_FAILURE);
        }

        // Allouer de la mémoire pour les valeurs de la ligne
        new_node->data.id = data.id;
        new_node->data.values = calloc(num_columns, sizeof(char*));
        if (new_node->data.values == NULL) {
            printf("Erreur d'allocation mémoire pour les valeurs.\n");
            exit(EXIT_FAILURE);
        }

        // Copier chaque valeur
        for (int i = 0; i < num_columns; i++) {
            if (data.values[i] != NULL) {
                new_node->data.values[i] = strdup(data.values[i]);
                if (new_node->data.values[i] == NULL) {
                    printf("Erreur d'allocation mémoire pour la valeur de la colonne %d.\n", i);
                    exit(EXIT_FAILURE);
                }
            }
        }

        new_node->left = NULL;
        new_node->right = NULL;
        return new_node;
    }

    printf("Comparaison de ID = %d avec ID = %d\n", data.id, root->data.id);

    if (data.id < root->data.id) {
        root->left = insert_into_btree(root->left, data, num_columns);
    } else if (data.id > root->data.id) {
        root->right = insert_into_btree(root->right, data, num_columns);
    } else {
        // ID déjà présent, ne pas insérer
        printf("ID = %d déjà présent dans l'arbre. Ignorer l'insertion.\n", data.id);
    }

    return root;
}

void print_btree(Node* root, Table* table) {
    if (root == NULL) return;

    print_btree(root->left, table);

    printf("ID: %-3d | ", root->data.id);
    for (int i = 0; i < table->num_columns; i++) {
        // Assurer que la valeur n'est pas NULL
        if (root->data.values[i] != NULL) {
            printf("%s: %-20s | ", table->columns[i].name, root->data.values[i]);
        } else {
            printf("%s: <NULL>              | ", table->columns[i].name);
        }
    }
    printf("\n");

    print_btree(root->right, table);
}

