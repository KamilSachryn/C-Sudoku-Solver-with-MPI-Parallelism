/******************************************************************************
Title : sudoku_solver.c
Author : Kamil Sachryn
Created on : December 7th 20201
Description : Parallel Backtracking based sudoku solver
Purpose : Demonstarte parrerel processing of a backtracking algorithm
Usage : mpirun -np n sudoku_solver board start_depth
Build with : mpicc -o sudoku_solver sudoku_solver.c
******************************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <time.h>

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "mpi.h"


int id;
int p;
int cutoff_count = 0;
int cutoff_depth = 0;


//is number num in given row
int used_in_row(int grid[9][9], int row, int num)
{
	for (int col = 0; col < 9; col++)
	{
		if (grid[row][col] == num)
		{
			return 1;
		}
	}
	return 0;
}
//is number num in given collumn
int used_in_col(int grid[9][9], int col, int num)
{
	for (int row = 0; row < 9; row++)
	{
		if (grid[row][col] == num)
		{
			return 1;
		}
	}
	return 0;
}

//is number num in given box
int used_in_box(int grid[9][9], int box_row, int box_col, int num)
{
	for (int row = 0; row < 3; row++)
	{
		for (int col = 0; col < 3; col++)
		{
			if (grid[row + box_row][col + box_col] == num)
			{
				return 1;
			}
		}
	}
	return 0;
}

int is_safe(int grid[9][9], int row, int col, int num)
{


	return !used_in_row(grid, row, num)
		&& !used_in_col(grid, col, num)
		&& !used_in_box(grid, row - row % 3,
			col - col % 3, num)
		&& grid[row][col] == 0;
}

void print_in_order()
{
	if (id == 0)
	{
		int* tmp = malloc(1 * sizeof(int));
		for (int i = 1; i < p; i++)
			MPI_Send(tmp, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
	}
	else
	{
		int* res = malloc(1 * sizeof(int));
		MPI_Recv(res, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}
	sleep(id / 2);
}


void print_grid(int grid[9][9])
{
	for (int i = 0; i < 9; i++)
	{
		if (i % 3 == 0)
		{
			printf("+-------+-------+-------+\n");
		}

		for (int j = 0; j < 9; j++)
		{
			if (j % 3 == 0)
			{
				printf("| ");
			}
			printf("%i ", grid[i][j]);
			if (j == 8)
			{

				printf("|");

			}

		}

		printf("\n");

	}
	printf("+-------+-------+-------+\n\n");
}


int solve(int grid[9][9], int level)
{
	int row, col;

	//Begin parallelization once level reaches cutoff depth
	if (level == cutoff_depth)
	{
		cutoff_count++;
		//Interleaf processors at cutoff depth
		if (cutoff_count % p != id)
		{
			return 0;
		}
	}

	int found_unassigned = 0;

	//Find an empty square
	for (row = 0; row < 9; row++)
	{
		for (col = 0; col < 9; col++)
		{
			if (grid[row][col] == 0)
			{
				found_unassigned = 1;
				break;
			}
		}
		if (found_unassigned == 1)
		{
			break;
		}
	}

	//if no squares are empty we found a solution 
	if (found_unassigned == 0)
	{
		return 1;
	}

	//try numbers until valid option is found
	for (int num = 1; num <= 9; num++)
	{
		//check if number is safe
		if (is_safe(grid, row, col, num))
		{
			//if safe, recurse down 
			grid[row][col] = num;
			if (solve(grid, level + 1))
			{
				return 1;
			}
			//recursion reached dead end in a descendant, reset back to 0
			grid[row][col] = 0;
		}
	}
	//Backtrack
	return 0;
}


int main(int argc, char* argv[])
{
	//Set up MPI and parse inputs
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	MPI_Comm_size(MPI_COMM_WORLD, &p);

	const int root = 0;
	int grid[9][9];

	char file_location[256];
	strcpy(file_location, argv[1]);

	FILE* p_file;
	p_file = fopen(file_location, "r");
	if (p_file == NULL)
	{
		printf("Invalid file\n");
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	for (int i = 0; i < 9; ++i)
	{
		for (int j = 0; j < 9; ++j)
		{
			fscanf(p_file, "%i", &grid[i][j]);
		}
	}

	cutoff_depth = (int)strtol(argv[2], NULL, 10);

	//Finished setting up mpi / inputs

	if (id == root)
	{
		printf("Input Grid:\n");
		print_grid(grid);

		printf("\n\n\n");
	}

	int solved = solve(grid, 0);

	print_in_order();

	if (solved)
	{
		printf("p%i found solution: \n", id);
		print_grid(grid);
	}



	MPI_Finalize();
	return 0;

}
