CC=gcc
OBJ=dither_png.o
LIB=-lpng
CFLAGS=--std=c89

%.o: %.c
	$(CC) -c -o $@ $< $(LIB) $(CFLAGS)

dither_png: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIB) $(CFLAGS)

.PHONE : clean

clean : 
	-rm -f $(OBJ) dither_png > /dev/null 
