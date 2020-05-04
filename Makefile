SHELL = /bin/bash

.SUFFIXES:
.SUFFIXES: .c .h .o

CC	= gcc
INCLDIR	= include/
BINDIR	= bin/
OBJDIR	= obj/
SRCDIR	= src/

# Ficheiro final
_BIN	= plate-recognizer
BIN	= $(addprefix $(BINDIR), $(_BIN))

SRC	= $(wildcard src/*.c)
_OBJS	= $(patsubst src/%.c, %.o, $(SRC))
OBJS	= $(addprefix $(OBJDIR), $(_OBJS))


# compilation flags
CFLAGS = -lm#-Wall -std=c99 -pedantic -g -I$(INCLDIR)
OFLAGS = -lm

# compile binary and object files
.PHONY: all
all: $(BIN) #docs

$(BINDIR):
	mkdir -p $(BINDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(INCLDIR):
	mkdir -p $(INCLDIR)

# Regras

$(BIN): $(BINDIR) $(BISONGEN) $(OBJS) $(BISONOBJS) $(FLEXOBJS)
	$(CC) -o $(BIN) $(OBJS) $(BISONOBJS) $(FLEXOBJS) $(OFLAGS)

$(OBJS): $(OBJDIR) $(SRC)
	$(CC) -c $(patsubst %.o, %.c, $(patsubst obj/%, src/%, $@)) -o $@ $(CFLAGS)

$(FLEXOBJS): $(FLEXGEN)
	$(CC) -c $(patsubst %.o, %.c, $(patsubst obj/%, include/%, $@)) -o $@ $(CFLAGS)

$(BISONOBJS): $(BISONGEN)
	$(CC) -c $(patsubst %.o, %.c, $(patsubst obj/%, include/%, $@)) -o $@ $(CFLAGS)



DOCDIR = docs/
TEXDIR = latex/

.PHONY: docs docs-clean
docs: Doxyfile
	doxygen
#	generate PDF from LaTeX sources
	cd $(DOCDIR)$(TEXDIR) && $(MAKE)
#	mv $(DOCDIR)$(TEXDIR)refman.pdf $(DOCDIR)

docs-clean:
	cd $(DOCDIR)$(TEXDIR) && $(MAKE) clean


# clean entire project directory
.PHONY: clean
clean:
	- rm -rf $(BINDIR) $(OBJDIR) $(DOCDIR)  $(INCLDIR)


# check code quality
.PHONY: cppcheck memcheck
cppcheck:
	cppcheck --enable=all --language=c --std=c99 --inconclusive \
	--suppress=missingInclude $(SRC) -i $(INCLDIR)

memcheck: all
	valgrind -v --show-leak-kinds=all --leak-check=full --track-origins=yes \
	./$(BIN)

# debugging make
print-% :
	@echo $* = $($*)
