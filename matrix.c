/* C Example */
#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <time.h>

const int ROWS = 32;
const int COLUMNS = 3;

void generateMatrix(int rows, int columns, int matrix[rows][columns]);
void printMatrix(int rows, int columns, int matrix[rows][columns]);
void multiply(int rows1, int columns1, int matrix1[rows1][columns1],
              int rows2, int columns2, int matrix2[rows2][columns2],
              int mul[rows1][columns2]);

int main(argc, argv) int argc;
char *argv[];
{
    int rank, size;
    int matrix1[ROWS][COLUMNS];
    int matrix2[COLUMNS][ROWS];
    int mul[ROWS][ROWS];
    generateMatrix(ROWS, COLUMNS, matrix1);
    generateMatrix(COLUMNS, ROWS, matrix2);

    printMatrix(ROWS, COLUMNS, matrix1);
    printMatrix(COLUMNS, ROWS, matrix2);

    multiply(ROWS, COLUMNS, matrix1, COLUMNS, ROWS, matrix2, mul);

    printMatrix(ROWS, ROWS, mul);

    MPI_Init(&argc, &argv);               /* starts MPI */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); /* get current process id */
    MPI_Comm_size(MPI_COMM_WORLD, &size); /* get number of processes */
    printf("Hello world from process %d of %d\n", rank, size);
    MPI_Finalize();
    return 0;
}

void multiply(int rows1, int columns1, int matrix1[rows1][columns1],
              int rows2, int columns2, int matrix2[rows2][columns2],
              int mul[rows1][columns2])
{
    for (int i = 0; i < rows1; i++)
    {
        for (int j = 0; j < columns2; j++)
        {
            mul[i][j] = 0;
            for (int k = 0; k < rows2; k++)
            {
                mul[i][j] += matrix1[i][k] * matrix2[k][j];
            }
        }
    }
}

void generateMatrix(int rows, int columns, int matrix[rows][columns])
{
    srand(time(NULL));
    for (int row = 0; row < rows; row++)
        for (int column = 0; column < columns; column++)
            matrix[row][column] = rand() % 100;
}

void printMatrix(int rows, int columns, int matrix[rows][columns])
{
    printf("\n");
    for (int row = 0; row < rows; row++)
    {
        for (int column = 0; column < columns; column++)
        {
            printf("%d\t", matrix[row][column]);
        }
        printf("\n");
    }
}