// Gestion des arbres binaires / listes chainées pour les bdd et les tables

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

void print_btree_with_columns(Node* root, Table* table, int* column_indices, int num_columns, const char* where_condition) {
    if (root == NULL) return;

    print_btree_with_columns(root->left, table, column_indices, num_columns, where_condition);

    // Appliquer la condition WHERE (si elle existe)
    if (where_condition != NULL && strlen(where_condition) > 0) {
        // Exemple simple : WHERE sur une seule colonne "name = 'Alice'"
        char col_name[32], col_value[128];
        sscanf(where_condition, "%31[^=]=%127s", col_name, col_value);

        // Nettoyer les guillemets et les espaces
        trim_whitespace(col_name);
        trim_whitespace(col_value);

        // Travailler avec un pointeur intermédiaire pour modifier la chaîne
        char* value_ptr = col_value;
        if (*value_ptr == '\'') value_ptr++; // Retirer le guillemet d'ouverture
        char* end = value_ptr + strlen(value_ptr) - 1;
        if (*end == '\'') *end = '\0'; // Retirer le guillemet de fermeture

        int col_index = -1;
        for (int i = 0; i < table->num_columns; i++) {
            if (strcmp(table->columns[i].name, col_name) == 0) {
                col_index = i;
                break;
            }
        }

        if (col_index == -1 || strcmp(root->data.values[col_index], value_ptr) != 0) {
            // Colonne inexistante ou condition non satisfaite
            print_btree_with_columns(root->right, table, column_indices, num_columns, where_condition);
            return;
        }
    }

    // Afficher les colonnes sélectionnées
    printf("ID: %-3d | ", root->data.id);
    for (int i = 0; i < num_columns; i++) {
        int col_index = column_indices[i];
        if (root->data.values[col_index] != NULL) {
            printf("%s: %-20s | ", table->columns[col_index].name, root->data.values[col_index]);
        } else {
            printf("%s: <NULL>              | ", table->columns[col_index].name);
        }
    }
    printf("\n");

    print_btree_with_columns(root->right, table, column_indices, num_columns, where_condition);
}


