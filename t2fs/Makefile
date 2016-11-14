#
# Makefile ESQUELETO
#
# DEVE ter uma regra "all" para geração da biblioteca
# regra "clean" para remover todos os objetos gerados.
#
# NECESSARIO adaptar este esqueleto de makefile para suas necessidades.
#
#

CC=gcc
LIB=./lib
INC=./include
BIN=./bin
SRC=./src
TEST=./teste

FLAGS=-std=c99 -m32
INCLUDE=-I$(INC) -I$(SRC)
WARN=-Wall -Wextra -Werror
LINK=
OPT=-O3
DEBUG=-O0 -g
BUILD=$(FLAGS) $(INCLUDE) $(WARN) $(LINK)

all: libt2fs testes

libt2fs:
    $(CC) -c $(SRC)/records.c $(DEBUG) -o$(BIN)/records.o $(BUILD)
    $(CC) -c $(SRC)/paths.c $(DEBUG) -o$(BIN)/paths.o $(BUILD)
    $(CC) -c $(SRC)/fat.c $(DEBUG) -o$(BIN)/fat.o $(BUILD)
    $(CC) -c $(SRC)/t2fs.c $(DEBUG) -o$(BIN)/lib.o $(BUILD)
    ar crs $(LIB)/libt2fs.a $(LIB)/apidisk.o $(BIN)/fat.o $(BIN)/records.o $(BIN)/paths.o $(BIN)/lib.o

testes:
    $(CC) $(TEST)/testes.c -o $(BIN)/teste $(BUILD) -L$(LIB) -lt2fs

clean:
    rm -rf $(LIB)/*.a $(BIN)/*


