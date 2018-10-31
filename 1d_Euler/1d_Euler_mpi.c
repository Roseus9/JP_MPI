#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "mpi.h"
int core = 28; // Value changes in accordance to the number of cores used
/*Calculate Euler equation by using Lax method*/
#define IMAX 72072 / core //IMAX = 720720 / number of process
#define NMAX 20000
#define QMAX 3
#define D 6.0
#define Rl 1.9
#define Tl 1.9
#define Rr 1.0
#define Tr 1.0
#define V 0.0

#define F_FILE "eul_mpi_f.txt"
#define L_FILE "eul_mpi_l10.txt"
#define T_FILE "average.txt"

int main(int argc, char *argv[]);
void update(int id, int p);

void calw(double uu[QMAX][IMAX + 2], double ww[QMAX][IMAX + 2]);
void calh(double uu[QMAX][IMAX + 2], double uh[QMAX][IMAX + 2]);

int main(int argc, char *argv[])
{
    int id, p;
	struct timeval start, end;
	double get_time, time_max, time_ave;


    MPI_Init(&argc, &argv); // 並列処理の開始
    MPI_Comm_rank(MPI_COMM_WORLD, &id); // ランク(プロセス番号のようなもの)の取得
    MPI_Comm_size(MPI_COMM_WORLD, &p); // 全ランク数の取得

	gettimeofday(&start, NULL);

    update(id, p);

	gettimeofday(&end, NULL);

	get_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) * 1.0E-6;
	//printf("id: %d time: %lf[sec]\n", id, get_time);

	MPI_Allreduce(&get_time, &time_max, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
	MPI_Allreduce(&get_time, &time_ave, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
	time_ave = time_ave / (double)p;

	FILE *t_file;
	if (id == 0)
	{
		t_file = fopen(T_FILE, "a");
		fprintf(t_file, "%d %lf\n", core, time_ave);
		fclose(t_file);
		printf("\n  time_max: %lf[sec]", time_max);
		printf("\n  time_ave: %lf[sec]\n", time_ave);
	}
    //Terminate MPI
    MPI_Finalize();
    return 0;
}

void update(int id, int p)
/*
Purpose:
    UPDATE computers the solution of the heat equation.
*/
{
	double x[IMAX + 2], uu[QMAX][IMAX + 2], un[QMAX][IMAX + 2];
	double ww[QMAX][IMAX + 2], uh[QMAX][IMAX + 2];
	
    int i, n, q;
	//int IMAX;
	double t_delta, x_delta;
	double x_min = 0.0, x_max = 12.0;

	double cfl;

	MPI_Status status;
	double uu_send_1[3], uu_send_2[3];
	double uu_recv_1[3], uu_recv_2[3];
	int dest, src, tag;

	FILE *f_file, *l_file;
	int ii, nn;
	
	t_delta = 0.00005;
	x_delta = (x_max - x_min) / (double)(p * IMAX - 1);

	//Setup of x
    for(i = 0; i <= IMAX + 1; i++)
    {
        x[i] = ((double)(id * IMAX + i - 1) * x_max
               +(double)(p * IMAX - id * IMAX - i) * x_min)
               /(double)(p * IMAX - 1);
    }

    //Set the values of U at the initial time
	for (i = 1; i <= IMAX; i++)
	{
		if (x[i] <= x_max / 2.0)
		{
			uu[0][i] = Rl;
			uu[1][i] = Rl * V;
			uu[2][i] = (Rl * Tl * D * D) / (2.0 * (D + 2.0)) + 0.5 * Rl * V * V;
		}
		else
		{
			uu[0][i] = Rr;
			uu[1][i] = Rr * V;
			uu[2][i] = (Rr * Tr * D * D) / (2.0 * (D + 2.0)) + 0.5 * Rr * V * V;
		}
	}

    /*
        Check the CFL condition, have processor 0 print out its value,
        and quit if it is too large. 
    */
    cfl = t_delta / x_delta;
    
    if(id == 0)
    {
        printf("\n");
        printf("UPDATE\n");
        printf(" dt = %lf\n", t_delta);
        printf(" dx = %lf\n", x_delta);
        printf(" CFL stability criterion value = dt / dx = %lf\n", cfl);
    }

    if(0.5 <= cfl)
    {
        if(id == 0)
        {
            printf("\n");
            printf("UPDATE - Warning!\n");
            printf(" Computation cancelled!\n");
            printf(" CFL condition failed.\n");
            printf(" 0.5 <= K * dT / dx / dx = %lf\n", cfl);
        }
        return;
    }
	//Open 2 files
    if(id == 0)
    {
        f_file = fopen(F_FILE, "w");
        fclose(f_file);

        l_file = fopen(L_FILE, "w");
        fclose(l_file);
    }
    //Saving the value of U as first file
	calh(uu, uh);
    for(ii = 0; ii < p; ii++)
    {
        MPI_Barrier(MPI_COMM_WORLD);
        if(ii != id)
        {
            continue;
        }
        f_file = fopen(F_FILE, "a");
        for(nn = 1; nn <= IMAX; nn++)
        {
            fprintf(f_file, "%lf %lf %lf %lf\n", x[nn], uh[0][nn], uh[1][nn], uh[2][nn]);
        }
        //fprintf(h_file_f, "\n");
        fclose(f_file);
    }

    //Compute the values of U at the next time, based on current data.
	for(n = 1; n <= NMAX; n++)
    {
		//Send and Recieve data by using MPI communication 

		//original

		dest = (0 < id ? id - 1 : MPI_PROC_NULL);
		src = (id < p - 1 ? id + 1 : MPI_PROC_NULL);

		for (q = 0; q < 3; q++)
		{
			uu_send_1[q] = uu[q][1];
			uu_send_2[q] = uu[q][IMAX];
		}

		tag = 1;
		MPI_Sendrecv(uu_send_1, 3, MPI_DOUBLE, dest, tag,
					 uu_recv_1, 3, MPI_DOUBLE, src, tag, MPI_COMM_WORLD, &status);
		
		dest = (id < p - 1 ? id + 1 : MPI_PROC_NULL);
		src = (0 < id ? id - 1 : MPI_PROC_NULL);

		tag = 2;
		MPI_Sendrecv(uu_send_2, 3, MPI_DOUBLE, dest, tag,
					 uu_recv_2, 3, MPI_DOUBLE, src, tag, MPI_COMM_WORLD, &status);

		for (q = 0; q < 3; q++)
		{
			uu[q][IMAX + 1] = uu_recv_1[q];
			uu[q][0] = uu_recv_2[q];
		}

		//Calculation
		calw(uu, ww);
		for (i = 1; i <= IMAX; i++)
		{
			for (q = 0; q < QMAX; q++)
			{
				un[q][i] = 0.5 * (uu[q][i - 1] + uu[q][i + 1]) - 0.5 * cfl * (ww[q][i + 1] - ww[q][i - 1]);
			}
		}

		//Boundary condition
        if(id == 0)
        {
			for (q = 0; q < QMAX; q++)
			{
				un[q][1] = uu[q][1];
			}
        }
        if(id == p - 1)
        {
			for (q = 0; q < QMAX; q++)
			{
				un[q][IMAX] = uu[q][IMAX];
			}
        }
		//Update value of U
        for(i = 1; i <= IMAX; i++)
        {
			for (q = 0; q < QMAX; q++)
			{
				uu[q][i] = un[q][i];
			}
        }
    }
	calh(uu, uh);

    //Saving the value of U as Second file
    for(ii = 0; ii < p; ii++)
    {
        MPI_Barrier(MPI_COMM_WORLD);
        if(ii != id)
        {
            continue;
        }
        l_file = fopen(L_FILE, "a");
        for(nn = 1; nn <= IMAX; nn++)
        {
			fprintf(l_file, "%lf %lf %lf %lf\n", x[nn], uh[0][nn], uh[1][nn], uh[2][nn]);
        }
        fclose(l_file);
    }
    return;
}

void calw(double uu[QMAX][IMAX + 2], double ww[QMAX][IMAX + 2])

{
	int i;

	for (i = 0; i < IMAX + 2; i++)
	{
		ww[0][i] = uu[1][i];
		ww[1][i] = ((2.0 * uu[2][i]) / D) + ((D - 1.0) * uu[1][i] * uu[1][i]) / (D * uu[0][i]);
		ww[2][i] = ((((D + 2.0) * uu[0][i] * uu[2][i]) - uu[1][i] * uu[1][i]) * uu[1][i]) / (D * uu[0][i] * uu[0][i]);
	}
}

void calh(double uu[QMAX][IMAX + 2], double uh[QMAX][IMAX + 2])
{
	int i;

	for (i = 0; i < IMAX + 2; i++)
	{
		uh[0][i] = uu[0][i];
		uh[1][i] = uu[1][i] / uu[0][i];
		uh[2][i] = ((D + 2.0) * ((2.0 * uu[0][i] * uu[2][i]) - uu[1][i] * uu[1][i])) / (D * D * uu[0][i] * uu[0][i]);
	}
}
