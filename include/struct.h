#ifndef __STRUCT_PNG_REDUCER_H
#define __STRUCT_PNG_REDUCER_H

#define CHECK_PNG_HEADER_SIZE	8

// struct related to input argument line
typedef struct
{
	int correction;
	char* input;
	char* output;
	int percent;	
	int gamma;
} CMD_LINE_STRUCT;

// struct that returned after reading file
typedef struct
{
	int height;
	int width;
	png_bytep* rows;
	int color_type;
	int bit_depth;
	png_bytep (*get_pixel) (png_bytep, int);
	int (*metric) (png_bytep*, int, int, int, int, int);
	void (*change_pixel) (png_bytep, png_bytep);
	int correction;
	float gamma;
} READ_PNG_STRUCT;


// struct that describes accumulated weight in dynamic algorithm (path_weight)
// and
// previous point in minimal path (prev_point)
typedef struct
{
	int path_weight;
	int prev_point;
} PNG_NODE_PATH;

#endif
