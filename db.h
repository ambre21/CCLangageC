
#ifndef DB_H
#define DB_H

// Déclaration anticipée
struct Table;

// Table de la db
typedef struct TableNode {
    char name[32];
    struct Table* table;  // Pointeur vers Table
    struct TableNode* next;
} TableNode;

// DB complète
typedef struct {
    TableNode* first;         // Liste de toutes les tables
    struct Table* current_table;  // Table actuellement sélectionnée
    int nb_tables;            // Nombre de tables
} Db;

void create_table(Db* db, const char* name);
void select_table(Db* db, const char* name);
void list_tables(const Db* db);
void init_db(Db* db);
void free_db(Db* db);
void add_column(Db* db, const char* table_name, const char* column_name);


#endif