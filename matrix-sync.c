#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <time.h>
#include <limits.h>

// The number of rows for matrix1 and the number of columns for matrix2.
const int ROWS = 64;
// The number of columns for matrix1 and the rows of columns for matrix2.
const int COLUMNS = 32;
// Root process.
const int ROOT_PROCESS = 0;

// Generates the matrix of provided size.
void generateMatrix(int rows, int columns, int matrix[rows][columns]);
// Prints the matrix.
void printMatrix(int rows, int columns, int matrix[rows][columns]);
// Multiplies two matrices and stores the resultant elements into the <code>mul</code> product matrix.
void multiplyMatrix(int rows1, int columns1, int matrix1[rows1][columns1],
                    int rows2, int columns2, int matrix2[rows2][columns2],
                    int mul[rows1][columns2]);
// Stores the row from matrix to matrix_part.
void splitRow(int rows, int columns, int split_row, int matrix[rows][columns], int matrix_part[rows]);
// Stores the column of matrix to inverse_row_matrix.
void inverseColumnToRow(int rows, int columns, int inv_column, int matrix[rows][columns], int inverse_row_matrix[rows]);
// Prints the dashed lines.
void printDashedLine(int times);

int main(argc, argv) int argc;
char *argv[];
{
    int process_rank, process_size;

    MPI_Init(&argc, &argv);                       /* starts MPI */
    MPI_Comm_rank(MPI_COMM_WORLD, &process_rank); /* get current process id */
    MPI_Comm_size(MPI_COMM_WORLD, &process_size); /* get number of processes */

    if (process_rank == ROOT_PROCESS)
    {
        int matrix1[ROWS][COLUMNS];
        int matrix2[COLUMNS][ROWS];
        int mul[ROWS][ROWS];
        generateMatrix(ROWS, COLUMNS, matrix1);
        generateMatrix(COLUMNS, ROWS, matrix2);

        printMatrix(ROWS, COLUMNS, matrix1);
        printMatrix(COLUMNS, ROWS, matrix2);

        float starting_time = MPI_Wtime();
        printDashedLine(2);
        printf("Starting time: %f", starting_time);
        printDashedLine(2);

        int current_task = ROOT_PROCESS;
        for (int i = 0; i < ROWS; i++)
        {
            int matrix1_part[COLUMNS];
            inverseColumnToRow(ROWS, COLUMNS, i, matrix1, matrix1_part);
            for (int j = 0; j < ROWS; j++)
            {
                // Select the process in a round-robin fashion.
                if (current_task == process_size - 1)
                {
                    current_task = ROOT_PROCESS;
                }
                current_task++;

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

        // TERMINATES the processes by sending the INT_MIN as a signal.
        for (int task_index = 0; task_index < process_size; task_index++)
        {
            if (task_index == ROOT_PROCESS)
            {
                continue;
            }
            int exit_token = INT_MIN;
            MPI_Send(&exit_token, 1, MPI_INT, task_index, 0, MPI_COMM_WORLD);
        }

        // Note the ending time.
        float ending_time = MPI_Wtime();

        // Print the final product matrix.
        printMatrix(ROWS, ROWS, mul);
        
        printDashedLine(2);
        printf("Ending time: %f", ending_time);
        printDashedLine(2);

        // Expected final product matrix.
        printf("\n\nExpected\n");
        multiplyMatrix(ROWS, COLUMNS, matrix1, COLUMNS, ROWS, matrix2, mul);
        printMatrix(ROWS, ROWS, mul);

        // Time taken.
        float calc_time = ending_time - starting_time;
        printDashedLine(2);
        printf("Took %f", calc_time);
        printDashedLine(2);
    }

    MPI_Status status;
    if (process_rank != ROOT_PROCESS)
    {
        while (1)
        {
            int total_rows;
            // Number of elements will be sent by the root process.
            MPI_Recv(&total_rows, 1, MPI_INT, ROOT_PROCESS, 0, MPI_COMM_WORLD, &status);
            if (total_rows == INT_MIN)
            {
                // Process will termiate.
                break;
            }
            int matrix1_part[total_rows];
            int matrix2_part[total_rows];

            MPI_Recv(&matrix1_part, total_rows, MPI_INT, ROOT_PROCESS, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&matrix2_part, total_rows, MPI_INT, ROOT_PROCESS, 0, MPI_COMM_WORLD, &status);

            int product_matrix = 0;
            for (int row_index = 0; row_index < total_rows; row_index++)
            {
                product_matrix += matrix1_part[row_index] * matrix2_part[row_index];
            }

            MPI_Send(&product_matrix, 1, MPI_INT, ROOT_PROCESS, 0, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();
    return 0;
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