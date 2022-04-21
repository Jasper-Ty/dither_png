CC=gcc
OBJ=dither_png.o
LIB=-lpng

%.o: %.c
	$(CC) -c -o $@ $< $(LIB)

dither_png: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIB)

.PHONE : clean

clean : 
	-rm -f $(OBJ) dither_png > /dev/null 
