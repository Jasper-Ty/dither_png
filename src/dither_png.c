/*
 * dither_png.c
 *
 * Program that takes .PNG files and creates a dithered .PNG file output
 */
#define DEFAULT_OUTPUT_FILENAME "out.png"

#include <stdio.h>  /* printf(), fprintf(), perror(), 
                       FILE*, fopen(), fclose(), fread() */
#include <stdlib.h> /* malloc(), free(),
                       exit(), EXIT_SUCCESS, EXIT_FAILURE */
#include <limits.h> /* SIZE_MAX */
#include <spng.h>   /* spng_* */

#include "parse.h"  /* Options, parse_options() */

#define IMAGE_LIMIT_WIDTH 42069
#define IMAGE_LIMIT_HEIGHT 42069

#define CHUNK_SIZE_LIMIT 1024

#define BUFSIZE

/* SETUP/TEARDOWN FUNCTIONS
 *
 * {setup/teardown}_{read/write}
 *     Initializes/destroys LibPNG structs and data
 *     for the input(read)/output(write) PNG file
 *******************************************************************************/

void setup_read (FILE *fp,
                 spng_ctx *ctx)
{
/* Setup limits (to safely open untrusted files) ------------------------ */
    spng_set_image_limits (ctx, IMAGE_LIMIT_WIDTH, IMAGE_LIMIT_HEIGHT);
    spng_set_chunk_limits (ctx, CHUNK_SIZE_LIMIT, SIZE_MAX);
    spng_set_crc_action (ctx, SPNG_CRC_USE, SPNG_CRC_USE);
    /* ------------------------------------------------------------------ */

    if (!(ctx = spng_ctx_new(0))) 
        goto setup_read_err;



setup_read_err:
    spng_ctx_free (ctx);
    exit (EXIT_FAILURE);
}

void teardown_read (FILE *fp)
{

}

void setup_write (FILE *fp)
{
}

void teardown_write (FILE *fp)
{
}


/* DITHERING FUNCTIONS
 *
 * dither_floyd_steinberg 
 *     Dithers using an error diffusion algorithm 
 * generate_threshold_matrix
 *     Creates a threshold matrix of order N, used for dithered ordering
 * dither_ordered
 *     Dithers using a threshold matrix
 *******************************************************************************/

/* Floyd-Steinberg dithering implementation */
void dither_floyd_steinberg (png_bytep *idat, 
                             png_structp png_ptr,
                             png_infop info_ptr)
{
    size_t x, y;

/* Abort if not grayscale image data! ----------------------------------- */
    size_t dx = png_get_channels (png_ptr, info_ptr);
    if (dx > 2) return; /* 2-channel: Grayscale/Alpha, 1-channel: Grayscale */
    /* ------------------------------------------------------------------ */

/* Get dimensions ------------------------------------------------------- */
    size_t width = (size_t) png_get_image_width (png_ptr, info_ptr);
    size_t height = (size_t) png_get_image_height (png_ptr, info_ptr);
    /* ------------------------------------------------------------------ */

/* Create temp array for performing calculations (avoid overflow errs) -- */
    int *T = malloc (width*height*sizeof(int));
    if (!T) {
        perror ("malloc");
        return;
    }
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            T[y*width+x] = (int) idat[y][x*dx];
    /* ------------------------------------------------------------------ */

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
            idat[y][x*dx] = (png_byte) T[y*width+x];
    free (T);
    /* ------------------------------------------------------------------ */
}

int *generate_threshold_matrix (size_t N)
{
    size_t dim = 1 << N;
    int *matrix = malloc (dim*dim*sizeof(*matrix));

    size_t x, y, bit;
    for (y = 0; y < dim; y++) {
        for (x = 0; x < dim; x++) {
            unsigned char v = 0, mask = N-1, xc = x^y, yc = y;
            for (bit = 0; bit < 2*N; mask--) {
                v |= ((yc >> mask)&1) << bit++;
                v |= ((xc >> mask)&1) << bit++;
            }
            matrix [y*dim+ x] = v << ((4-N) << 1);
        }
    }
    return matrix;
}

/* Ordered dithering implementation */ 
void dither_ordered (png_bytep *idat, 
                     png_structp png_ptr,
                     png_infop info_ptr,
                     size_t order)
{
    int *threshold = generate_threshold_matrix (order);
    size_t dim = 1 << order;
    size_t x, y;

/* Abort if not grayscale image data! ----------------------------------- */
    size_t dx = png_get_channels (png_ptr, info_ptr);
    if (dx > 2) return; /* 2-channel: Grayscale/Alpha, 1-channel: Grayscale */
    /* ------------------------------------------------------------------ */

/* Get dimensions ------------------------------------------------------- */
    size_t width = dx * ( (size_t) png_get_image_width (png_ptr, info_ptr) );
    size_t height = (size_t) png_get_image_height (png_ptr, info_ptr);
    /* ------------------------------------------------------------------ */

/* Perform dithering algorithm ------------------------------------------ */
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x += dx)
            idat[y][x] = 255*(idat[y][x] > threshold[(y%dim)*dim+(x%(dim*dx))]);
    /* ------------------------------------------------------------------ */
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
    char *out_filename = ( (argc >= 3) ? argv[2] : DEFAULT_OUTPUT_FILENAME );
    FILE *img_write;
    if (!(img_write = fopen(out_filename, "wb"))) {
       perror("fopen");
       exit(EXIT_FAILURE);
    } 
    /* -------------------------------------------------------------------*/


/* Initialize LibSPNG ----------------------------------------------------*/
    spng_ctx *ctx = spng_ctx_new(0);
    char *buf = malloc (
    /* -------------------------------------------------------------------*/


/* Load image data and perform dither ------------------------------------*/
    /* -------------------------------------------------------------------*/


/* Finalize: save PNG and free structs -----------------------------------*/
    /* -------------------------------------------------------------------*/


    return EXIT_SUCCESS;
}
