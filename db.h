#ifndef DB_H
#define DB_H

#include "btree.h"  // Pour accéder à la structure Table

// Table de la db
typedef struct TableNode {
    char name[32];
    Table* table;
    struct TableNode* next;
} TableNode;

// DB complète
typedef struct {
    TableNode* first;
    Table* current_table;
    int nb_tables;
} Db;


void create_table(Db* db, const char* name);
void select_table(Db* db, const char* name);
void list_tables(const Db* db);
void init_db(Db* db);
void free_db(Db* db);

#endif
