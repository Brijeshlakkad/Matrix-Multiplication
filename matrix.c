#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <time.h>
#include <limits.h>
// We will use the row-major order to store multidimensional arrays in linear storage such as random access memory.
// This also helps to scatter the elements of the array and process them in more easy way.
// Main benefit of this approach would be that time complexity will be O(log(N)) insteaad of O(N^2); where N is equal to the product of number of rows * columns.
// Reference: https://en.wikipedia.org/wiki/Row-_and_column-major_order

const int buffer_size = 2;
const int root_process = 0;

int *generateMatrix(int rows, int columns);
void printMatrix(int *matrix, int rows, int columns);
void multiplyMatrix(int rows1, int columns1, int matrix1[rows1][columns1],
                    int rows2, int columns2, int matrix2[rows2][columns2],
                    int mul[rows1][columns2]);
int multiply(int a, int b);
int *inverseColumnToRow(int *matrix, int rows, int columns);
// void inverseColumnToRow(int rows, int columns, int inv_column, int matrix[rows][columns], int inverse_row_matrix[rows]);
void printDashedLine(int times);

int main(argc, argv) int argc;
char *argv[];
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: please enter the dimension of the matrix(ROWS<space>COLUMNS<return>)\n");
        exit(1);
    }
    const int ROWS = atoi(argv[1]);
    const int COLUMNS = atoi(argv[2]);

    int process_rank, process_size;

    // multiplyMatrix(ROWS, COLUMNS, matrix1, COLUMNS, ROWS, matrix2, mul);

    // printMatrix(ROWS, ROWS, mul);

    if (MPI_Init(&argc, &argv) != MPI_SUCCESS)
    {
        perror("Error initializing MPI!");
        exit(1);
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &process_rank); /* get current process id */
    MPI_Comm_size(MPI_COMM_WORLD, &process_size); /* get number of processes */

    if (process_size < 2)
    {
        fprintf(stderr, "World size must be two for %s\n", argv[0]);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (process_rank == root_process)
    {
        float starting_time = MPI_Wtime();
        printDashedLine(2);
        printf("Starting time: %f", starting_time);
        printDashedLine(2);

        // Prepare matrices
        int *resultant_matrix;
        if ((resultant_matrix = malloc(ROWS * ROWS * sizeof(int))) == NULL)
        {
            printf("Resultant matrix cannot be created!");
            exit(1);
        }
        const int LENGTH_OF_METRIX = ROWS * COLUMNS;
        int *matrix1 = generateMatrix(ROWS, COLUMNS);
        int *matrix2 = generateMatrix(COLUMNS, ROWS);

        // printMatrix(ROWS, COLUMNS, matrix1);
        // printMatrix(COLUMNS, ROWS, matrix2);

        int current_task = root_process;
        int i = 0;
        while (i < LENGTH_OF_METRIX)
        {
            i += ROWS;
            if (current_task == process_size - 1)
            {
                current_task = root_process;
            }
            current_task++;
            // printf("Sending to %d\n", current_task);

            // Task id represents the process id.
            int taskId = current_task % process_size;
            int send_count = ROWS;
            int *matrix1_part;
            if ((matrix1_part = malloc(ROWS * sizeof(int))) == NULL)
            {
                printf("Resultant matrix cannot be created!");
                exit(1);
            }
            matrix1_part = matrix1[i];

            int matrix2_part = inverseColumnToRow(matrix2, COLUMNS, ROWS);

            MPI_Send(&COLUMNS, 1, MPI_INT, taskId, 0, MPI_COMM_WORLD);

            MPI_Send(matrix1_part, COLUMNS, MPI_INT, taskId, 0, MPI_COMM_WORLD);
            MPI_Send(matrix2_part, COLUMNS, MPI_INT, taskId, 0, MPI_COMM_WORLD);
            int recv_data;
            MPI_Recv(&recv_data, 1, MPI_INT, taskId, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            resultant_matrix[i - ROWS] = recv_data;
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

int *inverseColumnToRow(int *matrix, int rows, int columns)
{
    int *matrix_part;
    if ((matrix_part = malloc(columns * sizeof(int))) == NULL)
    {
        printf("Resultant matrix cannot be created!");
        exit(1);
    }
    int new_index = 0;
    for (int row_index = offset; row_index < columns; row_index = row_index + offset, new_index++)
    {
        matrix_part[new_index] = matrix[row_index];
    }
    return matrix_part;
}


// void inverseColumnToRow(int rows, int columns, int inv_row, int matrix[rows][columns], int inverse_row_matrix[rows])
// {
//     for (int column_index = 0; column_index < columns; column_index++)
//     {
//         inverse_row_matrix[column_index] = matrix[inv_row][column_index];
//     }
// }

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

int *generateMatrix(int rows, int columns)
{
    srand(time(NULL));
    int *matrix;
    if ((matrix = malloc(rows * columns * sizeof(int))) == NULL)
    {
        printf("Matrix cannot be created!");
        exit(1);
    }

    for (int index = 0; index < rows * columns; index++)
    {
        matrix[index] = rand() % 100;
    }

    return matrix;
}

void printMatrix(int *matrix, int rows, int columns)
{
    printf("\n");
    for (int row = 0; row < rows * columns; row++)
    {
        for (int column = 0; column < columns; column++)
        {
            printf("%d\t", matrix[row]);
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