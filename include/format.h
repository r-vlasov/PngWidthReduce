#ifndef __PNG_COLORTYPE_FORMAT_H
#define __PNG_COLORTYPE_FORMAT_H


#include <png.h>

png_bytep
get_pixel_rgba(png_bytep, int);

png_bytep
get_pixel_rgb(png_bytep, int);

void
change_pixel_rgb(png_bytep, png_bytep);

void
change_pixel_rgba(png_bytep, png_bytep);

int
metric_rgba(png_bytep*, int, int, int, int, int);

int
metric_rgb(png_bytep*, int, int, int, int, int);


#endif
