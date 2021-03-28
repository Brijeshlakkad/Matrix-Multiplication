# matrix-async.c Usage
mpicc matrix-async.c -o matrix
mpirun -np [NUMBER_OF_PROESSES] matrix [NUMBER_OF_ROWS] [NUMBER_OF_COLUMNS]

### Example:
> mpirun -np 4 matrix 16 32
- The above command will run the MPI program with 4 numbers of processes, 16 numbers of rows for Matrix1 (16 numbers columns for Matrix2), and 32 numbers of rows for Matrix1 (32 numbers columns for Matrix2).

# matrix-sync.c Usage
mpicc matrix-sync.c -o matrix
mpirun -np [NUMBER_OF_PROESSES] matrix

### Example:
> mpirun -np 4 matrix
- The above command will run the MPI program with 4 numbers of processes.