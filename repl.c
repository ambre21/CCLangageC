//Boucle REPL : Read-Eval-Print Loop

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   // Pour ssize_t
#include "repl.h"  // pour le lien avec l'entête

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
  STATEMENT_SELECT
} StatementType;


//*****Structure
typedef struct { // représente une commande sql
  StatementType type;
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
    printf(".exit    - Quitter le programme\n");
    printf(".help    - Afficher cette aide\n");
    printf("insert   - Insérer un nouvel enregistrement\n");
    printf("select   - Afficher tous les enregistrements\n");
    // Ajouter suppression ? Création de data base ? Création de table ?
    return META_COMMAND_SUCCESS;
  } else {
    return META_COMMAND_UNRECOGNIZED_COMMAND;
  }
}

//*****Commandes SQL (préparation & exécution)
PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement) { //identifie le type de commande (insert, select...)

  if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
    statement->type = STATEMENT_INSERT;
    return PREPARE_SUCCESS;
  }
  if (strcmp(input_buffer->buffer, "select") == 0) {
    statement->type = STATEMENT_SELECT;
    return PREPARE_SUCCESS;
  }

  return PREPARE_UNRECOGNIZED_STATEMENT;
}

void execute_statement(Statement* statement) { //exécute la commande sql
  switch (statement->type) {
    case (STATEMENT_INSERT):
    //TODO Implement the command here
      break;
    case (STATEMENT_SELECT):
      //TODO implement the command here
      break;
  }
}


//*****Boucle REPL
void repl(void){ //interraction avec le user
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
     execute_statement(&statement); // exécute la commande si possible
     printf("Executed.\n");
  }
}
