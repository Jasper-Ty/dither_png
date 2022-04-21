CC=gcc
LIB=-lpng
CFLAGS=--std=c89

SRCDIR=src
OBJ=dither_png.o
OBJDIR=obj
BIN=dither_png
BINDIR=bin

vpath %.c src
vpath %.h src
vpath %.o obj

$(OBJDIR)/%.o: %.c
	$(CC) -c -o $@ $< $(LIB) $(CFLAGS)

$(BINDIR)/$(BIN): $(OBJDIR)/$(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIB) $(CFLAGS)

.PHONY: clean

clean: 
	-rm -f $(OBJDIR)/*.o $(BINDIR)/* > /dev/null 
