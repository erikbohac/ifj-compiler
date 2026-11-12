CC = gcc
INCLUDE_DIR = src
CFLAGS = -Wall -Wextra -pedantic -std=c23 -I$(INCLUDE_DIR)

BUILD_DIR = .
TARGET = $(BUILD_DIR)/ifj_compiler
SOURCES = $(wildcard $(INCLUDE_DIR)/*.c)
HEADERS = $(wildcard $(INCLUDE_DIR)/*.h)
CODE_FILES = $(SOURCES) $(HEADERS)


.PHONY: build format check test run clean help


build: $(TARGET)

$(TARGET): $(CODE_FILES)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

format:
	clang-format -i $(CODE_FILES)

check:
	clang-format --dry-run --Werror $(CODE_FILES)
	@for file in $(SOURCES); do \
		clang-tidy --warnings-as-errors='*' "$$file" -- -std=c23 -I$(INCLUDE_DIR) || exit 1; \
	done

test: #build
	@echo "No tests implemented yet"

run: build
	./$(TARGET)

clean:
	rm -f $(TARGET)

help:
	@echo "IFJ26 Compiler - Available Make Targets"
	@echo ""
	@echo "Usage: make <target>"
	@echo ""
	@echo "Targets:"
	@echo "  build   Build the IFJ26-to-IFJcode26 compiler"
	@echo "  help    Display this help message"
	@echo "  format  Format all source and header files"
	@echo "  clean   Remove temporary and generated files"
	@echo "  check   Check code formatting and static analysis"
	@echo "  test    Run all automated tests"
	@echo "  run     Build and execute the compiler"
