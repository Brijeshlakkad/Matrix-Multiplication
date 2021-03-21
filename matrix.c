/* C Example */
#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <time.h>
#include <limits.h>

const int ROWS = 32;
const int COLUMNS = 3;
const int buffer_size = 2;

void generateMatrix(int rows, int columns, int matrix[rows][columns]);
void printMatrix(int rows, int columns, int matrix[rows][columns]);
void multiplyMatrix(int rows1, int columns1, int matrix1[rows1][columns1],
                    int rows2, int columns2, int matrix2[rows2][columns2],
                    int mul[rows1][columns2]);
int multiply(int a, int b);

int main(argc, argv) int argc;
char *argv[];
{
    int process_rank, process_size;

    // multiplyMatrix(ROWS, COLUMNS, matrix1, COLUMNS, ROWS, matrix2, mul);

    // printMatrix(ROWS, ROWS, mul);

    MPI_Init(&argc, &argv);                       /* starts MPI */
    MPI_Comm_rank(MPI_COMM_WORLD, &process_rank); /* get current process id */
    MPI_Comm_size(MPI_COMM_WORLD, &process_size); /* get number of processes */

    printf("Hello world from process %d of %d\n", process_rank, process_size);

    if (process_rank == 0)
    {
        int matrix1[ROWS][COLUMNS];
        int matrix2[COLUMNS][ROWS];
        int mul[ROWS][ROWS];
        generateMatrix(ROWS, COLUMNS, matrix1);
        generateMatrix(COLUMNS, ROWS, matrix2);

        printMatrix(ROWS, COLUMNS, matrix1);
        printMatrix(COLUMNS, ROWS, matrix2);

        int current_task = 0;
        for (int i = 0; i < ROWS; i++)
        {
            for (int j = 0; j < ROWS; j++)
            {
                mul[i][j] = 0;
                for (int k = 0; k < COLUMNS; k++)
                {
                    if (current_task == process_size - 1)
                    {
                        current_task = 0;
                    }
                    current_task++;
                    printf("Sending to %d\n", current_task);
                    // Task id represents the process id.
                    int taskId = current_task % process_size;

                    // Pack the data
                    int data[buffer_size];
                    data[0] = matrix1[i][k];
                    data[1] = matrix2[k][j];
                    MPI_Send(&data, buffer_size, MPI_INT, taskId, 0, MPI_COMM_WORLD);

                    int recv_data;
                    MPI_Recv(&recv_data, 1, MPI_INT, taskId, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    printf("Process %d received token %d\n", process_rank, recv_data);

                    mul[i][j] += recv_data;
                }
            }
        }

        for (int task_index = 1; task_index < process_size; task_index++)
        {
            int data[buffer_size];
            data[0] = INT_MIN;
            data[1] = INT_MAX;
            MPI_Send(data, buffer_size, MPI_INT, task_index, 0, MPI_COMM_WORLD);
        }

        printMatrix(ROWS, ROWS, mul);
        // printf("\n\nExpected\n");
        // multiplyMatrix(ROWS, COLUMNS, matrix1, COLUMNS, ROWS, matrix2, mul);
        // printMatrix(ROWS, ROWS, mul);
    }

    if (process_rank != 0)
    {
        int rec_buffer_size = 2;
        while (rec_buffer_size > 0)
        {
            int recv_data[2];
            MPI_Recv(recv_data, buffer_size, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (recv_data[0] == INT_MIN && recv_data[1] == INT_MAX)
            {
                break;
            }

            int ans = recv_data[0] * recv_data[1];
            printf("Process %d received token %d * %d = %d\n", process_rank, recv_data[0], recv_data[1], ans);

            MPI_Send(&ans, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
    }
    printf("Process %d EXITING\n", process_rank);

    MPI_Finalize();
    return 0;
}

int multiply(int a, int b)
{
    return a * b;
}

void multiplyMatrix(int rows1, int columns1, int matrix1[rows1][columns1],
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