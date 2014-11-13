/* ---------------------------------------------------------------
 ** MPI - ring communication benchmark.
 ** FILE: mpiring.c
 ** DESCRIPTION: this program does a ring communication within  
 ** barriers on a variety of message sizes.
 **
 **  AUTHOR: Susan Coulter
 ** --------------------------------------------------------------- */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <float.h> 
#define NUMBER_REPS     50

#define MPICHK(_ret_)                                                        \
{if (MPI_SUCCESS != (_ret_))                                                 \
{                                                                            \
   fprintf(stderr, "mpi success not returned...\n");                         \
   fflush(stderr);                                                           \
   printf("<results> FAIL Job failed MPICHK\n");                             \
   exit(EXIT_FAILURE);                                                       \
}}


int main(argc,argv)
int argc;
char *argv[];
{
   int init_size=512;
   int num_sizes=10;

   if ( argc == 3 ) {
     init_size=atoi(argv[1]);
     num_sizes=atoi(argv[2]);
   }

   char *srmsg;
   int dstrank, msize, myrank, numranks, rc, repnum, srcrank, sndflg;
   int message_size;
   int tag=0;
   double StartTime, EndTime;
   double round_trips[num_sizes][NUMBER_REPS];
   double avg_round_trip[num_sizes];
   double min_round_trip[num_sizes];
   double max_round_trip[num_sizes];
   MPI_Status status;
   MPI_Request sndrq;

   MPICHK(MPI_Init(&argc,&argv));
   MPICHK(MPI_Comm_size(MPI_COMM_WORLD,&numranks));
   MPICHK(MPI_Comm_rank(MPI_COMM_WORLD,&myrank));

   if (myrank == 0)
      printf("Running %s test on %d ranks\n", argv[0], numranks);

   dstrank = (myrank + 1) % numranks;
   srcrank = (myrank + numranks - 1) % numranks;

   message_size=init_size;
   for(msize=0;msize<num_sizes;msize++)
   {
      srmsg = (char *)malloc(message_size);
      for (repnum=0;repnum<NUMBER_REPS;repnum++)
      {
         MPI_Barrier(MPI_COMM_WORLD);
         StartTime = MPI_Wtime();

	 MPI_Isend(srmsg,message_size,MPI_CHAR,dstrank,tag,MPI_COMM_WORLD,&sndrq);

	 MPI_Recv(srmsg,message_size,MPI_CHAR,srcrank,tag,MPI_COMM_WORLD,&status);

         while (1) {
           MPI_Test(&sndrq,&sndflg,&status);
           if (sndflg) { break; }
         }

         MPI_Barrier(MPI_COMM_WORLD);
         EndTime = MPI_Wtime();

         round_trips[msize][repnum] = EndTime - StartTime;

      }
      free(srmsg);
      message_size*=2;
      sleep(30);
   }

   if (myrank == 0)
   {
      message_size=init_size;
      for(msize=0;msize<num_sizes;msize++)
      {
         avg_round_trip[msize] = 0;
         min_round_trip[msize] = DBL_MAX;
         max_round_trip[msize] = 0;

         for (repnum=0;repnum<NUMBER_REPS;repnum++)
         {
            avg_round_trip[msize]+= round_trips[msize][repnum];
            if (round_trips[msize][repnum] < min_round_trip[msize])
               min_round_trip[msize] = round_trips[msize][repnum];
            if (round_trips[msize][repnum] > max_round_trip[msize])
               max_round_trip[msize] = round_trips[msize][repnum];
         }
         avg_round_trip[msize]/= ((double) NUMBER_REPS);
         printf("<td> Avg_%dSize_%dRanks %.2f Mbytes/sec\n",
                 message_size,
		 numranks,
                 1e6 * avg_round_trip[msize],
                 1e-6 * ((float) (message_size)) / avg_round_trip[msize]);
         printf("<td> Min_%dSize_%dRanks %.2f Mbytes/sec\n",
                 message_size,
		 numranks,
                 1e6 * min_round_trip[msize],
                 1e-6 * ((float) (message_size)) / min_round_trip[msize]);
         printf("<td> Max_%dSize_%dRanks %.2f Mbytes/sec\n",
                 message_size,
		 numranks,
                 1e6 * max_round_trip[msize],
                 1e-6 * ((float) (message_size)) / max_round_trip[msize]);
         message_size*=2;
      }

      printf("<results> PASS Job concluded successfully\n");

   }

   MPI_Finalize();
   exit(0);
}
