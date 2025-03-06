default: all

# Find all C files matching the pattern XX-*.c
SRCS := $(wildcard ??-*.c)

# Extract target names (XX part before '-')
PROJECTS := $(basename $(SRCS))

.PHONY: bin
bin:
	mkdir -p bin/


.PHONY: $(PROJECTS)
$(PROJECTS): %: bin
	@echo
	@echo Building $@...
	clang $@.c -o bin/$@ -Wall -Werror -Wextra

.PHONY: all
all: $(PROJECTS)

.PHONY: clean
clean:
	rm -rf ./bin/