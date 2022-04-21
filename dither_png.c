#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <setjmp.h>
#include <string.h>

#include <png.h>

#define DEFAULT_OUTPUT_FILENAME "out.png"

/* SETUP/TEARDOWN FUNCTIONS --------------------------------------------------------  
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
    int SIG_BYTES = 8;
    char header[SIG_BYTES];

    fread(&header, 1, SIG_BYTES, fp);
    if (png_sig_cmp(header, 0, SIG_BYTES)) 
    {
        fprintf(stderr, "Not a PNG\n");
        exit(EXIT_FAILURE);
    }

    *png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!(*png_ptr)) 
    {
        fprintf(stderr, "png_create_read_struct() error\n");
        exit(EXIT_FAILURE);
    }

    *info_ptr = png_create_info_struct(*png_ptr);
    if (!(*info_ptr))
    {
        fprintf(stderr, "png_create_info_struct() error\n");
        png_destroy_read_struct (png_ptr, NULL, NULL);
        exit(EXIT_FAILURE);
    }

    *end_info = png_create_info_struct(*png_ptr);
    if (!(*end_info))
    {
        fprintf(stderr, "png_create_info_struct() error\n");
        png_destroy_read_struct (png_ptr, info_ptr, NULL);
        exit(EXIT_FAILURE);
    }

    if (setjmp(png_jmpbuf(*png_ptr)))
    {
        png_destroy_read_struct (png_ptr, info_ptr, end_info);
        fclose (fp);
        exit(EXIT_FAILURE);
    }
    
    png_init_io (*png_ptr, fp);
    png_set_sig_bytes (*png_ptr, SIG_BYTES);

    png_read_info (*png_ptr, *info_ptr);
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

    if (setjmp(png_jmpbuf(*png_ptr))) {
        png_destroy_write_struct (png_ptr, info_ptr);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

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


/* PNG CONFIG FUNCTIONS --------------------------------------------------------  
 *
 * set_to_grayscale:
 *     Sets PNG to 8-bit grayscale
 * copy_IHDR:
 *     Copies IHDR data between two PNG files
 *******************************************************************************/
void set_to_grayscale (png_structp png_ptr,
                       png_infop info_ptr)
{
    png_uint_32 width, height;
    int bit_depth, color_type;
    png_get_IHDR (png_ptr, info_ptr, 
                  &width, &height, 
                  &bit_depth, 
                  &color_type, 
                  NULL, NULL, NULL); 

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

/* IMAGE DATA FUNCTIONS --------------------------------------------------------  
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
    /* Get dimensions */
    png_uint_32 img_height = png_get_image_height (png_ptr, info_ptr);
    png_uint_32 rowbytes = png_get_rowbytes(png_ptr, info_ptr);

    /* Allocate row pointers */
    png_bytep *row_ptrs = malloc(img_height * sizeof(*row_ptrs));
    if (!row_ptrs)
        return fprintf(stderr, "malloc failed()!"), NULL;

    /* Allocate each row */
    size_t row;
    for (row = 0; row < img_height; row++) {
        row_ptrs[row] = malloc(rowbytes * sizeof(**row_ptrs));
        if (!(row_ptrs[row])) 
            return fprintf(stderr, "malloc failed()!"), NULL;
    }

    /* Read the image data */
    png_read_image (png_ptr, row_ptrs);

    /* DONE */
    return row_ptrs;
}

void commit_rows_to_png (png_bytep *row_ptrs,
                         png_structp png_ptr,
                         png_infop info_ptr)
{
    png_set_rows (png_ptr, info_ptr, row_ptrs);
    png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
}

/* Floyd-Steinberg dithering implementation */ 
void dither_1bit (png_bytep *row_ptrs, 
                  png_structp png_ptr,
                  png_infop info_ptr)
{
    /* Abort if not 2-channel (Grayscale/Alpha) */
    size_t dx = png_get_channels (png_ptr, info_ptr);
    if (dx != 2) return;

    /* Get dimensions */
    size_t width = dx * ( (size_t) png_get_image_width (png_ptr, info_ptr) );
    size_t height = (size_t) png_get_image_height (png_ptr, info_ptr);

    size_t x, y;
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x += dx)
        {
            png_byte old = row_ptrs[y][x];
            row_ptrs[y][x] = 255 * (old < 128);
            png_byte err = old - row_ptrs[y][x];
            if (x<width-dx              ) row_ptrs[y  ][x+dx] += (png_byte) ((7*err)/16.0);
            if (x>0        && y<height-1) row_ptrs[y+1][x-dx] += (png_byte) ((3*err)/16.0);
            if (              y<height-1) row_ptrs[y+1][x   ] += (png_byte) ((5*err)/16.0);
            if (x<width-dx && y<height-1) row_ptrs[y+1][x+dx] += (png_byte) ((1*err)/16.0);
        }
    }
}

/* main
 * 
 * LET'S GOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO
 *******************************************************************************/
int main (int argc, 
          char **argv)
{

/* Print usage -----------------------------------------------------------*/
    if (argc <= 1)
    {
        printf("Usage: %s <file> [output_filename]\n", argv[0]);
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
    dither_1bit (row_ptrs, in_pngp, in_infop); 
    /* -------------------------------------------------------------------*/


/* Finalize: save PNG and free structs -----------------------------------*/
    commit_rows_to_png (row_ptrs, out_pngp, out_infop);

    free (row_ptrs);
    teardown_read (img_read, &in_pngp, &in_infop, &in_endp);
    teardown_write (img_write, &out_pngp, &out_infop);
    /* -------------------------------------------------------------------*/


    return EXIT_SUCCESS;
}
