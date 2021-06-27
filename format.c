/* Vlasov Roman. June */

#include <string.h>

#include "include/format.h"
#include "include/macros.h"

int COEF_WITHOUT_CORRECTION[3] = {1, 1, 1};
int COEF_WITH_CORRECTION[3] = {0.0041, 0.995, 0.023};

// get pixel in rgb color type from row
png_bytep
get_pixel_rgba(png_bytep row, int x)
{
	return &row[4 * x];
}

// get pixel in rgb color type from row
png_bytep
get_pixel_rgb(png_bytep row, int x)
{
	return &row[3 * x];
}

// change p1 to p2 (pixel)
void
change_pixel_rgba(png_bytep p1, png_bytep p2)
{
	memcpy(p1, p2, sizeof(*p1) * 4);
}
	
// change p1 to p2 (pixel)
void
change_pixel_rgb(png_bytep p1, png_bytep p2)
{
	memcpy(p1, p2, sizeof(*p1) * 3);
}

int 
metric_rgba(png_bytep* rows, int x, int y, int height, int width, int correction)
{
	int m = 0;
	int r, g, b;
	png_bytep p1, p2;
	int* corr = COEF_WITHOUT_CORRECTION;
	if (correction == WITH_CORRECTION)
		corr = COEF_WITH_CORRECTION;

	for	(int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			if 	((x + i) >= 0 && (x + i) <= height - 1 && 
				((y + j) >= 0 && (y + j) <= width - 1))
			{
				p1 = &(rows[x + i][4 * (y + j)]);
				p2 = &(rows[x][4 * y]);
				
				// red
				r = corr[0] * (p1[0] * p1[3] - p2[0] * p2[3]);
				// green	
				g = corr[1] * (p1[1] * p1[3] - p2[1] * p2[3]);
				// blue
				b = corr[2] * (p1[2] * p1[3] - p2[2] * p2[3]);

				m += r * r + g * g + b * b;
			}
		}
	}
	return m;
}

int 
metric_rgb(png_bytep* rows, int x, int y, int height, int width, int correction)
{
	int m = 0;
	int r, g, b;
	png_bytep p1, p2;
	int* corr = COEF_WITHOUT_CORRECTION;
	if (correction == WITH_CORRECTION)
		corr = COEF_WITH_CORRECTION;

	for	(int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			if 	((x + i) >= 0 && (x + i) <= height - 1 && 
				((y + j) >= 0 && (y + j) <= width - 1))
			{
				p1 = &(rows[x + i][3 * (y + j)]);
				p2 = &(rows[x][3 * y]);
				
				// red
				r = corr[0] * (p1[0] - p2[0]);
				// green	
				g = corr[1] * (p1[1] - p2[1]);
				// blue
				b = corr[2] * (p1[2] - p2[2]);

				m += r * r + g * g + b * b;
			}
		}
	}
	return m;
}
