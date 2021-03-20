#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(argc, argv) int argc;
char *argv[];
{
    int world_rank, world_size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int PING_PONG_LIMIT = 100;
    int ping_pong_count = 0;
    int partner_rank = (world_rank + 1) % 2;

    // We are assuming 2 processes for this task
    if (world_size != 2)
    {
        fprintf(stderr, "World size must be two for %s\n", argv[0]);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    while (ping_pong_count < PING_PONG_LIMIT)
    {
        if (world_rank == ping_pong_count % 2)
        {
            // Increment the ping pong count before you send it
            ping_pong_count++;
            MPI_Send(&ping_pong_count, 1, MPI_INT, partner_rank, 0,
                     MPI_COMM_WORLD);
            printf("%d sent and incremented ping_pong_count "
                   "%d to %d\n",
                   world_rank, ping_pong_count,
                   partner_rank);
        }
        else
        {
            MPI_Recv(&ping_pong_count, 1, MPI_INT, partner_rank, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("%d received ping_pong_count %d from %d\n",
                   world_rank, ping_pong_count, partner_rank);
        }
    }
    MPI_Finalize();
}
