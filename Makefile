TARGET=dither_png

SRCDIR=src
INCDIR=inc
OBJDIR=obj
BINDIR=bin

CC=gcc
LIB=-lpng
CFLAGS=--std=c89 -I./$(INCDIR)

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
