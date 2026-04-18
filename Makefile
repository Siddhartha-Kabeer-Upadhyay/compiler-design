CC = gcc
CFLAGS = -Wall -Wextra -std=c99
SRC = src/main.c src/hsv.c src/instruction.c src/tracer.c src/runtime.c src/codegen.c src/opt.c src/ir.c
OUT = glint

all: $(OUT)

test: $(OUT)
	@echo "Local script-based tests are not bundled in this tree."

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC) -lm

clean:
	rm -f $(OUT)
