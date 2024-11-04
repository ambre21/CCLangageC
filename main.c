//Point d'entré du programme : lance les autres fichiers

#include <stdio.h>
#include <stdlib.h>
#include "repl.h"
#include <stdbool.h>
#include <string.h>


int main(int argc, char* argv[], char* envp[]){
  printf("Bienvenue dans la base de données !\n"); // test pour la création du makefile
  repl();
  return 0;
  
}
