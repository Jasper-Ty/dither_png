TARGET=dither_png

CC=gcc
LIB=-lpng
CFLAGS=--std=c89

SRCDIR=src
OBJDIR=obj
BINDIR=bin

SRC=$(wildcard $(SRCDIR)/*.c)
OBJ=$(SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

$(BINDIR)/$(TARGET): $(BINDIR) $(OBJDIR) $(OBJ)
	$(CC) -o $@ $(OBJ) $(LIB) $(CFLAGS)

$(OBJ): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(CC) -c $< -o $@ $(LIB) $(CFLAGS)

$(BINDIR):
	mkdir $(BINDIR)

$(OBJDIR):
	mkdir $(OBJDIR)

.PHONY: clean

clean: 
	-rm -f $(OBJDIR)/*.o $(BINDIR)/* 1>/dev/null 2>&1
