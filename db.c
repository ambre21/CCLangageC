#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "db.h"
#include "btree.h"  //  structure Table

#define MAX_TABLE_NAME_LENGTH 31

//*****Fonctions
void init_db(Db* db) {
    db->first = NULL;
    db->current_table = NULL;
    db->nb_tables = 0;
}

void create_table(Db* db, const char* name) {
    if (strlen(name) > MAX_TABLE_NAME_LENGTH) {
        printf("Erreur : Le nom de la table ne doit pas dépasser %d caractères.\n", MAX_TABLE_NAME_LENGTH);
        return;
    }

    //Vérifie si la table existe déjà
    TableNode* current = db->first;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            printf("Erreur : La table '%s' existe déjà.\n", name);
            return;
        }
        current = current->next;
    }

    // Créer un nouveau noeud
    TableNode* new_table = (TableNode*)malloc(sizeof(TableNode));
    strncpy(new_table->name, name, sizeof(new_table->name) - 1);
    new_table->name[sizeof(new_table->name) - 1] = '\0';

    new_table->table = (Table*)malloc(sizeof(Table));
    if (new_table->table == NULL) {
        printf("Erreur d'allocation mémoire pour la table.\n");
        exit(EXIT_FAILURE);
    }
    new_table->table->root = NULL;
    new_table->table->next_id = 1;
    strncpy(new_table->table->name, name, sizeof(new_table->table->name) - 1);
    new_table->table->name[sizeof(new_table->table->name) - 1] = '\0'; // Assure que le nom est bien terminé
    new_table->next = db->first;
    db->first = new_table;
    db->nb_tables++;


    printf("La table '%s' a été créée avec succès.\n", name);
}


void list_tables(const Db* db) {
    if (db->first == NULL) {
        printf("Il n'y a pour l'instant aucune table dans la base de donnée.\n");
        return;
    }

    printf("Liste de tables existantes :\n");
    TableNode* current = db->first;
    while (current != NULL) {
        printf("- %s\n", current->name);
        current = current->next;
    }
}

// Fonction pour ajouter une colonne à une table
void add_column(Db* db, const char* table_name, const char* column_name) {
    TableNode* current = db->first;

    // Cherche la table spécifiée
    while (current != NULL) {
        if (strcmp(current->name, table_name) == 0) {
            Table* table = current->table;

            // Vérifie si la table peut encore accueillir des colonnes
            if (table->num_columns < 100) {
                // Crée une nouvelle colonne
                Column new_column;
                strncpy(new_column.name, column_name, sizeof(new_column.name) - 1);
                new_column.name[sizeof(new_column.name) - 1] = '\0';

                // Ajoute la nouvelle colonne à la table
                table->columns[table->num_columns++] = new_column;
                printf("Colonne '%s' ajoutée à la table '%s'.\n", column_name, table_name);
            } else {
                printf("Erreur : La table '%s' a atteint le nombre maximum de colonnes.\n", table_name);
            }
            return;
        }
        current = current->next;
    }

    printf("Erreur : La table '%s' n'existe pas.\n", table_name);
}


void free_db(Db* db) {
    TableNode* current = db->first;
    while (current != NULL) {
        TableNode* next = current->next;
        free(current->table);
        free(current);
        current = next;
    }
    db->first = NULL;
    db->current_table = NULL;
    db->nb_tables = 0;
    printf("Mémoire de la base de données libérée avec succès.\n");
}
