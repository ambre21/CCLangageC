//Boucle REPL : Read-Eval-Print Loop
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "repl.h"
#include "db.h"
#include "btree.h"
#include "utils.h"
#include "storage.h"

//*****Enumération
typedef enum { //résultat des métacommandes (commence par .exit)
  META_COMMAND_SUCCESS,
  META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum { // Résultat d'une préparation de commande sql
  PREPARE_SUCCESS,
  PREPARE_UNRECOGNIZED_STATEMENT
} PrepareResult;

typedef enum { // Type de commande sql possible
  STATEMENT_INSERT,
  STATEMENT_SELECT,
  STATEMENT_CREATE_TABLE,
  STATEMENT_LIST_TABLES,
  STATEMENT_ADD_COLUMN
} StatementType;


//*****Structure
typedef struct {
    StatementType type;       	// Type de la commande (INSERT, SELECT, etc.)
    char table_name[32];      	// Nom de la table (pour CREATE, INSERT, SELECT...)
    char column_names[1024];  	// Noms des colonnes pour INSERT (ex : "name, age")
    char values[1024];        	// Valeurs correspondantes pour INSERT (ex : "1, 'Alice'")
    char column_name[32];     	// Pour ADD COLUMN : nom de la nouvelle colonne
} Statement;

typedef struct { 		 //Permet de gérer l'entrée utilisateur
  char* buffer; 		 //pointe vers texte du user
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
MetaCommandResult do_meta_command(InputBuffer* input_buffer, Db* db) { // vérifie si le user a entré une métacommande : commence par .
	if (strcmp(input_buffer->buffer, ".exit") == 0) {
    	close_input_buffer(input_buffer);
    	exit(EXIT_SUCCESS);
  	} else if (strcmp(input_buffer->buffer, ".help") == 0) {
    	printf("Commandes disponibles :\n");
    	printf(".exit 																- Quitter le programme\n");
    	printf(".help 																- Afficher cette aide\n");
    	printf("insert into <table> (<colonne1>,<colonne2>) values (value1,value2)	- Insérer un nouvel enregistrement\n");
    	printf("select <colones> from <table>										- Afficher tous les enregistrements\n");
    	printf("create table <table_name> 											- Création d'une nouvelle table\n");
    	printf("add column <table_name> <column_name> 								- Ajouter une colonne dans une table\n");
    	printf("list tables 														- Lister toutes les tables de la base de donnée\n");
    	return META_COMMAND_SUCCESS;
  	} else if (strncmp(input_buffer->buffer, ".save", 5) == 0) {
    	char* filename = input_buffer->buffer + 6;
    	trim_whitespace(filename);
    	save_db(db, filename);
    	return META_COMMAND_SUCCESS;
 	} else if (strncmp(input_buffer->buffer, ".load", 5) == 0) {
    	char* filename = input_buffer->buffer + 6;
    	trim_whitespace(filename);
    	load_db(db, filename);
    	return META_COMMAND_SUCCESS;
	} else {
    	return META_COMMAND_UNRECOGNIZED_COMMAND;
  	}
}


//*****Commandes SQL (préparation & exécution)
PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement) {
    // **INSERT INTO
    if (strncmp(input_buffer->buffer, "insert into", 11) == 0) {
        statement->type = STATEMENT_INSERT;

        // Extraire le nom de la table
        char* table_start = input_buffer->buffer + 11;
        while (isspace((unsigned char)*table_start)) table_start++; // Ignorer les espaces

        char* table_end = strstr(table_start, " (");
        if (table_end == NULL) {
            printf("Erreur de format : parenthèse ouvrante manquante pour les colonnes.\n");
            return PREPARE_UNRECOGNIZED_STATEMENT;
        }

        size_t table_name_length = table_end - table_start;
        if (table_name_length == 0 || table_name_length >= sizeof(statement->table_name)) {
            printf("Erreur : Le nom de la table est invalide ou trop long.\n");
            return PREPARE_UNRECOGNIZED_STATEMENT;
        }
        strncpy(statement->table_name, table_start, table_name_length);
        statement->table_name[table_name_length] = '\0';
        // Supprime les espaces autour du nom de la table
        trim_whitespace(statement->table_name);

        // Extraire les colonnes entre parenthèses après le nom de la table
        char* cols_start = strchr(table_end, '(');
        char* cols_end = strchr(cols_start, ')');
        if (cols_start == NULL || cols_end == NULL) {
            printf("Erreur de format : Parenthèses manquantes pour les colonnes.\n");
            return PREPARE_UNRECOGNIZED_STATEMENT;
        }
        size_t cols_length = cols_end - cols_start - 1;
        strncpy(statement->column_names, cols_start + 1, cols_length);
        statement->column_names[cols_length] = '\0';
        trim_whitespace(statement->column_names);

        // Extraire les valeurs entre parenthèses après "values"
        char* values_keyword = strstr(cols_end, "values");
        if (values_keyword == NULL) {
            printf("Erreur de format : 'values' manquant.\n");
            return PREPARE_UNRECOGNIZED_STATEMENT;
        }

        char* vals_start = strchr(values_keyword, '(');
        char* vals_end = strchr(vals_start, ')');
        if (vals_start == NULL || vals_end == NULL) {
            printf("Erreur de format : Parenthèses manquantes pour les valeurs.\n");
            return PREPARE_UNRECOGNIZED_STATEMENT;
        }
        size_t vals_length = vals_end - vals_start - 1;
        strncpy(statement->values, vals_start + 1, vals_length);
        statement->values[vals_length] = '\0';
        trim_whitespace(statement->values);

        // Vérification de la correspondance entre les colonnes et les valeurs
        // Copie des chaînes pour les séparer en tokens
        char cols_copy[1024];
        char vals_copy[1024];
        strncpy(cols_copy, statement->column_names, sizeof(cols_copy) - 1);
        cols_copy[sizeof(cols_copy) - 1] = '\0';
        strncpy(vals_copy, statement->values, sizeof(vals_copy) - 1);
        vals_copy[sizeof(vals_copy) - 1] = '\0';

        int num_columns = 0, num_values = 0;
        char* col_token = strtok(cols_copy, ",");
        while (col_token != NULL) {
            num_columns++;
            col_token = strtok(NULL, ",");
        }
        char* val_token = strtok(vals_copy, ",");
        while (val_token != NULL) {
            num_values++;
            val_token = strtok(NULL, ",");
        }

        if (num_columns != num_values) {
            printf("Erreur : Le nombre de colonnes ne correspond pas au nombre de valeurs.\n");
            return PREPARE_UNRECOGNIZED_STATEMENT;
        }

        return PREPARE_SUCCESS;
    }

	//**Select
    if (strncmp(input_buffer->buffer, "select", 6) == 0) {
    	statement->type = STATEMENT_SELECT;

    	// Extraire les colonnes
    	char* cols_start = input_buffer->buffer + 7;
    	char* from_pos = strstr(cols_start, "from");
    	if (from_pos == NULL) {
        	printf("Erreur de format : 'from' manquant dans la commande SELECT.\n");
        	return PREPARE_UNRECOGNIZED_STATEMENT;
    	}

    	size_t len_cols = from_pos - cols_start;
    	strncpy(statement->column_names, cols_start, len_cols);
    	statement->column_names[len_cols] = '\0';
    	trim_whitespace(statement->column_names);

    	// Extraire le nom de la table
    	char* table_start = from_pos + 5;
    	strncpy(statement->table_name, table_start, sizeof(statement->table_name) - 1);
		statement->table_name[sizeof(statement->table_name) - 1] = '\0';
		trim_whitespace(statement->table_name);
    	return PREPARE_SUCCESS;
	}


    // **CREATE TABLE
    if (strncmp(input_buffer->buffer, "create table", 12) == 0) {
        statement->type = STATEMENT_CREATE_TABLE;
        char* table_name = input_buffer->buffer + 13;
        strncpy(statement->table_name, table_name, sizeof(statement->table_name) - 1);
        statement->table_name[sizeof(statement->table_name) - 1] = '\0';
        return PREPARE_SUCCESS;
    }

    // **LIST TABLES**
    if (strcmp(input_buffer->buffer, "list tables") == 0) {
        statement->type = STATEMENT_LIST_TABLES;
        return PREPARE_SUCCESS;
    }

    // **ADD COLUMN**
    if (strncmp(input_buffer->buffer, "add column", 10) == 0) {
    	statement->type = STATEMENT_ADD_COLUMN;

    	// Extraire le nom de la table
    	char* table_start = input_buffer->buffer + 11;
    	char* column_pos = strstr(table_start, " ");
    	if (column_pos == NULL) {
        	printf("Erreur de format : nom de colonne manquant.\n");
        	return PREPARE_UNRECOGNIZED_STATEMENT;
    	}

    	size_t len_table = column_pos - table_start;
    	strncpy(statement->table_name, table_start, len_table);
    	statement->table_name[len_table] = '\0';

    	// Extraire le nom de la colonne
    	char* column_start = column_pos + 1;
    	strncpy(statement->column_name, column_start, sizeof(statement->column_name) - 1);
    	statement->column_name[sizeof(statement->column_name) - 1] = '\0';

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
    		db->current_table = NULL;

   			while (current != NULL) {
        		if (strcmp(current->name, statement->table_name) == 0) {
            		db->current_table = current->table;
            		break;
        		}
        		current = current->next;
    		}

    		// Vérifier que la table existe
    		if (db->current_table == NULL) {
        		printf("Erreur : La table '%s' n'existe pas.\n", statement->table_name);
        		return;
    		}

    		// Vérifier que les colonnes spécifiées sont valides
    		char cols_copy[1024];
    		strncpy(cols_copy, statement->column_names, sizeof(cols_copy) - 1);
    		cols_copy[sizeof(cols_copy) - 1] = '\0';

    		int column_indices[100];
    		int num_specified_columns = 0;
    		char* col_token = strtok(cols_copy, ",");
    		while (col_token != NULL) {
        		trim_whitespace(col_token);

        		int found = 0;
        		for (int i = 0; i < db->current_table->num_columns; i++) {
            		if (strcmp(db->current_table->columns[i].name, col_token) == 0) {
                		column_indices[num_specified_columns++] = i;
                		found = 1;
                		break;
            		}
        		}

        		if (!found) {
            		printf("Erreur : Colonne '%s' non trouvée dans la table '%s'.\n", col_token, statement->table_name);
           			return;
        		}

        		col_token = strtok(NULL, ",");
    		}

    		// Créer une nouvelle ligne avec des valeurs dynamiques
    		Row new_row;
    		new_row.id = db->current_table->next_id++;
   			new_row.values = calloc(db->current_table->num_columns, sizeof(char*));

    		// Extraire les valeurs et les insérer dans les champs correspondants
    		char vals_copy[1024];
    		strncpy(vals_copy, statement->values, sizeof(vals_copy) - 1);
    		vals_copy[sizeof(vals_copy) - 1] = '\0';

    		char* val_token = strtok(vals_copy, ",");
    		for (int i = 0; i < num_specified_columns; i++) {
          		if (val_token == NULL) {
              		printf("Erreur : Nombre de valeurs inférieur au nombre de colonnes spécifiées.\n");
              		free(new_row.values);
              		return;
          		}

          		trim_whitespace(val_token);
          		if (*val_token == '\'') val_token++;  // Retirer les guillemets
          		char* end = val_token + strlen(val_token) - 1;
          		if (*end == '\'') *end = '\0';

          		// Sauvegarder la valeur pour la colonne correspondante
          		int col_index = column_indices[i];
          		new_row.values[col_index] = my_strdup(val_token); // Allouer et copier la valeur

          		val_token = strtok(NULL, ",");
      		}

    		// Vérifier qu'il n'y a pas de valeurs supplémentaires
    		if (val_token != NULL) {
        		printf("Erreur : Nombre de valeurs supérieur au nombre de colonnes spécifiées.\n");
        		for (int i = 0; i < db->current_table->num_columns; i++) {
            		free(new_row.values[i]);
        		}
        		free(new_row.values);
        		return;
    		}

    		// Insérer dans l'arbre binaire
    		db->current_table->root = insert_into_btree(db->current_table->root, new_row, db->current_table->num_columns);
    		printf("Ligne insérée avec succès dans la table '%s'.\n", db->current_table->name);
    		break;

        case STATEMENT_SELECT:
    		// Vérifier la base de données et la liste des tables
    		if (db == NULL || db->first == NULL) {
        		printf("Erreur : La base de données ou la liste des tables est NULL.\n");
       	 		return;
    		}

    		// Trouver la table
    		current = db->first;
    		db->current_table = NULL;

    		while (current != NULL) {
        		if (strcmp(current->name, statement->table_name) == 0) {
            		db->current_table = current->table;
            		break;
        		}
        		current = current->next;
    		}

    		if (db->current_table == NULL) {
        		printf("Erreur : La table '%s' n'existe pas.\n", statement->table_name);
        		return;
    		}

    		// Vérifier que la table a des colonnes
    		if (db->current_table->num_columns <= 0) {
        		printf("Erreur : La table '%s' n'a pas de colonnes définies.\n", statement->table_name);
        		return;
    		}

    		// Vérifier les colonnes spécifiées
    		char cols_copy2[1024];
    		strncpy(cols_copy2, statement->column_names, sizeof(cols_copy2) - 1);
    		cols_copy2[sizeof(cols_copy2) - 1] = '\0';
    		trim_whitespace(cols_copy2);

    		int column_indices2[100];
    		int num_columns_to_select = 0;

    		if (strcmp(cols_copy2, "*") == 0) {
        		// Sélectionner toutes les colonnes
        		for (int i = 0; i < db->current_table->num_columns; i++) {
            		column_indices2[num_columns_to_select++] = i;
        		}
    		} else {
        		// Vérifier et sélectionner les colonnes demandées
        		char* col_token = strtok(cols_copy2, ",");
        		while (col_token != NULL) {
            		trim_whitespace(col_token);
            		int found = 0;
            		for (int i = 0; i < db->current_table->num_columns; i++) {
                		if (strcmp(db->current_table->columns[i].name, col_token) == 0) {
                    		column_indices2[num_columns_to_select++] = i;
                    		found = 1;
                    		break;
                		}
            		}
            		if (!found) {
                		printf("Erreur : Colonne '%s' non trouvée dans la table '%s'.\n", col_token, statement->table_name);
                		return;
            		}
            		col_token = strtok(NULL, ",");
        		}
    		}

    		// Vérifier si des colonnes ont été sélectionnées
    		if (num_columns_to_select <= 0) {
        		printf("Erreur : Aucune colonne valide spécifiée pour la sélection.\n");
        		return;
    		}

    		// Parcourir et afficher les lignes
    		if (db->current_table->root == NULL) {
        		printf("La table '%s' est vide.\n", statement->table_name);
        		return;
    		}

    		printf("Résultat de la table '%s':\n", statement->table_name);
    		print_btree_with_columns(db->current_table->root, db->current_table, column_indices2, num_columns_to_select);
    		break;

        case STATEMENT_ADD_COLUMN:
    		// Trouver la table à laquelle ajouter la colonne
    		current = db->first;
    		while (current != NULL) {
        		if (strcmp(current->name, statement->table_name) == 0) {
            		Table* table = current->table;
            		// Ajout de la colonne
            		Column new_column;
            		strncpy(new_column.name, statement->column_name, sizeof(new_column.name) - 1);
            		new_column.name[sizeof(new_column.name) - 1] = '\0';

            		// Ajouter la colonne à la table
            		if (table->num_columns < 100) {
                		table->columns[table->num_columns++] = new_column;
                		printf("Colonne '%s' ajoutée à la table '%s'.\n", new_column.name, statement->table_name);
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

		default:
    		printf("Erreur : Type de commande non reconnu.\n");
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
      switch (do_meta_command(input_buffer, &db)) {
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