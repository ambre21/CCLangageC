#ifndef STORAGE_H
#define STORAGE_H

#include "db.h"

// Déclare les fonctions de sauvegarde et de chargement
void save_db(Db* db, const char* filename);
void load_db(Db* db, const char* filename);

#endif // STORAGE_H
