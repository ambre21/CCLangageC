// Pour la gestion de la sauvegarde et du chargement de la sauvegarde
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "storage.h"
#include "btree.h" // Si nécessaire pour manipuler les arbres binaires
#include "utils.h" // Pour trim_whitespace et my_strdup

// Sauvegarder la base de données
void save_db(Db* db, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf("Erreur : Impossible d'ouvrir le fichier '%s' pour écriture.\n", filename);
        return;
    }

    fprintf(file, "DATABASE_START\n");

    TableNode* current_table = db->first;
    while (current_table != NULL) {
        fprintf(file, "TABLE %s\n", current_table->name);

        Table* table = current_table->table;

        // Sauvegarde des colonnes
        fprintf(file, "COLUMNS ");
        for (int i = 0; i < table->num_columns; i++) {
            fprintf(file, "%s", table->columns[i].name);
            if (i < table->num_columns - 1) fprintf(file, ", ");
        }
        fprintf(file, "\n");

        // Sauvegarde des lignes dans l'arbre binaire
        void save_btree(Node* root, FILE* file, int num_columns) {
            if (root == NULL) return;

            save_btree(root->left, file, num_columns);

            fprintf(file, "ROW %d", root->data.id);
            for (int i = 0; i < num_columns; i++) {
                fprintf(file, ", %s", root->data.values[i] ? root->data.values[i] : "<NULL>");
            }
            fprintf(file, "\n");

            save_btree(root->right, file, num_columns);
        }
        save_btree(table->root, file, table->num_columns);

        fprintf(file, "END_TABLE\n");
        current_table = current_table->next;
    }

    fprintf(file, "DATABASE_END\n");

    fclose(file);
    printf("Base de données sauvegardée dans '%s'.\n", filename);
}

// Charger la base de données
void load_db(Db* db, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Erreur : Impossible d'ouvrir le fichier '%s' pour lecture.\n", filename);
        return;
    }

    char line[1024];
    Table* current_table = NULL;
    while (fgets(line, sizeof(line), file)) {
        trim_whitespace(line);

        if (strcmp(line, "DATABASE_START") == 0) {
            continue;
        } else if (strncmp(line, "TABLE ", 6) == 0) {
            // Création de la table
            char* table_name = line + 6;
            create_table(db, table_name);
            current_table = db->first->table; // Assumer que la table est ajoutée en tête
        } else if (strncmp(line, "COLUMNS ", 8) == 0) {
            // Ajouter les colonnes
            char* columns = line + 8;
            char* col_name = strtok(columns, ",");
            while (col_name != NULL) {
                trim_whitespace(col_name);
                add_column(db, current_table->name, col_name);
                col_name = strtok(NULL, ",");
            }
        } else if (strncmp(line, "ROW ", 4) == 0) {
            // Ajouter une ligne
            char* row_data = line + 4;
            int id;
            char* values[100];
            int num_values = 0;

            char* token = strtok(row_data, ",");
            sscanf(token, "%d", &id); // Lire l'ID
            token = strtok(NULL, ",");

            while (token != NULL) {
                trim_whitespace(token);
                values[num_values++] = my_strdup(token);
                token = strtok(NULL, ",");
            }

            // Insérer la ligne dans l'arbre
            Row new_row = {.id = id, .values = calloc(num_values, sizeof(char*))};
            for (int i = 0; i < num_values; i++) {
                new_row.values[i] = values[i];
            }
            current_table->root = insert_into_btree(current_table->root, new_row, current_table->num_columns);
        } else if (strcmp(line, "END_TABLE") == 0) {
            current_table = NULL;
        } else if (strcmp(line, "DATABASE_END") == 0) {
            break;
        }
    }

    fclose(file);
    printf("Base de données chargée depuis '%s'.\n", filename);
}
