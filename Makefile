CC = gcc
CFLAGS = -Wall -Wextra -std=c99
SRC = src/main.c src/hsv.c
OUT = glint

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC) -lm

clean:
	rm -f $(OUT)