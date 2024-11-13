// Gestion des arbres binaires / listes chainées pour les bdd et les tables

#include <stdio.h>
#include <stdlib.h>
#include "btree.h" // pour le lien avec le fichier entête

Node* insert_into_btree(Node* root, Row data) {
  printf("test");
    if (root == NULL) {
        printf("Création d'un nouveau nœud avec ID = %d, Nom = %s\n", data.id, data.name);
        Node* new_node = (Node*)malloc(sizeof(Node));
        if (new_node == NULL) {
            printf("Erreur d'allocation mémoire pour le nouveau nœud.\n");
            exit(EXIT_FAILURE);
        }
        new_node->data = data;
        new_node->left = NULL;
        new_node->right = NULL;
        return new_node;
    }

    // Afficher l'état actuel de l'arbre avant de décider où insérer
    printf("Comparaison de ID = %d avec ID = %d\n", data.id, root->data.id);

    if (data.id < root->data.id) {
        printf("Insertion de ID = %d dans le sous-arbre gauche de ID = %d\n", data.id, root->data.id);
        root->left = insert_into_btree(root->left, data);
    } else if (data.id > root->data.id) {
        printf("Insertion de ID = %d dans le sous-arbre droit de ID = %d\n", data.id, root->data.id);
        root->right = insert_into_btree(root->right, data);
    } else {
        // ID déjà présent, ne pas insérer
        printf("ID = %d déjà présent dans l'arbre. Ignorer l'insertion.\n", data.id);
    }

    return root;
}


void print_btree(Node* root) {
    if (root == NULL) return;
    print_btree(root->left);
    // Assurer que le champ name n'est pas NULL
    if (root->data.name != NULL) {
        printf("ID: %-3d | Nom: %-20s\n", root->data.id, root->data.name);
    } else {
        printf("ID: %-3d | Nom: <NULL>\n", root->data.id);
    }
    print_btree(root->right);
}
