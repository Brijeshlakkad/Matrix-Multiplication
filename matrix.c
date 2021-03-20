/* C Example */
#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <time.h>

const int N = 3;

void generateMatrix(int matrix[N][N]);
void printMatrix(int matrix[N][N]);
void multiply(int matrix1[N][N], int matrix2[N][N], int mul[N][N]);

int main(argc, argv) int argc;
char *argv[];
{
    int rank, size;
    int matrix1[N][N];
    int matrix2[N][N];
    int mul[N][N];
    generateMatrix(matrix1);
    generateMatrix(matrix2);

    printMatrix(matrix1);
    printMatrix(matrix2);

    multiply(matrix1, matrix2, mul);

    printMatrix(mul);

    MPI_Init(&argc, &argv);               /* starts MPI */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); /* get current process id */
    MPI_Comm_size(MPI_COMM_WORLD, &size); /* get number of processes */
    printf("Hello world from process %d of %d\n", rank, size);
    MPI_Finalize();
    return 0;
}

void multiply(int matrix1[N][N], int matrix2[N][N], int mul[N][N])
{
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        { 
            mul[i][j] = 0;
            for (int k = 0; k < N; k++)
            {
                mul[i][j] += matrix1[i][k] * matrix2[k][j];
            }
        }
    }
}

void generateMatrix(int matrix[N][N])
{
    srand(time(NULL));
    for (int row = 0; row < N; row++)
        for (int column = 0; column < N; column++)
            matrix[row][column] = rand() % 100;
}

void printMatrix(int matrix[N][N])
{
    printf("\n");
    for (int row = 0; row < N; row++)
    {
        for (int column = 0; column < N; column++)
        {
            printf("%d\t", matrix[row][column]);
        }
        printf("\n");
    }
}