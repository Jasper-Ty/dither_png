/*
 * dither_png.c
 *
 * Program that takes .PNG files and creates a dithered .PNG file output
 */

#include <stdio.h>  /* printf(), fprintf(), perror(), 
                       FILE*, fopen(), fclose(), fread() */
#include <stdlib.h> /* malloc(), free(),
                       exit(), EXIT_SUCCESS, EXIT_FAILURE */

#include <png.h>   
#include "parse.h" 

#define DEFAULT_OUTPUT_FILENAME "out.png"

/* SETUP/TEARDOWN FUNCTIONS
 *
 * {setup/teardown}_{read/write}:
 *     Initializes/destroys LibPNG structs and data
 *     for the input(read)/output(write) PNG file
 *******************************************************************************/

void setup_read (FILE *fp,
                 png_structp *png_ptr, 
                 png_infop *info_ptr,
                 png_infop *end_info)
{

/* Check first 8 bytes to see if file is PNG ---------------------------- */
    int SIG_BYTES = 8;
    char header[SIG_BYTES];

    fread(&header, 1, SIG_BYTES, fp);
    if (png_sig_cmp(header, 0, SIG_BYTES)) 
    {
        fprintf(stderr, "Not a PNG\n");
        exit(EXIT_FAILURE);
    }
    /* ------------------------------------------------------------------ */

/* Create read, info, and end structs ----------------------------------- */ 
    *png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!(*png_ptr)) {
        fprintf(stderr, "png_create_read_struct() error\n");
        exit(EXIT_FAILURE);
    }

    *info_ptr = png_create_info_struct(*png_ptr);
    if (!(*info_ptr)) {
        fprintf(stderr, "png_create_info_struct() error\n");
        png_destroy_read_struct (png_ptr, NULL, NULL);
        exit(EXIT_FAILURE);
    }

    *end_info = png_create_info_struct(*png_ptr);
    if (!(*end_info)) {
        fprintf(stderr, "png_create_info_struct() error\n");
        png_destroy_read_struct (png_ptr, info_ptr, NULL);
        exit(EXIT_FAILURE);
    }
    /* ------------------------------------------------------------------ */

/* setjmp for error handling -------------------------------------------- */
    if (setjmp(png_jmpbuf(*png_ptr))) {
        png_destroy_read_struct (png_ptr, info_ptr, end_info);
        fclose (fp);
        exit(EXIT_FAILURE);
    }
    /* ------------------------------------------------------------------ */
    
/* Open PNG file and read PNG info -------------------------------------- */
    png_init_io (*png_ptr, fp);
    png_set_sig_bytes (*png_ptr, SIG_BYTES);

    png_read_info (*png_ptr, *info_ptr);
    /* ------------------------------------------------------------------ */
}

void teardown_read (FILE *fp,
                    png_structp *png_ptr,
                    png_infop *info_ptr,
                    png_infop *end_info)
{
    png_read_end (*png_ptr, *end_info);
    png_destroy_read_struct (png_ptr, info_ptr, end_info);
    if (fclose(fp) == EOF) {
        perror("fclose");
        exit(EXIT_FAILURE);
    }
}

void setup_write (FILE *fp,
                  png_structp *png_ptr,
                  png_infop *info_ptr)
{
/* Create read and info structs ----------------------------------------- */
    *png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fprintf(stderr, "png_create_write_struct() error\n");
        exit(EXIT_FAILURE);
    }
    *info_ptr = png_create_info_struct (*png_ptr);
    if (!(*info_ptr)) {
        fprintf(stderr, "png_create_info_struct() error\n");
        png_destroy_write_struct (png_ptr, NULL);
        exit(EXIT_FAILURE);
    }
    /* ------------------------------------------------------------------ */

/* setjmp for error handling -------------------------------------------- */
    if (setjmp(png_jmpbuf(*png_ptr))) {
        png_destroy_write_struct (png_ptr, info_ptr);
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    /* ------------------------------------------------------------------ */

    png_init_io (*png_ptr, fp);
}

void teardown_write (FILE *fp,
                     png_structp *png_ptr,
                     png_infop *info_ptr)
{
    png_destroy_write_struct (png_ptr, info_ptr);
    if (fclose(fp) == EOF) {
        perror("fclose");
        exit(EXIT_FAILURE);
    }
}


/* PNG CONFIG FUNCTIONS 
 *
 * set_to_grayscale:
 *     Sets PNG to 8-bit grayscale
 * copy_IHDR:
 *     Copies IHDR data between two PNG files
 *******************************************************************************/

void set_to_grayscale (png_structp png_ptr,
                       png_infop info_ptr)
{
    png_byte color_type = png_get_color_type (png_ptr, info_ptr);
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGB_ALPHA)
        png_set_rgb_to_gray_fixed (png_ptr, 1, -1, -1);
    png_read_update_info (png_ptr, info_ptr);
}

void copy_IHDR (png_structp in_pngp, png_infop in_infop,
                png_structp out_pngp, png_infop out_infop)
{
    png_uint_32 width, height;
    int bit_depth, color_type;
    int interlace_type, compression_type, filter_method;

    png_get_IHDR (in_pngp, in_infop, 
                  &width, &height, 
                  &bit_depth, 
                  &color_type, 
                  &interlace_type, 
                  &compression_type, 
                  &filter_method); 
    png_set_IHDR (out_pngp, out_infop,
                  width, height,
                  bit_depth,
                  color_type,
                  interlace_type,
                  compression_type,
                  filter_method);
}

/* IMAGE DATA FUNCTIONS
 *
 * load_rows:
 *     Allocates and returns 2D byte array populated with PNG image data
 * commit_rows_to_png:
 *     Sets PNG image data and commits it to PNG file
 * dither_1bit:
 *     Converts 8-bit grayscale image data to 1-bit black and white image data
 *******************************************************************************/

png_bytep *load_rows (png_structp png_ptr,
                      png_infop info_ptr)
{
/* Get dimensions for allocating  --------------------------------------- */
    png_uint_32 img_height = png_get_image_height (png_ptr, info_ptr);
    png_uint_32 rowbytes = png_get_rowbytes(png_ptr, info_ptr);

/* Allocate row pointers and rows --------------------------------------- */
    png_bytep *row_ptrs = malloc(img_height * sizeof(*row_ptrs));
    if (!row_ptrs)
        return fprintf(stderr, "malloc failed()!"), NULL;
    size_t row;
    for (row = 0; row < img_height; row++) {
        row_ptrs[row] = malloc(rowbytes * sizeof(**row_ptrs));
        if (!(row_ptrs[row])) 
            return fprintf(stderr, "malloc failed()!"), NULL;
    }

/* Read in png, then done! ---------------------------------------------- */
    png_read_image (png_ptr, row_ptrs);
    return row_ptrs;
}


void commit_rows_to_png (png_bytep *row_ptrs,
                         png_structp png_ptr,
                         png_infop info_ptr)
{
    png_set_rows (png_ptr, info_ptr, row_ptrs);
    png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
}

void free_rows (png_bytep *row_ptrs,
                png_structp png_ptr,
                png_infop info_ptr)
{
    size_t height = (size_t) png_get_image_height (png_ptr, info_ptr);
    size_t y;
    for (y = 0; y < height; y++)
        free(row_ptrs[y]);

    free(row_ptrs);
}

/* Floyd-Steinberg dithering implementation */ 
void dither_1bit_floyd_steinberg (png_bytep *row_ptrs, 
                                  png_structp png_ptr,
                                  png_infop info_ptr)
{
    size_t x, y;

/* Abort if not grayscale image data! ----------------------------------- */
    size_t dx = png_get_channels (png_ptr, info_ptr);
    if (dx > 2) return; /* 2-channel: Grayscale/Alpha, 1-channel: Grayscale */

/* Get dimensions ------------------------------------------------------- */
    size_t width = (size_t) png_get_image_width (png_ptr, info_ptr);
    size_t height = (size_t) png_get_image_height (png_ptr, info_ptr);

/* Create temp array for performing calculations (avoid overflow errs) -- */
    int *T = malloc (width*height*sizeof(int));
    if (!T) {
        perror ("malloc");
        return;
    }
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            T[y*width+x] = (int) row_ptrs[y][x*dx];

/* Perform dithering algorithm ------------------------------------------ */
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            int val = T[y*width + x] > 128;
            int err = T[y*width + x] - 255*val;
            T[y*width + x] = 255*val;

            if (x<width-1              ) T[(y  )*width+(x+1)] += err * (7.0/16.0); 
            if (x>0       && y<height-1) T[(y+1)*width+(x-1)] += err * (3.0/16.0); 
            if (             y<height-1) T[(y+1)*width+(x  )] += err * (5.0/16.0); 
            if (x<width-1 && y<height-1) T[(y+1)*width+(x+1)] += err * (1.0/16.0); 
        }
    }
    /* ------------------------------------------------------------------ */

/* Commit temp array data to row_ptrs ----------------------------------- */
    for (y = 0; y < height; y++) 
        for (x = 0; x < width; x++) 
            row_ptrs[y][x*dx] = (png_byte) T[y*width+x];
    free (T);
}

/* Threshold matrix */
#define ORDER_COEFF 4
unsigned char order_matrix[] = 
{
     0, 32,  8, 40,  2, 34, 10, 42,
    48, 16, 56, 24, 50, 18, 58, 26,
    12, 44,  4, 36, 14, 46,  6, 38,
    60, 28, 52, 20, 62, 30, 54, 22,
     3, 35, 11, 43,  1, 33,  9, 41,
    51, 19, 59, 27, 49, 17, 57, 25,
    15, 47,  7, 39, 13, 45,  4, 37,
    63, 31, 55, 23, 61, 29, 53, 21
};

/* Ordered dithering implementation */ 
void dither_1bit_ordered (png_bytep *row_ptrs, 
                          png_structp png_ptr,
                          png_infop info_ptr)
{
    size_t x, y;

/* Abort if not grayscale image data! ----------------------------------- */
    size_t dx = png_get_channels (png_ptr, info_ptr);
    if (dx > 2) return; /* 2-channel: Grayscale/Alpha, 1-channel: Grayscale */

/* Get dimensions ------------------------------------------------------- */
    size_t width = dx * ( (size_t) png_get_image_width (png_ptr, info_ptr) );
    size_t height = (size_t) png_get_image_height (png_ptr, info_ptr);

/* Perform dithering algorithm ------------------------------------------ */
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x += dx)
            row_ptrs[y][x] = 255*(row_ptrs[y][x] > ORDER_COEFF*order_matrix[(y%8)*8+(x%(8*dx))]);
}


/* main
 * 
 * Loads PNG image and converts to grayscale, then uses
 * the grayscale image data to perform 1-bit dithering. 
 * The image data is then committed to an output PNG image
 *******************************************************************************/
int main (int argc, 
          char **argv)
{
    Options opts;
    parse_options (&argc, &argv, opts);

/* Print usage -----------------------------------------------------------*/
    if (argc <= 1)
    {
        printf("Usage: %s <file> [output_filename] [-f]\n", argv[0]);
        printf("    -f: Use Floyd-Steinberg dithering algorithm\n");
        exit(EXIT_SUCCESS);
    }
    /* -------------------------------------------------------------------*/


/* Open files ------------------------------------------------------------*/
    char *in_filename = argv[1];
    FILE *img_read; 
    if (!(img_read = fopen(in_filename, "rb"))) {
       perror("fopen");
       exit(EXIT_FAILURE);
    } 
    char *out_filename = ( (argc == 3) ? argv[2] : DEFAULT_OUTPUT_FILENAME );
    FILE *img_write;
    if (!(img_write = fopen(out_filename, "wb"))) {
       perror("fopen");
       exit(EXIT_FAILURE);
    } 
    /* -------------------------------------------------------------------*/


/* Initialize LibPNG structs ---------------------------------------------*/
    png_structp in_pngp = NULL; 
    png_infop in_infop = NULL;
    png_infop in_endp = NULL;
    setup_read (img_read, &in_pngp, &in_infop, &in_endp);

    png_structp out_pngp = NULL;
    png_infop out_infop = NULL;
    setup_write (img_write, &out_pngp, &out_infop);

    set_to_grayscale (in_pngp, in_infop);
    copy_IHDR (in_pngp, in_infop, out_pngp, out_infop);
    /* -------------------------------------------------------------------*/


/* Load image data and perform dither ------------------------------------*/
    png_bytep *row_ptrs = load_rows (in_pngp, in_infop);
    if (!row_ptrs) {
        fprintf (stderr, "Unable to read image data\n");
        teardown_read (img_read, &in_pngp, &in_infop, &in_endp);
        teardown_write (img_write, &out_pngp, &out_infop);
        exit(EXIT_FAILURE);
    }
    if (opts['f']) {
        dither_1bit_floyd_steinberg (row_ptrs, in_pngp, in_infop);
    } else {
        dither_1bit_ordered (row_ptrs, in_pngp, in_infop);
    }
    /* -------------------------------------------------------------------*/


/* Finalize: save PNG and free structs -----------------------------------*/
    commit_rows_to_png (row_ptrs, out_pngp, out_infop);

    free_rows(row_ptrs, out_pngp, out_infop);
    teardown_read (img_read, &in_pngp, &in_infop, &in_endp);
    teardown_write (img_write, &out_pngp, &out_infop);
    /* -------------------------------------------------------------------*/


    return EXIT_SUCCESS;
}
