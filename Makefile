CC = gcc
CFLAGS = -Wall -Wextra -std=c99
SRC = src/main.c src/hsv.c src/instruction.c
OUT = glint

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC) -lm

clean:
	rm -f $(OUT)