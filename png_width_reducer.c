/* Vlasov Roman. June 2021 */

#include <stdio.h> 
#include <stdlib.h>
#include <png.h>
#include <string.h>
#include <math.h>


#include "include/struct.h"
#include "include/format.h"
#include "include/macros.h"

// cmdline parsing
CMD_LINE_STRUCT
parse_argv_line(int argc, char** argv)
{
	if (argc != 4)
	{
		fprintf(stderr, "Usage: ./exe input.png output.png percentage");
		exit(0);
	}

	CMD_LINE_STRUCT cls = {
		.correction = WITHOUT_CORRECTION,
		.input = argv[1],
		.output = argv[2]
	};

	int perc = atoi(argv[3]);
	if (perc > 0 && perc <= 100)
		cls.percent = perc;	
	else
	{
		fprintf(stderr, "invalid percentage\n");
		abort();
	}
	
	char* gamma = getenv("GAMMA_CORRECTION");
	if (gamma == NULL)
	{
		cls.correction = WITHOUT_CORRECTION;
		cls.gamma = 1;
	}
	else 
	{
		cls.correction = WITH_CORRECTION;
		cls.gamma = atof(gamma);
		if (cls.gamma <= 0)
		{
			fprintf(stderr, "invalid gamma value in envvar\n");
			abort();
		}
	}
	return cls;
}

// check if input file is png
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

// reading function	
READ_PNG_STRUCT
read_png(char *name, int correction, float gamma)
{
	printf("--- Start reading ---\n");
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
	
	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL); if (!png)
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

	if (setjmp(png_jmpbuf(png)))
		abort();		
	
	png_init_io(png, fp);
	png_set_sig_bytes(png, CHECK_PNG_HEADER_SIZE);
	png_read_info(png, info);

	int width = png_get_image_width(png, info);
	int height = png_get_image_height(png, info);
	int color_type = png_get_color_type(png, info);
	int bit_depth = png_get_bit_depth(png, info);

	if (bit_depth <= 4)
	{
		png_set_packing(png); // 1,2,4 bytes -> 8
		bit_depth = 8;
	}
	if (bit_depth == 16)
	{
		png_set_strip_16(png);
		bit_depth = 8;
	}

	READ_PNG_STRUCT pngstr = {
		.height = height,
		.width = width,
		.color_type = color_type,
		.bit_depth = bit_depth,
		.correction = correction
	};	

	fprintf(stdout, "... Reading png with height=%d, width=%d (bitdepth=%d)\n", height, width, bit_depth);

	if (color_type == PNG_COLOR_TYPE_RGB)
	{
		fprintf(stdout, "... color type : RGB\n");

		pngstr.metric = metric_rgb;
		pngstr.get_pixel = get_pixel_rgb;
		pngstr.change_pixel = change_pixel_rgb;
	}
	if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
	{
		fprintf(stdout, "... color type : RGBA\n");

		pngstr.metric = metric_rgba;
		pngstr.get_pixel = get_pixel_rgba;
		pngstr.change_pixel = change_pixel_rgba;
	}
	if (color_type == PNG_COLOR_TYPE_PALETTE)
	{
		fprintf(stdout, "... color type : PALETTE\n");

		png_set_palette_to_rgb(png);
		pngstr.metric = metric_rgb;
		pngstr.get_pixel = get_pixel_rgb;
		pngstr.change_pixel = change_pixel_rgb;
		pngstr.color_type = PNG_COLOR_TYPE_RGB;
	}
	if (color_type == (PNG_COLOR_TYPE_GRAY | PNG_COLOR_TYPE_GRAY_ALPHA))
	{
		fprintf(stderr, "png file has unsupported color type (gray)\n");
		abort();
	}

	if (correction == WITH_CORRECTION)
	{
		png_set_gamma(png, PNG_GAMMA_LINEAR, 1 / gamma);
 		fprintf(stderr, "--- Enable gamma correction while reading\n");
	}

	// updating read information
	png_read_update_info(png, info);

	png_bytep* row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
	if (!row_pointers)
	{
		fprintf(stderr, "malloc error\n");
		abort();
	}
	for (int i = 0; i < height; i++)
	{
		row_pointers[i] = (png_byte*) malloc(png_get_rowbytes(png, info));
		if (!row_pointers[i])
		{
			fprintf(stderr, "malloc error\n");
			abort();
		}
	}
	
	png_read_image(png, row_pointers);
	pngstr.rows = row_pointers;

	// free memory
	fclose(fp);
	png_destroy_read_struct(&png, &info, NULL);
	
	printf("--- Finish reading ---\n\n");
	return pngstr;
}

// find index of minimal element of array[0,...,size - 1]
int
find_min_idx(PNG_NODE_PATH* array, int size)
{
	int min = array[0].path_weight;
	int idx = 0;
	int tmp = 0;

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

/* dynamic algorithm to find minimal paths */
/* algo */
void 
fill_row(int** weights, PNG_NODE_PATH** paths, int height, int width, int level)
{
	if (level == height - 1)
	{
		for (int i = 0; i < width; i++)
		{
			paths[level][i].path_weight = weights[level][i];
			paths[level][i].prev_point = -1;
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
find_min_path(PNG_NODE_PATH** path, int** weights, int* min_path_idxs, int height, int width)
{
	create_paths(weights, path, height, width);
	
	min_path_idxs[0] = find_min_idx(path[0], width);
	int tmp = min_path_idxs[0];

	for (int i = 1; i < height; i++)
	{
		min_path_idxs[i] = path[i - 1][tmp].prev_point;
		tmp = min_path_idxs[i];
	}
	return min_path_idxs;
}
/* end algo */

/* calculate weights of points */
void
fill_weights(READ_PNG_STRUCT* png, int** weights)
{
	int m = 0, tmp = 0;
	png_bytep p1, p2;
	for (int x = 0; x < png->height; x++)
	{
		for (int y = 0; y < png->width; y++)
		{
			weights[x][y] = png->metric(png->rows, x, y, png->height, png->width, png->correction);
		}
	}
}

int
reduce_width(READ_PNG_STRUCT *png, int* dropper)
{
	if (!png->rows) 
		abort();

	for (int i = 0; i < png->height; i++)
	{
		png_bytep row = png->rows[i];
    	for (int j = 0; j < png->width; j++)
		{
			png_bytep px = &(row[(j * 4)]);
			if (j > dropper[i])
			{
				png_bytep px1 = png->get_pixel(row, j - 1);
				png_bytep px2 = png->get_pixel(row, j);
				png->change_pixel(px1, px2);
	   		}
    	}
	}
	png->width -= 1;
	if (png->width >= 0)
		return 0;
	return -1;
}

void
write_transform_gamma_pixels(png_structp png, png_row_infop info, png_bytep data)
{
	
	/* Contents of row_info:
	*  png_uint_32 width      width of row
	*  png_uint_32 rowbytes   number of bytes in row
	*  png_byte color_type    color type of pixels
	*  png_byte bit_depth     bit depth of samples
	*  png_byte channels      number of channels (1-4)
	*  png_byte pixel_depth   bits per pixel (depth*channels)
    */

	/* Counts the number of zero samples (or zero pixels if color_type is 3 */
	png_bytep dp = data;

	// C_initial = C_linear ** inv_gamma;
	// gamma = 2.4 -> inv_gamma = 1 / 2.4
	float inv_gamma = 1 / atof(getenv("GAMMA_CORRECTION"));

	if (png == NULL)
		return;

	for (int i = 0; i < info->width; i++)
	{
		for (int j = 0; j < info->channels; j++)
		{
			if (info->bit_depth == 8)
			{
				*dp = pow((float)*dp / 255, inv_gamma) * 255;
				dp++;
			}
			else
			{
				fprintf(stderr, "Unhandled bit_depth (output)\n");
				abort();
			}
		}
	}
}

// write output
void 
write_png_file(char *filename, READ_PNG_STRUCT* rpng)
{
	printf("--- Start writing ---\n");
	FILE *fp = fopen(filename, "wb");
	if (!fp) 
		abort();

	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png) 
		abort();

	png_infop info = png_create_info_struct(png);
	if (!info) 
		abort();

	if (setjmp(png_jmpbuf(png))) 
		abort();

	png_init_io(png, fp);

	png_set_IHDR(	png, info,
					rpng->width, 
					rpng->height,
					rpng->bit_depth, 
					rpng->color_type,
					PNG_INTERLACE_NONE,
					PNG_COMPRESSION_TYPE_DEFAULT,
					PNG_FILTER_TYPE_DEFAULT
  	);
		
	png_write_info(png, info);

	// set_gAMA not working, because it is only change png metadata:(
	// but i want to change every pixel	
	if (rpng->correction == WITH_CORRECTION)
	{
		png_set_write_user_transform_fn(png, write_transform_gamma_pixels);
		fprintf(stdout, "--- Reverse gamma correction\n");
	}

	if (!rpng->rows) 
		abort();

	png_write_image(png, rpng->rows);
	png_write_end(png, NULL);

	fclose(fp);

	png_destroy_write_struct(&png, &info);
	printf("--- Stop writing ---\n\n");
}

// memory free
void
free_alloced_main(READ_PNG_STRUCT* png, PNG_NODE_PATH** paths, int** weights, int* min_path, int height)
{
	for (int i = 0; i < height; i++)
	{
		free(png->rows[i]);
		free(paths[i]);
		free(weights[i]);
	}
	free(png->rows);
	free(paths);
	free(weights);
	free(min_path);
}

// start point
int
main(int argc, char** argv)
{
	CMD_LINE_STRUCT cmd = parse_argv_line(argc, argv);

	fprintf(stdout, "Compression = %d perc.\n", cmd.percent);

	READ_PNG_STRUCT png = read_png(cmd.input, cmd.correction, cmd.gamma);
	int cnt_iters = png.width * (100 - cmd.percent) / 100;
	
	// special for dynamic algorithm
	PNG_NODE_PATH** path = (PNG_NODE_PATH**) malloc(sizeof(PNG_NODE_PATH*) * png.height);
	for (int i = 0; i < png.height; i++)
	{
		path[i] = (PNG_NODE_PATH*) malloc(sizeof(PNG_NODE_PATH) * png.width);
	}

	// weights of points
	int** weights = (int**) malloc(sizeof(int*) * png.height);
	if (!weights)
	{
		fprintf(stderr, "malloc error\n");
		abort();
	}
	for (int i = 0; i < png.height; i++)
	{
		weights[i] = (int*) malloc(sizeof(int) * png.width);
		if (!weights[i])
		{
			fprintf(stderr, "malloc error\n");
			abort();
		}
	} 

	// deleted path
	int* min_path = (int*) malloc(sizeof(int) * png.height); 
	if (!min_path)
	{
		fprintf(stderr, "malloc error\n");
		abort();
	}

	// delete cnt_iters paths
	printf("--- Start algorithm ---\n");
	for (int i = 0; i < cnt_iters; i++) 
	{
		if (i % 5 == 0)
			fprintf(stdout, "... reduction : %d percent\n", i * 100 / cnt_iters);
		
		fill_weights(&png, weights);

		find_min_path(path, weights, min_path, png.height, png.width);
		if (reduce_width(&png, min_path))
			abort();
	}
	printf("--- Finish algorithm ---\n\n");

	// writing and freeing
	write_png_file(cmd.output, &png);
	free_alloced_main(&png, path, weights, min_path, png.height);
	
	return 0;
}
