/* n - height, m - width */
#include <stdio.h>
#include <stdlib.h>
int 
find_min_idx(int* arr, int size)
{
	int min = arr[0];
	int min_idx = 0;
	for (int i = 0; i < size; i++)
	{
		if (min > arr[i])
		{
			min = arr[i];
			min_idx = i;
		}
	}
	return min_idx;
}
		
void 
fill_empty(int** a, int** b, int** c, int n, int m, int level)
{
	if (level == 0) {
		for (int i = 0; i < m; i++)
		{
			b[level][i] = a[level][i];
			c[level][i] = -1;
		}
	}
	else
	{
		int idx;
		for (int i = 0; i < m; i++)
		{
			if (i == 0)
			{
				idx = find_min_idx(b[level - 1], 2);
				c[level][i] = idx;
			}
			else
			{
				if (i == m - 1)
				{
					idx = m - 2 + find_min_idx(b[level - 1] + m - 2, 2);
					c[level][i] = idx;
				}
				else
				{
					idx = i - 1 + find_min_idx(b[level - 1] + i - 1, 3);
					c[level][i] = idx;
				}
			}
			b[level][i] = a[level][i] + b[level - 1][idx];
		}	
	}
}

int main()
{
	int A[4][4] = {
		{10, 1, 2, 4},
		{2, 3, 4, 5},
		{4, 10, 23, 2},
		{28, 33, 32, 2}
	};
	int C[4][4] = {
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0}
	};

	int B[4][4] = {
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0}
	};
	int **a = malloc(sizeof(int*) * 4);
	int **b = malloc(sizeof(int*) * 4);
	int **c = malloc(sizeof(int*) * 4);

	for (int i = 0; i < 4; i++)
	{
		a[i] = malloc(sizeof(int) * 4);
		b[i] = malloc(sizeof(int) * 4);
		c[i] = malloc(sizeof(int) * 4);
		for (int j = 0; j < 4; j++)
		{
			a[i][j] = A[i][j];
			b[i][j] = B[i][j];
			c[i][j] = C[i][j];
		}
	}
	fill_empty(a, b, c, 4, 4, 0);
	fill_empty(a, b, c, 4, 4, 1);
	fill_empty(a, b, c, 4, 4, 2);
	fill_empty(a, b, c, 4, 4, 3);
/*	for (int i = 0; i < 4; i++) 
	{
		for (int j = 0; j < 4; j++)
		{
			fprintf(stderr, "%d ", b[i][j]);
		}
		fprintf(stderr, "\n");
	}
	for (int i = 0; i < 4; i++) 
	{
		for (int j = 0; j < 4; j++)
		{
			fprintf(stderr, "%d ", c[i][j]);
		}
		fprintf(stderr, "\n");
	}
*/	int min = find_min_idx(b[3], 4);
	fprintf(stderr, "min = %d ; ", min);
	for (int i = 1; i < 5; i++)
	{
		fprintf(stderr, "%d ; ", c[4 - i][min]);
		min = c[4 - i][min];
	}
	return 0;
}
