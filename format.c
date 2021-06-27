/* Vlasov Roman. June */

#include "include/format.h"

png_bytep
get_pixel_rgba(png_bytep row, int x)
{
	return &row[4 * x];
}

png_bytep
get_pixel_rgb(png_bytep row, int x)
{
	return &row[3 * x];
}

int 
metric_rgba(png_bytep* rows, int x, int y, int height, int width)
{
	int m = 0, tmp = 0;
	png_bytep p1, p2;
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
				tmp = (p1[0] * p1[3] - p2[0] * p2[3]);
				m += tmp * tmp;
				
				// green	
				tmp = (p1[1] * p1[3] - p2[1] * p2[3]);
				m += tmp * tmp;

				// blue
				tmp = (p1[2] * p1[3] - p2[2] * p2[3]);
				m += tmp * tmp;
			}
		}
	}
	return m;
}

int 
metric_rgb(png_bytep* rows, int x, int y, int height, int width)
{
	int m = 0, tmp = 0;
	png_bytep p1, p2;
	for	(int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			if 	((x + i) >= 0 && (x + i) <= height - 1 && 
				((y + j) >= 0 && (y + j) <= width - 1))
			{
				p1 = &(rows[x + i][3 * (y + j)]);
				p2 = &(rows[x][3* y]);
				
				// red
				tmp = (p1[0] - p2[0]);
				m += tmp * tmp;
				
				// green	
				tmp = (p1[1] - p2[1]);
				m += tmp * tmp;

				// blue
				tmp = (p1[2] - p2[2]);
				m += tmp * tmp;
			}
		}
	}
	return m;
}
