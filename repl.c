//Boucle REPL : Read-Eval-Print Loop

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   // Pour ssize_t
#include "repl.h"  // pour le lien avec l'entête
#include "db.h"  // pour le lien avec la bdd

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
  STATEMENT_LIST_TABLES
} StatementType;


//*****Structure
typedef struct { // représente une commande sql
  StatementType type;
  char table_name[32];  // pour CREATE TABLE et USE
  char row_name[32];    // pour INSERT
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
PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement) { //identifie le type de commande (insert, select...)

  //**Insert
  if (strncmp(input_buffer->buffer, "insert into", 11) == 0) {
    statement->type = STATEMENT_INSERT;

    // Extraire le nom de la table
    char* table_start = input_buffer->buffer + 12;
    char* table_end = strstr(table_start, " values");
    if (table_end == NULL) {
      return PREPARE_UNRECOGNIZED_STATEMENT;
    }

    // Copier le nom de la table
    size_t table_name_length = table_end - table_start;
    if (table_name_length > sizeof(statement->table_name) - 1) {
      return PREPARE_UNRECOGNIZED_STATEMENT;
    }
    strncpy(statement->table_name, table_start, table_name_length);
    statement->table_name[table_name_length] = '\0';

    // Extraire la valeur entre parenthèses
    char* value_start = strchr(table_end, '(');
    char* value_end = strchr(table_end, ')');
    if (value_start == NULL || value_end == NULL || value_end <= value_start) {
      return PREPARE_UNRECOGNIZED_STATEMENT;
    }

    // Copier le nom de la ligne
    size_t row_name_length = value_end - value_start - 1;
    if (row_name_length > sizeof(statement->row_name) - 1) {
      return PREPARE_UNRECOGNIZED_STATEMENT;
    }
    strncpy(statement->row_name, value_start + 1, row_name_length);
    statement->row_name[row_name_length] = '\0';

    return PREPARE_SUCCESS;
  }

  //**Select
  if (strcmp(input_buffer->buffer, "select") == 0) {
    statement->type = STATEMENT_SELECT;
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
  return PREPARE_UNRECOGNIZED_STATEMENT;
}

void execute_statement(Statement* statement, Db* db) { //exécute la commande sql
   switch (statement->type) {
     case (STATEMENT_CREATE_TABLE):
       create_table(db, statement->table_name);
     break;

     case (STATEMENT_LIST_TABLES):
       list_tables(db);
     break;

     case (STATEMENT_INSERT):
       // Sélectionne la table à insérer
       TableNode* current = db->first;
       while (current != NULL) {
         if (strcmp(current->name, statement->table_name) == 0) {
           db->current_table = current->table;
           break;
         }
         current = current->next;
       }

       if (db->current_table == NULL) {
         printf("Erreur : La table '%s' n'existe pas.\n", statement->table_name);
         break;
       } else {
         printf("Table courante sélectionnée : %s\n", db->current_table->name);
       }

       // Crée un nouvel enregistrement avec un ID unique
       Row new_row;
       new_row.id = db->current_table->next_id++;
       strncpy(new_row.name, statement->row_name, sizeof(new_row.name) - 1);
       new_row.name[sizeof(new_row.name) - 1] = '\0';

       // Insère la ligne dans l'arbre binaire de la table
     printf("Avant l'insertion de ID = %d, Nom = %s dans l'arbre.\n", new_row.id, new_row.name);
     db->current_table->root = insert_into_btree(db->current_table->root, new_row);
     printf("Après l'insertion de ID = %d.\n", new_row.id);

     break;

     case (STATEMENT_SELECT):
       if (db->current_table == NULL) {
         printf("Erreur : Aucune table sélectionnée. Utilisez la commande USE pour sélectionner une table.\n");
         break;
       }
       printf("Contenu de la table sélectionnée :\n");
       print_btree(db->current_table->root);
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
        case (META_COMMAND_SUCCESS):
          continue;
        case (META_COMMAND_UNRECOGNIZED_COMMAND):
          printf("Commande non reconnue : '%s'\n", input_buffer->buffer);
          continue;
      }
    }
    Statement statement;
    switch (prepare_statement(input_buffer, &statement)) { // voit si reconnait commande sql
      case (PREPARE_SUCCESS):
        printf("recognized statement\n");
        break;
      case (PREPARE_UNRECOGNIZED_STATEMENT):
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
