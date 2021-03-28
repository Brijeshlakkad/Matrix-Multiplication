#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <time.h>
#include <limits.h>
// We will use the row-major order to store multidimensional arrays in linear storage such as random access memory.
// This also helps to scatter the elements of the array and process them in more easy way.
// Reference: https://en.wikipedia.org/wiki/Row-_and_column-major_order

// Root process.
const int ROOT_PROCESS = 0;
// Generates the matrix of provided size.
void generateMatrix(int *matrix, int rows, int columns);
// Prints the matrix.
void printMatrix(int *matrix, int rows, int columns);
// Multiplies two matrices and prints the product matrix.
void multiplyMatrix(int *matrix1, int rows1, int columns1, int *matrix2, int rows2, int columns2);
// Prints the dashed lines.
void printDashedLine(int times);
// To print the partial received matrix.
void printPartialMatrix(int *matrix, int size);

int main(argc, argv) int argc;
char *argv[];
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: please enter the dimension of the matrix(ROWS<space>32<return>)\n");
        exit(1);
    }
    const int ROWS = atoi(argv[1]);
    const int COLUMNS = atoi(argv[2]);

    int process_rank, process_size;

    if (MPI_Init(&argc, &argv) != MPI_SUCCESS)
    {
        perror("Error initializing MPI!");
        exit(1);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &process_rank); /* get current process id */
    MPI_Comm_size(MPI_COMM_WORLD, &process_size); /* get number of processes */

    if (ROWS % process_size != 0)
    {
        if (process_rank == 0)
        {
            printDashedLine(1);
            fprintf(stderr, "Usage: please enter process size/rows such that number_of_rows %% number_of_process == 0\nOR\nEnter the different numbers for rows!\n");
        }
        MPI_Finalize();
        exit(1);
    }

    if (process_size < 2)
    {
        fprintf(stderr, "World size must be two for %s\n", argv[0]);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // To store the starting time.
    float starting_time;

    int LENGTH_OF_METRIX = ROWS * COLUMNS;
    // Will be allocated memory only by the root process
    int *matrix1;

    // As second matrix must be possessed by every process, memory allocation should be done by every process.
    int *matrix2;
    if ((matrix2 = malloc(LENGTH_OF_METRIX * sizeof(int))) == NULL)
    {
        printf("Second matrix cannot be created!");
        exit(1);
    }

    if (process_rank == ROOT_PROCESS)
    {
        if ((matrix1 = (int *)malloc(LENGTH_OF_METRIX * sizeof(int))) == NULL)
        {
            printf("First matrix cannot be created!");
            exit(1);
        }
        generateMatrix(matrix1, ROWS, COLUMNS);
        generateMatrix(matrix2, COLUMNS, ROWS);

        printMatrix(matrix1, ROWS, COLUMNS);
        printMatrix(matrix2, COLUMNS, ROWS);

        // Notes the starting time.
        starting_time = MPI_Wtime();
        printDashedLine(2);
        printf("Starting time: %f", starting_time);
        printDashedLine(2);
    }
    // Number of elements need to be sent to each process.
    int send_count = LENGTH_OF_METRIX / process_size;

    // Will store the received elements for matrix1.
    int matrix1_rows[send_count];

    // Scatters the matrix1 elements
    MPI_Scatter(matrix1, send_count, MPI_INT, matrix1_rows, send_count, MPI_INT, ROOT_PROCESS, MPI_COMM_WORLD);
    // Broadcasts the matrix2 to the all processes.
    MPI_Bcast(matrix2, LENGTH_OF_METRIX, MPI_INT, ROOT_PROCESS, MPI_COMM_WORLD);

    // Columns of matrix 2
    // Resultant product matrix will be the size of this columns of matrix 2.
    // Because ROWS % number_of_process = 0
    int product_matrix_rows = send_count / COLUMNS;

    // printf("send_count %d\n", send_count);
    // printf("product_matrix_rows %d\n", product_matrix_rows);
    int *product_matrix;

    // Example to understand the below steps.
    // Let's assume, matrix1 = 4x3 and matrix2 = 3x4 and number_of_processes = 2
    // Sending half of matrix1 to each process. (2x3 of matrix1 and sending whole matrix2 to all the processes)
    // matrix1 will have a resultant half product matrix of (2x4).
    // ROWS is number of rows for matrix1 and number of columns for matrix2.
    // COLUMNS is number of columns for matrix1 and number of rows for matrix2.
    int product_matrix_length = product_matrix_rows * ROWS;
    if ((product_matrix = malloc((product_matrix_length) * sizeof(int))) == NULL)
    {
        printf("Resultant matrix cannot be created!");
        exit(1);
    }

    // Uses three linear equations to calculate the position of elements in the respective matrix.
    for (int product_matrix_row_index = 0; product_matrix_row_index < product_matrix_rows; product_matrix_row_index++)
    {
        for (int row_index = 0; row_index < ROWS; row_index++)
        {
            int product_matrix_index = product_matrix_row_index * ROWS + row_index;
            product_matrix[product_matrix_index] = 0;
            for (int column_index = 0; column_index < COLUMNS; column_index++)
            {
                product_matrix[product_matrix_index] += matrix1_rows[column_index + COLUMNS * (product_matrix_row_index)] * matrix2[(column_index) * (ROWS) + row_index];
            }
        }
    }
    
    // printf("\nproduct_matrix %d %d", process_rank, product_matrix_length);
    // printPartialMatrix(product_matrix, product_matrix_length);

    // Prepare matrices
    // int resultant_matrix[process_size][product_matrix_length];
    int *resultant_matrix;
    if (ROOT_PROCESS == process_rank)
    {
        // ROWS * ROWS = process_size * product_matrix_length
        if ((resultant_matrix = malloc((process_size * product_matrix_length) * sizeof(int))) == NULL)
        {
            printf("Resultant matrix cannot be created!");

            exit(1);
        }
    }

    // Gather the row sums from the buffer and put it in the final matrix
    MPI_Gather(product_matrix, product_matrix_length, MPI_INT, resultant_matrix, product_matrix_length, MPI_INT, ROOT_PROCESS, MPI_COMM_WORLD);

    // Blocks until all the processes call this method on the MPI_COMM_WORLD communicator 
    MPI_Barrier(MPI_COMM_WORLD);

    if (ROOT_PROCESS == process_rank)
    {
        // Note the ending time.
        float ending_time = MPI_Wtime();
        printf("Product Matrix:\n");
        printMatrix(resultant_matrix, ROWS, ROWS);

        // Expected final product matrix.
        printf("\n\nExpected Matrix:\n");
        multiplyMatrix(matrix1, ROWS, COLUMNS, matrix2, COLUMNS, ROWS);

        printDashedLine(2);
        printf("Ending time: %f", ending_time);
        printDashedLine(2);

        // Time taken.
        float calc_time = ending_time - starting_time;
        printDashedLine(2);
        printf("Took %f", calc_time);
        printDashedLine(2);
    }

    MPI_Finalize();
    if (ROOT_PROCESS == process_rank)
    {
        // Allocated only at the root processes
        free(matrix1);
        free(resultant_matrix);
    }
    free(matrix2);
    free(product_matrix);
    return 0;
}

void multiplyMatrix(int *matrix1, int rows1, int columns1, int *matrix2, int rows2, int columns2)
{
    int *result_matrix;
    if ((result_matrix = malloc(rows1 * columns2 * sizeof(int))) == NULL)
    {
        printf("Resultant matrix cannot be created!");
        exit(1);
    }
    for (int i = 0; i < rows1; i++)
    {
        for (int j = 0; j < columns2; j++)
        {
            result_matrix[i * columns2 + j] = 0;
            for (int k = 0; k < rows2; k++)
            {
                result_matrix[i * columns2 + j] += matrix1[i * rows2 + k] * matrix2[k * columns2 + j];
            }
        }
    }
    printMatrix(result_matrix, rows1, columns2);
}

void generateMatrix(int *matrix, int rows, int columns)
{
    srand(time(NULL));

    for (int index = 0; index < rows * columns; index++)
    {
        matrix[index] = rand() % 100;
    }
}

void printMatrix(int *matrix, int rows, int columns)
{
    printf("\n");
    for (int row = 0; row < rows * columns; row++)
    {
        if (row != 0 && (row % columns) == 0)
        {
            printf("\n");
        }
        printf("%d\t", matrix[row]);
    }
    printf("\n");
}

void printPartialMatrix(int *matrix, int size)
{
    printf("\n");
    for (int row = 0; row < size; row++)
    {
        printf("%d\t", matrix[row]);
    }
    printf("\n");
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