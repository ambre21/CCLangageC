# **Gestionnaire de Base de Données en C**

Un projet éducatif implémentant un système simple de gestion de bases de données. Le projet met en pratique des concepts fondamentaux de programmation en C, tels que les structures, les listes chaînées, et les arbres binaires. Il inclut également des fonctionnalités de persistance des données et des outils de gestion mémoire.

---

## **Créateur**

Ambre LAURENT 3SI1

---

## **Fonctionnalités**

- **Création et gestion de bases de données** :
    - Création de tables.
    - Ajout de colonnes dynamiques.
- **Manipulation des données** :
    - Insertion de lignes dans les tables.
    - Sélection et affichage de données avec support des colonnes spécifiques.
- **Persistance** :
    - Sauvegarde des bases de données dans un fichier texte.
    - Chargement des bases de données à partir d'un fichier.
- **Optimisation mémoire** :
    - Vérification et correction des fuites mémoire avec **Valgrind**.

---

## **Prérequis**

### **Outils nécessaires**
1. **GCC** (Compilateur C) et **Make** (Automatisation de la compilation) :
   ```bash
   sudo apt install build-essential
   ```
2. **Valgrind** (Analyse mémoire, optionnel mais recommandé) :
   ```bash
   sudo apt install valgrind
   ```

---

## **Installation**

1. **Clonez le dépôt** :
   ```bash
   git clone git@github.com:ambre21/CCLangageC.git
   cd CCLangageC
   ```

2. **Compilez le projet** :
   ```bash
   make
   ```

3. **Exécutez le programme** :
   ```bash
   ./cc_langage_c
   ```

4. **Nettoyez les fichiers temporaires** (optionnel) :
   ```bash
   make clean
   ```

---

## **Commandes Disponibles**

### **Commandes SQL**
| Commande                               | Description                                                                 |
|----------------------------------------|-----------------------------------------------------------------------------|
| `create table <table_name>`            | Crée une nouvelle table nommée `<table_name>`.                              |
| `add column <table_name> <column_name>`| Ajoute une colonne `<column_name>` à la table `<table_name>`.               |
| `insert into <table> (cols) values (vals)` | Insère une ligne dans la table. Les colonnes et valeurs doivent correspondre. |
| `select * from <table>`                | Affiche toutes les données de la table `<table>`.                           |
| `select col1, col2 from <table>`       | Affiche uniquement les colonnes `col1` et `col2` de la table `<table>`.     |

### **Commandes Métas**
| Commande       | Description                                  |
|----------------|----------------------------------------------|
| `.save <file>` | Sauvegarde la base de données dans `<file>`. |
| `.load <file>` | Charge une base de données depuis `<file>`.  |
| `.exit`        | Quitte le programme.                         |
| `.help`        | Afficher l'aide                              |

---

## **Détails du Makefile**

Le Makefile est conçu pour être simple et efficace :
- **Compilation modulaire** :
    - Chaque fichier source `.c` est compilé en un fichier objet `.o`.
    - Cela optimise la recompilation en ne reconstruisant que les fichiers modifiés.
- **Choix des drapeaux de compilation** :
  - Pour éviter des options innutiles, seules les options de compilations nécessaires ont été ajoutées au projet
      - `-Wall -Wextra` : Active les avertissements pour un code robuste.
      - `-std=c11` : Utilise la norme C11 pour garantir la compatibilité.
      - `-g` : Ajoute des symboles de débogage nécessaires pour **Valgrind**.
- **Règle `clean`** :
    - Supprime les fichiers objets et l'exécutable, gardant le projet propre.

---

## **Persistance des Données**

### **Format de Sauvegarde**
Les bases de données sont sauvegardées dans un fichier texte au format suivant :
```plaintext
DATABASE_START
TABLE <table_name>
COLUMNS col1, col2, ...
ROW 1, value1, value2, ...
ROW 2, value1, value2, ...
END_TABLE
DATABASE_END
```

### **Commandes de Persistance**
1. **Sauvegarder** :
   ```plaintext
   .save <filename>
   ```
   Sauvegarde la base de données actuelle dans le fichier spécifié.

2. **Charger** :
   ```plaintext
   .load <filename>
   ```
   Charge une base de données depuis le fichier spécifié.

---

## **Analyse Mémoire avec Valgrind**

Valgrind est utilisé pour vérifier les problèmes de gestion mémoire, comme :
- **Fuites de mémoire** (ex. mémoire allouée mais non libérée).
- **Accès non valides** (ex. dépassement d'index).

### **Étapes d'Analyse**
1. Compilez le projet avec les informations de débogage :
   ```bash
   make
   ```

2. Exécutez le programme avec Valgrind :
   ```bash
   valgrind --leak-check=full --track-origins=yes ./cc_langage_c
   ```

3. Analysez les résultats :
    - **Pas de problèmes :**
      ```plaintext
      All heap blocks were freed -- no leaks are possible
      ```
    - **Problèmes détectés :**
      Valgrind fournira des détails, incluant les lignes de code incriminées.

---

## **Structure du Projet**

| Fichier      | Description                                                          |
|--------------|----------------------------------------------------------------------|
| `main.c`     | Point d'entrée du programme.                                         |
| `repl.c`     | Implémente la boucle REPL pour interagir avec l'utilisateur.         |
| `btree.c`    | Gestion des arbres binaires pour stocker les données des tables.     |
| `db.c`       | Implémente les fonctions de gestion des tables et colonnes.          |
| `storage.c`  | Sauvegarde et chargement de la base de données.                      |
| `utils.c`    | Fonctions utilitaires comme `trim_whitespace`.                       |
| `Makefile`   | Fichier de configuration pour compiler le projet.                    |

---

## **Améliorations Possibles**

- **Support pour les types avancés** : Ajout d'autres types de colonnes (ex. booléens, dates).
- **Amélioration du format de persistance** : Support de JSON ou SQLite.
- **Ajout de commandes SQL avancées** : Support pour `DELETE`, `UPDATE`, ou des jointures.
