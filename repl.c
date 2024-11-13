//Boucle REPL : Read-Eval-Print Loop
#define _GNU_SOURCE
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   // Pour ssize_t
#include <assert.h>   // pour les assert (code de test)
#include "repl.h"     // pour le lien avec l'entête
#include "db.h"       // pour le lien avec la bdd
#include "btree.h"    // pour les fonctions insert_into_btree et print_btree

//*****Enumération
typedef enum { //résultat des métacommandes (commence par .exit)
  META_COMMAND_SUCCESS,
  META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum { // Résultat d'une préparation de commande sql
  PREPARE_SUCCESS,
  PREPARE_UNRECOGNIZED_STATEMENT
} PrepareResult;

typedef enum { // Type de commande sql possible (peut être compléter)
  STATEMENT_INSERT,
  STATEMENT_SELECT,
  STATEMENT_CREATE_TABLE,
  STATEMENT_LIST_TABLES,
  STATEMENT_ADD_COLUMN
} StatementType;


//*****Structure
typedef struct {
    StatementType type;       // Type de la commande (INSERT, SELECT, etc.)
    char table_name[32];      // Nom de la table (pour CREATE, INSERT, SELECT...)
    char column_names[1024];  // Noms des colonnes pour INSERT (ex : "name, age")
    char values[1024];        // Valeurs correspondantes pour INSERT (ex : "1, 'Alice'")
    char column_name[32];     // Pour ADD COLUMN : nom de la nouvelle colonne
    char column_type[32];     // Pour ADD COLUMN : type de la nouvelle colonne
    char where_condition[1024]; // Pour SELECT (condition WHERE)
} Statement;

typedef struct { //Permet de gérer l'entrée utilisateur
  char* buffer; //pointe vers texte du user
  size_t buffer_length; // longueur du buffer
  ssize_t input_length; // longueur du texte saisi
} InputBuffer;


//*****Fonction
InputBuffer* new_input_buffer() { // initialiser le buffer
  InputBuffer* input_buffer = (InputBuffer*)malloc(sizeof(InputBuffer));
  input_buffer->buffer = NULL;
  input_buffer->buffer_length = 0;
  input_buffer->input_length = 0;

  return input_buffer;
}

void print_prompt() { // affichage user (db > au début de la ligne)
  printf("db > ");
}



void read_input(InputBuffer* input_buffer) { // lit l'entrée user
  ssize_t bytes_read =
      getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);

  if (bytes_read <= 0) {
    printf("Error reading input\n");
    exit(EXIT_FAILURE);
  }

  // Ignore trailing newline
  input_buffer->input_length = bytes_read - 1;
  input_buffer->buffer[bytes_read - 1] = 0;
}


void close_input_buffer(InputBuffer* input_buffer) { // Libère la mémoire du buffer
    free(input_buffer->buffer);
    free(input_buffer);
}


//*****Meta-Commandes
MetaCommandResult do_meta_command(InputBuffer* input_buffer) { // vérifie si le user a entré une métacommande : commence par .
  if (strcmp(input_buffer->buffer, ".exit") == 0) {
    close_input_buffer(input_buffer);
    exit(EXIT_SUCCESS);
  } else if (strcmp(input_buffer->buffer, ".help") == 0) {
    printf("Commandes disponibles :\n");
    printf(".exit         - Quitter le programme\n");
    printf(".help         - Afficher cette aide\n");
    printf("insert into   - Insérer un nouvel enregistrement\n");
    printf("select        - Afficher tous les enregistrements\n");
    // Ajouter suppression ? Création de data base ? Création de table ?
    return META_COMMAND_SUCCESS;
  } else {
    return META_COMMAND_UNRECOGNIZED_COMMAND;
  }
}

//*****Commandes SQL (préparation & exécution)
PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement) {
  // **INSERT INTO**
    if (strncmp(input_buffer->buffer, "insert into", 11) == 0) {
        statement->type = STATEMENT_INSERT;

        // Extraire le nom de la table
        char* table_start = input_buffer->buffer + 12;
        char* table_end = strstr(table_start, " values");
        if (table_end == NULL) {
            return PREPARE_UNRECOGNIZED_STATEMENT;
        }

        size_t table_name_length = table_end - table_start;
        assert(table_name_length > 0 && "Le nom de la table ne peut pas être vide.");
        strncpy(statement->table_name, table_start, table_name_length);
        statement->table_name[table_name_length] = '\0';

        // Vérification si le nom de la table est valide
        assert(strlen(statement->table_name) > 0 && "Le nom de la table est invalide.");

        // Extraire les colonnes entre parenthèses après le nom de la table
        char* cols_start = strchr(table_end, '(') + 1; // Après "values("
        char* cols_end = strchr(cols_start, ')');  // Avant ")"
        assert(cols_start != NULL && "Erreur de format, colonnes manquantes.");
        assert(cols_end != NULL && "Erreur de format, parenthèses manquantes.");
        size_t cols_length = cols_end - cols_start;
        strncpy(statement->column_names, cols_start, cols_length);
        statement->column_names[cols_length] = '\0';

        // Vérification des colonnes
        assert(strlen(statement->column_names) > 0 && "Les noms de colonnes ne peuvent pas être vides.");

        // Extraire les valeurs après "values"
        char* vals_start = strchr(cols_end, '(') + 1; // Après "("
        char* vals_end = strchr(vals_start, ')');  // Avant ")"
        assert(vals_start != NULL && "Erreur de format, valeurs manquantes.");
        assert(vals_end != NULL && "Erreur de format, parenthèses manquantes.");
        size_t vals_length = vals_end - vals_start;
        strncpy(statement->values, vals_start, vals_length);
        statement->values[vals_length] = '\0';

        // Vérification des valeurs
        assert(strlen(statement->values) > 0 && "Les valeurs ne peuvent pas être vides.");

        // Vérification de la correspondance entre les colonnes et les valeurs
        int num_columns = 0, num_values = 0;
        char* col_token = strtok(statement->column_names, ",");
        while (col_token != NULL) {
            num_columns++;
            col_token = strtok(NULL, ",");
        }
        char* val_token = strtok(statement->values, ",");
        while (val_token != NULL) {
            num_values++;
            val_token = strtok(NULL, ",");
        }

        assert(num_columns == num_values && "Le nombre de colonnes ne correspond pas au nombre de valeurs.");

        return PREPARE_SUCCESS;
    }

    //**SELECT
    if (strncmp(input_buffer->buffer, "select", 6) == 0) {
        statement->type = STATEMENT_SELECT;

        // Extraire les colonnes entre "select" et "from"
        char* cols_start = input_buffer->buffer + 7;
        char* from_pos = strstr(cols_start, "from");
        if (from_pos != NULL) {
            size_t len_cols = from_pos - cols_start;
            strncpy(statement->column_names, cols_start, len_cols);
            statement->column_names[len_cols] = '\0';

            // Extraire la table après "from"
            char* table_start = from_pos + 5;
            strncpy(statement->table_name, table_start, strlen(table_start));
            statement->table_name[strlen(table_start)] = '\0';

            // Vérification de WHERE
            char* where_pos = strstr(table_start, "where");
            if (where_pos != NULL) {
                strcpy(statement->where_condition, where_pos + 6); // après "where "
            }
        }

        return PREPARE_SUCCESS;
    }

    //**Create Table
    if (strncmp(input_buffer->buffer, "create table", 12) == 0) {
        statement->type = STATEMENT_CREATE_TABLE;
        char* table_name = input_buffer->buffer + 13;
        strncpy(statement->table_name, table_name, sizeof(statement->table_name) - 1);
        statement->table_name[sizeof(statement->table_name) - 1] = '\0';
        return PREPARE_SUCCESS;
    }

    //**List Tables
    if (strcmp(input_buffer->buffer, "list tables") == 0) {
        statement->type = STATEMENT_LIST_TABLES;
        return PREPARE_SUCCESS;
    }

    //**Add Column
    if (strncmp(input_buffer->buffer, "add column", 10) == 0) {
        statement->type = STATEMENT_ADD_COLUMN;

        // Extraction du nom de la table, de la colonne et du type
        char* table_start = input_buffer->buffer + 11; // Après "add column "
        char* column_pos = strstr(table_start, " "); // Cherche l'espace entre la table et la colonne
        if (column_pos != NULL) {
            size_t len_table = column_pos - table_start;
            strncpy(statement->table_name, table_start, len_table);
            statement->table_name[len_table] = '\0';

            // Extraction du nom de la colonne et du type
            char* column_start = column_pos + 1; // Après l'espace commence le nom de la colonne
            char* type_pos = strstr(column_start, " "); // Cherche l'espace entre la colonne et le type
            if (type_pos != NULL) {
                size_t len_column = type_pos - column_start;
                strncpy(statement->column_name, column_start, len_column);
                statement->column_name[len_column] = '\0';
                strcpy(statement->column_type, type_pos + 1); // Le type est après l'espace
            }
        }

        return PREPARE_SUCCESS;
    }

    return PREPARE_UNRECOGNIZED_STATEMENT;
}


void execute_statement(Statement* statement, Db* db) {
   TableNode* current;
   switch (statement->type) {
     case STATEMENT_CREATE_TABLE:
       create_table(db, statement->table_name);
     break;

     case STATEMENT_LIST_TABLES:
       list_tables(db);
     break;

     case STATEMENT_INSERT:
            // Trouver la table
            current = db->first;
            while (current != NULL) {
                if (strcmp(current->name, statement->table_name) == 0) {
                    db->current_table = current->table;
                    break;
                }
                current = current->next;
            }

            // Assertion pour vérifier que la table existe
            assert(db->current_table != NULL && "La table spécifiée n'existe pas.");

            // Créer une nouvelle ligne
            Row new_row;
            new_row.id = db->current_table->next_id++;

            // Assigner les valeurs aux colonnes spécifiées
            // Exemple simple pour gérer la colonne "name"
            if (strstr(statement->column_names, "name") != NULL) {
                // Extraire la valeur de 'name' et l'assigner à 'new_row.name'
                char* value_name = strtok(statement->values, ",");
                if (value_name != NULL) {
                    value_name = value_name + 1;  // Enlève les guillemets ou espaces
                    value_name[strlen(value_name) - 1] = '\0'; // Enlève le dernier caractère
                    strncpy(new_row.name, value_name, sizeof(new_row.name) - 1);
                }
            }

            // Insérer dans l'arbre binaire
            db->current_table->root = insert_into_btree(db->current_table->root, new_row);
            break;

     case STATEMENT_SELECT:
       // Vérifie qu'une table a été sélectionnée
       assert(db->current_table != NULL && "Erreur : Aucune table sélectionnée.");

       // Vérifie que l'arbre binaire n'est pas NULL
       assert(db->current_table->root != NULL && "Erreur : L'arbre binaire de la table est vide.");

       printf("Contenu de la table '%s' :\n", db->current_table->name);
       print_btree(db->current_table->root);
     break;

      case STATEMENT_ADD_COLUMN:
            // Trouver la table à laquelle ajouter la colonne
            current = db->first;
            while (current != NULL) {
                if (strcmp(current->name, statement->table_name) == 0) {
                    // Si la table existe, ajout de la colonne
                    Table* table = current->table;
                    Column new_column;
                    strncpy(new_column.name, statement->column_name, sizeof(new_column.name) - 1);
                    new_column.name[sizeof(new_column.name) - 1] = '\0';
                    strncpy(new_column.type, statement->column_type, sizeof(new_column.type) - 1);
                    new_column.type[sizeof(new_column.type) - 1] = '\0';

                    // Ajouter la colonne à la table
                    if (table->num_columns < 100) {
                        table->columns[table->num_columns++] = new_column;
                        printf("Colonne '%s' de type '%s' ajoutée à la table '%s'.\n", new_column.name, new_column.type, statement->table_name);
                    } else {
                        printf("Erreur : Le nombre maximum de colonnes est atteint pour la table '%s'.\n", statement->table_name);
                    }
                    break;
                }
                current = current->next;
            }

            // Si la table n'a pas été trouvée
            if (current == NULL) {
                printf("Erreur : La table '%s' n'existe pas.\n", statement->table_name);
            }
            break;

   }
}


//*****Boucle REPL
void repl(void){ //interraction avec le user
  Db db;
  init_db(&db);

  InputBuffer* input_buffer = new_input_buffer();
  while (true) {
    print_prompt(); // affiche invite de commande
    read_input(input_buffer); //lit entrée user
    if (input_buffer->buffer[0] == '.') {  // vérification si meta-commande
      switch (do_meta_command(input_buffer)) {
        case META_COMMAND_SUCCESS:
          continue;
        case META_COMMAND_UNRECOGNIZED_COMMAND:
          printf("Commande non reconnue : '%s'\n", input_buffer->buffer);
          continue;
      }
    }
    Statement statement;
    switch (prepare_statement(input_buffer, &statement)) { // voit si reconnait commande sql
      case PREPARE_SUCCESS:
        printf("recognized statement\n");
        break;
      case PREPARE_UNRECOGNIZED_STATEMENT:
        printf("Unrecognized keyword at start of '%s'.\n",
               input_buffer->buffer);
        continue;
    }
     execute_statement(&statement, &db); // exécute la commande si possible
     printf("Executed.\n");
  }

  //Libère la mémoire proprement
  free_db(&db);
  close_input_buffer(input_buffer);
}