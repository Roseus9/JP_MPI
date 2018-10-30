#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define L_FILE "pingpongplot.txt"

int main(int argc, char** argv) {

	double start_time, end_time;

	//MPI_Status stat;

	const int PING_PONG_LIMIT = 10000; // no. of bounce

	/*int value = 0;
	printf("Enter byte size: ");
	fflush(stdout);
	scanf("%d", &value);
	*/
	char byte[500000] = "byte";

	// Initialize the MPI environment
	MPI_Init(NULL, NULL);

	// Find out rank, size
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	FILE *l_file;

	// Obtain processor_name that is running the core
	char processor_name[MPI_MAX_PROCESSOR_NAME];
  	int name_len;
  	MPI_Get_processor_name(processor_name, &name_len);

	// Print processor_name base on world_rank
	int i;
	for(i = 0; i < 2; i++)
	{
		printf("%d is process name  %s\n", world_rank, processor_name);
	}

	// Assume that at least 2 processes needed for this task
	if (world_size != 2)
	{
		fprintf(stderr, "World size must be two for %s\n", argv[0]);
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	int ping_pong_count = 0;
	int partner_rank = (world_rank + 1) % 2;
	
	// Initialize start time
	start_time = MPI_Wtime();

	while (ping_pong_count < PING_PONG_LIMIT)
	{
		if(world_rank == ping_pong_count % 2) // Determines who receive and sends, good line of code
		{
			// Increment the ping pong count before sending
			ping_pong_count++;
			MPI_Send(&byte, 500000, MPI_BYTE, partner_rank, 0, MPI_COMM_WORLD);
			MPI_Send(&ping_pong_count, 1, MPI_INT, partner_rank, 0, MPI_COMM_WORLD);
			MPI_Barrier(MPI_COMM_WORLD);
			printf("%s sent %lu bytes to %d. ping_pong_count = %d\n", processor_name, sizeof(byte), partner_rank, ping_pong_count);
			//MPI_Send(&ping_pong_count, 1, MPI_INT, partner_rank, 0, MPI_COMM_WORLD);
		}
		else
		{
			MPI_Recv(&byte, 500000, MPI_BYTE, partner_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			MPI_Recv(&ping_pong_count, 1, MPI_INT, partner_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			MPI_Barrier(MPI_COMM_WORLD);
			//MPI_Recv(&ping_pong_count, 1, MPI_INT, partner_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			printf("%s received %lu bytes from %d\n", processor_name, sizeof(byte), partner_rank);
		}
	}
    
	// Initialize the end time
	end_time = MPI_Wtime();

	MPI_Finalize();

	// Calculate time taken
	if (world_rank == 0)
	{
		printf("Runtime = %f s\n", end_time-start_time);
		l_file = fopen(L_FILE, "a");
		fprintf(l_file, "%lu %f\n", sizeof(byte), end_time-start_time);
		fclose(l_file);
	}
}

/*
This program is meant to be executed with only two processes. The processes first determine their partner with some simple arithmetic. 
A ping_pong_count is initiated to zero and it is incremented at each ping pong step by the sending process. Data byte size with increment 
of 50000 is sent. As the ping_pong_count is incremented, the processes take turns being the sender and receiver. Finally, after the limit 
is reached (10000), the processes stop sending and receiving. 
*/

/*
Compile for 1 server:
mpicc ping_pong.c -o ping_pong
mpirun -np 2 ./ping_pong

Compile for 2 server:
Edit machinefile
mpicc ping_pong.c -o ping_pong
scp -r <file> user@<IP Address>:<Directory>
mpirun -np 2 -machinefile machinefile ./ping_pong