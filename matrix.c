/* C Example */
#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <time.h>
#include <limits.h>

const int ROWS = 32;
const int COLUMNS = 32;
const int buffer_size = 2;
const int root_process = 2;

void generateMatrix(int rows, int columns, int matrix[rows][columns]);
void printMatrix(int rows, int columns, int matrix[rows][columns]);
void multiplyMatrix(int rows1, int columns1, int matrix1[rows1][columns1],
                    int rows2, int columns2, int matrix2[rows2][columns2],
                    int mul[rows1][columns2]);
int multiply(int a, int b);
void splitRow(int rows, int columns, int split_row, int matrix[rows][columns], int matrix_part[rows]);
void inverseColumnToRow(int rows, int columns, int inv_column, int matrix[rows][columns], int inverse_row_matrix[rows]);
void printDashedLine(int times);

int main(argc, argv) int argc;
char *argv[];
{
    int process_rank, process_size;

    // multiplyMatrix(ROWS, COLUMNS, matrix1, COLUMNS, ROWS, matrix2, mul);

    // printMatrix(ROWS, ROWS, mul);

    MPI_Init(&argc, &argv);                       /* starts MPI */
    MPI_Comm_rank(MPI_COMM_WORLD, &process_rank); /* get current process id */
    MPI_Comm_size(MPI_COMM_WORLD, &process_size); /* get number of processes */

    if (process_rank == root_process)
    {
        float starting_time = MPI_Wtime();
        printDashedLine(2);
        printf("Starting time: %f", starting_time);
        printDashedLine(2);
        int matrix1[ROWS][COLUMNS];
        int matrix2[COLUMNS][ROWS];
        int mul[ROWS][ROWS];
        generateMatrix(ROWS, COLUMNS, matrix1);
        generateMatrix(COLUMNS, ROWS, matrix2);

        // printMatrix(ROWS, COLUMNS, matrix1);
        // printMatrix(COLUMNS, ROWS, matrix2);

        int current_task = root_process;
        for (int i = 0; i < ROWS; i++)
        {
            int matrix1_part[COLUMNS];
            inverseColumnToRow(ROWS, COLUMNS, i, matrix1, matrix1_part);
            for (int j = 0; j < ROWS; j++)
            {

                if (current_task == process_size - 1)
                {
                    current_task = root_process;
                }
                current_task++;
                // printf("Sending to %d\n", current_task);

                // Task id represents the process id.
                int taskId = current_task % process_size;

                int matrix2_part[COLUMNS];
                splitRow(COLUMNS, ROWS, j, matrix2, matrix2_part);

                MPI_Send(&COLUMNS, 1, MPI_INT, taskId, 0, MPI_COMM_WORLD);

                MPI_Send(&matrix1_part, COLUMNS, MPI_INT, taskId, 0, MPI_COMM_WORLD);
                MPI_Send(&matrix2_part, COLUMNS, MPI_INT, taskId, 0, MPI_COMM_WORLD);

                int recv_data;
                MPI_Recv(&recv_data, 1, MPI_INT, taskId, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                mul[i][j] = recv_data;
            }
        }

        // TERMINATES the processes.
        for (int task_index = 0; task_index < process_size; task_index++)
        {
            if (task_index == root_process)
            {
                continue;
            }
            int exit_token = INT_MIN;
            MPI_Send(&exit_token, 1, MPI_INT, task_index, 0, MPI_COMM_WORLD);
        }

        // printMatrix(ROWS, ROWS, mul);

        float ending_time = MPI_Wtime();
        printDashedLine(2);
        printf("Ending time: %f", ending_time);
        printDashedLine(2);

        float calc_time = ending_time - starting_time;
        printDashedLine(2);
        printf("Took %f", calc_time);
        printDashedLine(2);
        // printf("\n\nExpected\n");
        // multiplyMatrix(ROWS, COLUMNS, matrix1, COLUMNS, ROWS, matrix2, mul);
        // printMatrix(ROWS, ROWS, mul);
    }

    MPI_Status status;
    if (process_rank != root_process)
    {
        while (1)
        {
            int total_rows;
            MPI_Recv(&total_rows, 1, MPI_INT, root_process, 0, MPI_COMM_WORLD, &status);
            if (total_rows == INT_MIN)
            {
                break;
            }
            int matrix1_part[total_rows];
            int matrix2_part[total_rows];
            MPI_Recv(&matrix1_part, total_rows, MPI_INT, root_process, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&matrix2_part, total_rows, MPI_INT, root_process, 0, MPI_COMM_WORLD, &status);

            int product_matrix = 0;
            for (int row_index = 0; row_index < total_rows; row_index++)
            {
                product_matrix += matrix1_part[row_index] * matrix2_part[row_index];
            }

            // printf("Process %d received token %d\n", process_rank, product_matrix);

            MPI_Send(&product_matrix, 1, MPI_INT, root_process, 0, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();
    return 0;
}

int multiply(int a, int b)
{
    return a * b;
}

void splitRow(int rows, int columns, int split_row, int matrix[rows][columns], int matrix_part[rows])
{
    for (int row_index = 0; row_index < rows; row_index++)
    {
        matrix_part[row_index] = matrix[row_index][split_row];
    }
}

void inverseColumnToRow(int rows, int columns, int inv_row, int matrix[rows][columns], int inverse_row_matrix[rows])
{
    for (int column_index = 0; column_index < columns; column_index++)
    {
        inverse_row_matrix[column_index] = matrix[inv_row][column_index];
    }
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

void printDashedLine(int times)
{
    int times_done = times;
    printf("\n");
    while (times_done > 0)
    {
        printf("-------------------------------\n");
        times_done--;
    }
}