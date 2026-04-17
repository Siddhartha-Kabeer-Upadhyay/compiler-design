CC = gcc
CFLAGS = -Wall -Wextra -std=c99
SRC = src/main.c src/hsv.c src/instruction.c src/tracer.c src/runtime.c src/codegen.c src/opt.c src/ir.c
OUT = glint
TEST_CORE = tests/test_core
ASM_TOOL = scripts/local_ci/glint_asm/glint_asm

all: $(OUT)

test: $(OUT) $(TEST_CORE) $(ASM_TOOL)
	./$(TEST_CORE)
	./scripts/local_ci/test_parity.sh

tool-asm: $(ASM_TOOL)

$(ASM_TOOL): scripts/local_ci/glint_asm/glint_asm.c
	$(CC) $(CFLAGS) -o $(ASM_TOOL) scripts/local_ci/glint_asm/glint_asm.c -lm

$(TEST_CORE): tests/test_core.c src/hsv.c src/instruction.c src/runtime.c src/tracer.c
	$(CC) $(CFLAGS) -Isrc -o $(TEST_CORE) tests/test_core.c src/hsv.c src/instruction.c src/runtime.c src/tracer.c -lm

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC) -lm

clean:
	rm -f $(OUT) $(TEST_CORE) $(ASM_TOOL)
