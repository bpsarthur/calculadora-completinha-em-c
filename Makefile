# Makefile - Calculadora Cientifica (TDM-GCC / MinGW)
# Uso:  mingw32-make        (compila)
#       mingw32-make run    (compila e executa)
#       mingw32-make clean

RL      = libs/raylib-5.5_win64_mingw-w64
CC      = gcc
CFLAGS  = -O2 -std=c11 -Wall -I$(RL)/include
LDFLAGS = -L$(RL)/lib -lraylib -lopengl32 -lgdi32 -lwinmm
SRC     = main.c expr.c
OUT     = calculadora.exe

$(OUT): $(SRC) expr.h
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

run: $(OUT)
	./$(OUT)

clean:
	del /q $(OUT) 2>nul || true
