#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <time.h>
#include <limits.h>
// We will use the row-major order to store multidimensional arrays in linear storage such as random access memory.
// This also helps to scatter the elements of the array and process them in more easy way.
// Reference: https://en.wikipedia.org/wiki/Row-_and_column-major_order

const int buffer_size = 2;
const int root_process = 0;

void generateMatrix(int *matrix, int rows, int columns);
void printMatrix(int *matrix, int rows, int columns);
void multiplyMatrix(int rows1, int columns1, int matrix1[rows1][columns1],
                    int rows2, int columns2, int matrix2[rows2][columns2],
                    int mul[rows1][columns2]);
int multiply(int a, int b);
int *inverseColumnToRow(int *matrix, int rows, int columns);
// void inverseColumnToRow(int rows, int columns, int inv_column, int matrix[rows][columns], int inverse_row_matrix[rows]);
void printDashedLine(int times);
void print2DMatrix(int rows, int columns, int matrix[rows][columns]);
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
            fprintf(stderr, "Usage: please enter process size such that %d %% number_of_process == 0\nOR\nEnter the different number rows!\n", ROWS);
        }
        MPI_Finalize();
        exit(1);
    }

    if (process_size < 2)
    {
        fprintf(stderr, "World size must be two for %s\n", argv[0]);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    float starting_time = MPI_Wtime();
    int LENGTH_OF_METRIX = ROWS * COLUMNS;

    int *matrix1;

    // As second matrix must be possessed by every process, memory allocation should be done by every process.
    int *matrix2;
    if ((matrix2 = malloc(LENGTH_OF_METRIX * sizeof(int))) == NULL)
    {
        printf("Second matrix cannot be created!");
        exit(1);
    }
    // int *inverse_matrix2;

    if (process_rank == root_process)
    {
        float starting_time = MPI_Wtime();
        printDashedLine(2);
        printf("Starting time: %f", starting_time);
        printDashedLine(2);

        if ((matrix1 = (int *)malloc(LENGTH_OF_METRIX * sizeof(int))) == NULL)
        {
            printf("First matrix cannot be created!");
            exit(1);
        }
        generateMatrix(matrix1, ROWS, COLUMNS);
        generateMatrix(matrix2, COLUMNS, ROWS);

        printMatrix(matrix1, ROWS, COLUMNS);
        printMatrix(matrix2, COLUMNS, ROWS);
    }

    int send_count = LENGTH_OF_METRIX / process_size;

    int matrix1_rows[send_count];
    // if ((matrix1_rows = malloc(send_count * sizeof(int))) == NULL)
    // {
    //     printf("Receiving buffer for matrix 1 cannot be created!");
    //     exit(1);
    // }

    MPI_Scatter(matrix1, send_count, MPI_INT, matrix1_rows, send_count, MPI_INT, root_process, MPI_COMM_WORLD);
    MPI_Bcast(matrix2, LENGTH_OF_METRIX, MPI_INT, root_process, MPI_COMM_WORLD);

    printf("\nmatrix 1\n");
    printPartialMatrix(matrix1_rows, send_count);
    printf("\nmatrix 2\n");
    printPartialMatrix(matrix2, LENGTH_OF_METRIX);

    // Columns of matrix 2
    // Resultant product matrix will be the size of this columns of matrix 2.
    // Because ROWS % number_of_process = 0
    int product_matrix_work_length = LENGTH_OF_METRIX / send_count;
    printf("send_count %d\n", send_count);
    printf("matrix2_columns %d\n", product_matrix_work_length);
    int *product_matrix;
    if ((product_matrix = malloc((product_matrix_work_length * ROWS) * sizeof(int))) == NULL)
    {
        printf("Resultant matrix cannot be created!");
        exit(1);
    }

    int produc_matrix_index;
    for (int product_matrix_work_index = 0; product_matrix_work_index < product_matrix_work_length; product_matrix_work_index++)
    {
        for (int column_index = 0; column_index < ROWS; column_index++)
        {
            produc_matrix_index = product_matrix_work_index * ROWS + column_index;
            product_matrix[produc_matrix_index] = 0;
            for (int row_index = 0; row_index < ROWS; row_index++)
            {
                // printf("%d::: %d -> ++ %d * %d\n", process_rank, produc_matrix_index, row_index, ((row_index) * (ROWS) + column_index));
                // printf("%d::: %d += %d * %d\n\n", process_rank, product_matrix[produc_matrix_index], matrix1_rows[row_index], matrix2[(row_index) * (ROWS) + column_index]);
                product_matrix[produc_matrix_index] += matrix1_rows[row_index + ROWS * (product_matrix_work_index)] * matrix2[(row_index) * (ROWS) + column_index];
            }
            printf("%d::: %d -> %d\n", process_rank, produc_matrix_index, product_matrix[produc_matrix_index]);
        }
    }
    printf("\nproduct_matrix %d %d: %d", process_rank, produc_matrix_index, product_matrix_work_length * ROWS);
    printPartialMatrix(product_matrix, product_matrix_work_length * ROWS);

    // Prepare matrices
    // int resultant_matrix[process_size][ROWS * (LENGTH_OF_METRIX / send_count)];
    int *resultant_matrix;
    if (root_process == process_rank)
    {
        if ((resultant_matrix = malloc((LENGTH_OF_METRIX) * sizeof(int))) == NULL)
        {
            printf("Resultant matrix cannot be created!");
            exit(1);
        }
    }

    // Gather the row sums from the buffer and put it in matrix C
    MPI_Gather(product_matrix, ROWS * (LENGTH_OF_METRIX / send_count), MPI_INT, resultant_matrix, ROWS * (LENGTH_OF_METRIX / send_count), MPI_INT, root_process, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);

    if (root_process == process_rank)
    {
        printf("resultant_matrix");
        printPartialMatrix(resultant_matrix, ROWS * ROWS);
        // print2DMatrix(process_size, ROWS * (LENGTH_OF_METRIX / send_count), resultant_matrix);
        float ending_time = MPI_Wtime();
        printDashedLine(2);
        printf("Ending time: %f", ending_time);
        printDashedLine(2);

        float calc_time = ending_time - starting_time;
        printDashedLine(2);
        printf("Took %f", calc_time);
        printDashedLine(2);
    }

    // printf("\n\nExpected\n");
    // multiplyMatrix(ROWS, COLUMNS, matrix1, COLUMNS, ROWS, matrix2, mul);
    // printMatrix(ROWS, ROWS, mul);

    MPI_Finalize();
    if (root_process == process_rank)
        free(matrix1);
    free(matrix2);
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
    for (int column_index = 0; column_index < columns; column_index++)
    {
        for (int row_index = 0; row_index < rows; row_index++)
        {
            matrix_part[new_index] = matrix[(column_index + row_index) * columns];
            new_index++;
        }
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

void print2DMatrix(int rows, int columns, int matrix[rows][columns])
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