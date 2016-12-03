#
# Makefile ESQUELETO
#
# DEVE ter uma regra "all" para geração da biblioteca
# regra "clean" para remover todos os objetos gerados.
#
# NECESSARIO adaptar este esqueleto de makefile para suas necessidades.
#
#

# CC=gcc
# LIB_DIR=./lib
# INC_DIR=./include
# BIN_DIR=./bin
# SRC_DIR=./src
# TEST_DIR=./teste
# BUILD=./build
# #WARN=-Wall -Wextra -Werror

# all: t2fs main

# main:
# 	$(CC) -c main.c $(DEBUG) -o$(BIN_DIR)/main.o $(BUILD) -Wall

# t2fs:
# 	$(CC) -c $(SRC_DIR)/paths.c $(DEBUG) -o$(BIN_DIR)/paths.o $(BUILD) $(WARN)
# 	$(CC) -c $(SRC_DIR)/t2fs.c $(DEBUG) -o$(BIN_DIR)/lib.o $(BUILD) $(WARN)
# 	ar crs t2fs.a $(BIN_DIR)/*.o

# testes:
# 	$(CC) $(TEST_DIR)/testes.c -o $(BIN_DIR)/teste $(BUILD) -L$(LIB_DIR) -lt2fs

# clean:
# 	rm -rf $(LIB_DIR)/*.a $(BIN_DIR)/*.o $(SRC_DIR)/*~ $(INC_DIR)/*~ *~


CC=gcc -g -w
LIB_DIR=./lib
INC_DIR=./include
BIN_DIR=./bin
SRC_DIR=./src
TST_DIR=./teste

all: objetos mvObj libt2fs.a mvLib main

objetos: $(SRC_DIR)/t2fs.c $(SRC_DIR)/paths.c $(INC_DIR)/apidisk.h $(INC_DIR)/bitmap2.h $(INC_DIR)/paths.h $(INC_DIR)/t2fs.h
	$(CC) -c $(SRC_DIR)/t2fs.c -Wall
	
mvObj:
	mv *.o $(BIN_DIR)

libt2fs.a: 
	ar crs libt2fs.a $(BIN_DIR)/*.o $(LIB_DIR)/*.o

mvLib:
	mv *.a $(LIB_DIR)

main:
	$(CC) -o mainprog main.c -L$(LIB_DIR) -lt2fs -Wall	

clean:
	rm -rf mainprog $(LIB_DIR)/*.a $(BIN_DIR)/t2fs.o $(TST_DIR)/*.o $(SRC_DIR)/*~ $(INC_DIR)/*~ *~