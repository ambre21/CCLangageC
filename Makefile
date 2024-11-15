# Info sur la compilation
TARGET = cc_langage_c

CC = gcc

CFLAGS = -Wall -Wextra -std=c11 -g

OBJS = main.o repl.o btree.o db.o storage.o utils.o

# Compilation de l'exécutable final
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compilation des fichiers source en objets
main.o: main.c repl.h db.h
	$(CC) $(CFLAGS) -c main.c

repl.o: repl.c repl.h db.h btree.h utils.h storage.h
	$(CC) $(CFLAGS) -c repl.c

btree.o: btree.c btree.h utils.h
	$(CC) $(CFLAGS) -c btree.c

db.o: db.c db.h btree.h
	$(CC) $(CFLAGS) -c db.c

storage.o: storage.c storage.h db.h btree.h utils.h
	$(CC) $(CFLAGS) -c storage.c

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c

# Nettoyage des fichiers objets et de l'exécutable
clean:
	rm -f $(OBJS) $(TARGET)
