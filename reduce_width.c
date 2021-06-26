#include <stdio.h>
#include <stdlib.h>
#include <png.h>

#define CHECK_PNG_HEADER_SIZE 8

int
check_png(FILE* fp)
{
	char check_png_header[CHECK_PNG_HEADER_SIZE];
	fread(check_png_header, sizeof(char), CHECK_PNG_HEADER_SIZE, fp);
	int is_png = !png_sig_cmp(check_png_header, 0, CHECK_PNG_HEADER_SIZE);
	if (!is_png)
		return -1;
	return 0;
}


typedef struct
{
	int height;
	int width;
	png_bytep* rows;
} READ_PNG_STRUCT;
	
READ_PNG_STRUCT
read_png(char *name)
{
	FILE* fp = fopen(name, "rb");
	if (!fp)
	{
		printf("error open");
		exit(-1);
	}
	if (check_png(fp))
	{
		printf("error check png");
		exit(-1);
	}
	
	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png)
	{
		printf("create png error");
		exit(-1);
	}
	
	png_infop info = png_create_info_struct(png);
	if (!info)
	{
		png_destroy_read_struct(&png, NULL, NULL);
		printf("create info error");
		exit(-1);
	}

	/* set error handling for set/longjmp methods of libpng error handling */	
	if (setjmp(png_jmpbuf(png)))
		abort();		
	
	png_init_io(png, fp);
	png_set_sig_bytes(png, CHECK_PNG_HEADER_SIZE);
	png_read_info(png, info);

	int width = png_get_image_width(png, info);
	int height = png_get_image_height(png, info);
	int color_type = png_get_color_type(png, info);
	int bit_depth = png_get_bit_depth(png, info);

	png_bytep* row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
	for (int i = 0; i < height; i++)
		row_pointers[i] = (png_byte*) malloc(png_get_rowbytes(png, info));
	
	png_read_image(png, row_pointers);
	fclose(fp);
	png_destroy_read_struct(&png, &info, NULL);
	
	READ_PNG_STRUCT pngstr = {
		.height=height,
		.width=width,
		.rows=row_pointers
	};	
	return pngstr;
}


typedef struct
{
	int path_weight;
	int prev_point;
} PNG_NODE_PATH;

int
find_min_idx(PNG_NODE_PATH* array, int size)
{
	int min = array[0].path_weight;
	int idx = 0;
	int tmp;

	for (int i = 1; i < size; i++)
	{
		tmp = array[i].path_weight;
		if (tmp < min)
		{
			min = tmp;
			idx = i;
		}
	}
	return idx;
}

void 
fill_row(int** weights, PNG_NODE_PATH** paths, int height, int width, int level)
{
	if (level == height - 1)
	{
		for (int i = 0; i < width; i++)
		{
			paths[level][0].path_weight = weights[level][i];
			paths[level][0].prev_point = -1;
		}
	}
	else
	{
		int idx;

		/* left border */
		idx = find_min_idx(paths[level + 1], 2);
		paths[level][0].path_weight = weights[level][0] + paths[level + 1][idx].path_weight;
		paths[level][0].prev_point = idx;

		/* from left + 1 to right - 1 */
		for (int i = 1; i < width - 1; i++)
		{
			idx = i - 1 + find_min_idx(paths[level + 1] + i - 1, 3);
			paths[level][i].path_weight = weights[level][i] + paths[level + 1][idx].path_weight;
			paths[level][i].prev_point = idx;
		}

		/* right border */
		idx = width - 2 + find_min_idx(paths[level + 1] + width - 2, 2);
		paths[level][width - 1].path_weight = weights[level][width - 1] + paths[level + 1][idx].path_weight;
		paths[level][width - 1].prev_point = idx;
	}
}
			

void
create_paths(int** weights, PNG_NODE_PATH** paths, int height, int width)
{
	for (int l = height - 1; l >= 0; l--)
		fill_row(weights, paths, height, width, l);
}
	

int*
find_min_path(int** weights, int height, int width)
{
	PNG_NODE_PATH** path = (PNG_NODE_PATH**) malloc(sizeof(PNG_NODE_PATH*) * height);
	for (int i = 0; i < height; i++)
	{
		path[i] = (PNG_NODE_PATH*) malloc(sizeof(PNG_NODE_PATH) * width);
	}

	create_paths(weights, path, height, width);
	
	int* min_path_idxs = (int*) malloc(sizeof(int) * height);
	min_path_idxs[0] = find_min_idx(path[0], width);
	int tmp = min_path_idxs[0];

	for (int i = 1; i < height; i++)
	{
		min_path_idxs[i] = path[i - 1][tmp].prev_point;
		tmp = min_path_idxs[i];
	}
	for (int i = 0; i < height; i++)
		free(path[i]);
	free(path);
	return min_path_idxs;
}

typedef struct
{
	int R;
	int G;
	int B;
} RGB_STRUCT;

/*
int
metric(RGB_STRUCT** rgb, int x, int y, int height, int width)
{
	int m = 0;
	int mr = 0, mg = 0, mb = 0;
	for (int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			if 	((x + i) >= 0 && (x + i) <= height - 1 &&
				((y + j) >= 0 && (y + j) <= width - 1))
			{
				mr = (rgb[x + i][y + j].R - rgb[x][y].R);
				mr *= 2;

				mg = (rgb[x + i][y + j].G - rgb[x][y].G);
				mg *= 2;

				mb = (rgb[x + i][y + j].B - rgb[x][y].B);
				mb *= 2;
				m += mr + mg + mb;
			}
		}
	}
	return m;
}
*/

int
metric(png_bytep* rows, int x, int y, int height, int width)
{
	int m = 0;
	int mr = 0, mg = 0, mb = 0;
	png_bytep p1, p2;
	for (int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			if 	((x + i) >= 0 && (x + i) <= height - 1 &&
				((y + j) >= 0 && (y + j) <= width - 1))
			{
				p1 = &rows[x + i][4 + (y + j)];
				p2 = &rows[x][4 * y];

				mr = (p1[0] - p2[0]);
				mr *= mr;

				mg = (p1[1] - p2[1]);
				mg *= mg;

				mb = (p1[2] - p2[2]);
				mb *= mb;

				m += mr + mg + mb;
			}
		}
	}
	return m;
}
void
fill_weights(png_bytep* rows, int** weights, int height, int width)
{
/*	RGB_STRUCT** rgb = (RGB_STRUCT**) malloc(sizeof(RGB_STRUCT*) * height);
	for (int i = 0; i < height; i++)
		rgb[i] = (RGB_STRUCT*) malloc(sizeof(RGB_STRUCT) * width);
	for (int i = 0; i < height; i++)
	{
		png_bytep row = rows[i];
		for (int j = 0; j < width; j++)
		{
			png_bytep px = &(row[j * 4]);
			rgb[i][j].R = px[0];
			rgb[i][j].G = px[1];
			rgb[i][j].B = px[2];
		}
	}
*/
	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++)
			weights[i][j] = metric(rows, i, j, height, width);
/*
	for (int i = 0; i < height; i++)
		free(rgb[i]);
	free(rgb);
*/
}

int
reduce_width(READ_PNG_STRUCT *png, int* dropper)
{
	if (!png->rows) 
		abort();

	for (int i = 0; i < png->height; i++)
		printf("%d ; ", dropper[i]);
	printf("\n\n");
	for (int i = 0; i < png->height; i++)
	{
		png_bytep row = png->rows[i];
    	for (int j = 0; j < png->width; j++)
		{
			if(j <= dropper[i])
			{
				continue;
			}
			if (j > dropper[i])
			{
				png_bytep px = &(row[(j - 1) * 4]);
				png_bytep px2 = &(row[j * 4]);
				px[0] = px2[0];
				px[1] = px2[1];
				px[2] = px2[2];
//				px[3] = px2[3];
	   		}
    	}
	}
	png->width -= 1;
	if (png->width >= 0)
		return 0;
	return -1;
}


void write_png_file(char *filename, READ_PNG_STRUCT* rpng) {
  int y;

  FILE *fp = fopen(filename, "wb");
  if(!fp) abort();

  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) abort();

  png_infop info = png_create_info_struct(png);
  if (!info) abort();

  if (setjmp(png_jmpbuf(png))) abort();

  png_init_io(png, fp);

  // Output is 8bit depth, RGBA format.
  png_set_IHDR(
    png,
    info,
    rpng->width, rpng->height,
    8,
    PNG_COLOR_TYPE_RGBA,
    PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_DEFAULT,
    PNG_FILTER_TYPE_DEFAULT
  );
  png_write_info(png, info);

  // To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
  // Use png_set_filler().
//  png_set_filler(png, 0, PNG_FILLER_AFTER);

  if (!rpng->rows) abort();

  png_write_image(png, rpng->rows);
  png_write_end(png, NULL);

  fclose(fp);

  png_destroy_write_struct(&png, &info);
}



int
main(int argc, char** argv)
{
	if (argc != 4)
	{
		printf("valid args");
		exit(-1);
	}
	int perc = atoi(argv[3]);
	fprintf(stderr, "Compression = %d perc.\n", perc);

	READ_PNG_STRUCT png = read_png(argv[1]);
	int cnt_iters = png.width * (100 - perc) / 100;
	
	int** weights = (int**) malloc(sizeof(int*) * png.height);
	for (int i = 0; i < png.height; i++)
		weights[i] = (int*) malloc(sizeof(int) * png.width);
	int* min_path;

	for (int i = 0; i < cnt_iters; i++)
	{
		fprintf(stderr, "... reduction : %d\n", i);
		fill_weights(png.rows, weights, png.height, png.width);

		min_path = find_min_path(weights, png.height, png.width);
		if (reduce_width(&png, min_path))
			abort();
	}
	write_png_file(argv[2], &png);
}
