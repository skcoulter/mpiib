/* ---------------------------------------------------------------
 ** IB Performance testing
 ** FILE: ibperf_ring.c
 ** DESCRIPTION: This program runs client/server IB testing on  
 ** all the nodes in the job allocation.  Bidirectional
 **
 **  AUTHOR: Susan Coulter
 **  DATE: 2013-02-13
 ** --------------------------------------------------------------- */

#include "mpi.h"
#include <float.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h> 
#define NODEFILE      "/usr/projects/systems/skc/cj_gazebo/test_exec/ibperf_ring/src/nodefile"
#define MAX_NODES     10000

#define MPICHK(_ret_)                                                        \
{if (MPI_SUCCESS != (_ret_))                                                 \
{                                                                            \
   fprintf(stderr, "mpi success not returned...\n");                         \
   fflush(stderr);                                                           \
   printf("<results> FAIL Job failed MPICHK\n");                             \
   exit(EXIT_FAILURE);                                                       \
}}

/* globals */

char exitmsg[128];
typedef char * string;

/* bad end to program */

void bad_exit(char *msg)
{

/* write error message */

  fprintf(stderr, "  Fatal Error: %s\n", msg);
  printf("<results> FAIL %s\n", msg);
  exit(EXIT_FAILURE);

}

int main(argc,argv)
int argc;
char *argv[];
{
   FILE *fphost;
   FILE *fpnode;
   FILE *fptest;

   char cmd[15], host[10], input[10], result[80], test[50];
   int port=18515;
   int dstport, dstrank, idx, myport, myrank, numranks, rc;
   double *gbuf, *gp;
   double avg, num, tot, EndTime, RunTime, StartTime;

   MPI_Status status;
   MPI_Request sndrq;

/* can override default ib test */

   if ( argc == 2 ) {
     strncpy(cmd, argv[2], sizeof cmd);
   } else {
     strncpy(cmd, "ib_read_bw", sizeof cmd);
   }
 
   MPICHK(MPI_Init(&argc,&argv));
   MPICHK(MPI_Comm_size(MPI_COMM_WORLD,&numranks));
   MPICHK(MPI_Comm_rank(MPI_COMM_WORLD,&myrank));

/* run specific variables */

   string nodes[numranks];

/* get and print host info */

  if ((fphost=popen("/bin/hostname | /bin/sed 's/\\..*//'","r")) == NULL) {
     strncpy(exitmsg, "failed to open pipe to get host name", sizeof exitmsg);
     bad_exit(exitmsg);
   } 
   
   fgets(host, 10, fphost);
   host[strlen(host)-1] = '\0';
  
   idx=0;
   while (idx <= numranks) {
         MPI_Barrier(MPI_COMM_WORLD);
      if (idx == 0 && myrank == 0) {
         if ((fpnode=fopen(NODEFILE,"w"))==NULL) {
           strncpy(exitmsg, "Failure opening NODEFILE for WRITE", sizeof exitmsg);
           bad_exit(exitmsg);
         }
         fprintf(fpnode,"%s\n",host); 
         fflush(fpnode);
         fclose(fpnode);
         MPI_Barrier(MPI_COMM_WORLD);
      } else if (myrank == idx) {
         MPI_Barrier(MPI_COMM_WORLD);
         if ((fpnode=fopen(NODEFILE,"a"))==NULL) {
           strncpy(exitmsg, "Failure opening NODEFILE for WRITE", sizeof exitmsg);
           bad_exit(exitmsg);
         }
         fprintf(fpnode,"%s\n",host); 
         fflush(fpnode);
         fclose(fpnode);
      }
      idx++;
   }

/* Start */

   if (myrank == 0)
      printf("Running %s test on %d ranks\n", cmd, numranks);

/* everybody reads the node file */

   if ((fpnode=fopen(NODEFILE,"r"))==NULL) {
      strncpy(exitmsg, "Failure opening nodefile for READ", sizeof exitmsg);
      bad_exit(exitmsg);
   }

   idx=0;
   while (fgets(input, sizeof(input), fpnode) != NULL) {
      input[strlen(input)-1] = '\0';
      if (idx < MAX_NODES) {
         nodes[idx] = (char*)malloc(sizeof(char) * (strlen(input)+1));
         strncpy(nodes[idx],input,(strlen(input)+1));
         idx++;
      } else {
        strncpy(exitmsg, "node array overflow", sizeof exitmsg);
        bad_exit(exitmsg);
      }
   }

   MPI_Barrier(MPI_COMM_WORLD);
   StartTime = MPI_Wtime();

/* Ring */

   dstrank = (myrank + 1) % numranks;
   myport = myrank + port;
   dstport = dstrank + port;

/* launch my server */

   sprintf(test,"%s -p %i >& /dev/null &", cmd, myport);
   system(test);
   system("sleep 1");

   MPI_Barrier(MPI_COMM_WORLD);

/* run my client */

   sprintf(test,"%s -p %i %s | /bin/grep 65536 | /bin/sed 's/65536.* //'", cmd, dstport, nodes[dstrank]);
   if ((fptest=popen(test,"r")) == NULL) {
      strncpy(exitmsg, "failed to open pipe to run test", sizeof exitmsg);
      bad_exit(exitmsg);
   }

   fgets(result, 80, fptest);
   result[strlen(result)-1] = '\0';
   printf("<td> From_%s_to_%s %s\n",nodes[myrank], nodes[dstrank], result);
   num=0;
   num = atof(result);

   MPI_Barrier(MPI_COMM_WORLD);
   EndTime = MPI_Wtime();

   RunTime = EndTime - StartTime;

/* allocate buffer on rank 0
 *  *  * get stats and calc average */

   if (myrank ==0)
      gbuf = (double *)malloc(numranks*sizeof(double));

   MPICHK(MPI_Gather(&num, 1, MPI_DOUBLE, gbuf, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD));

   if (myrank == 0) {
      idx=0;
      gp = gbuf;
      while (idx < numranks) {
         tot = tot + *gp;
         idx++;
         gp++;
      }
      avg = tot / idx;
      printf("<td> RingAvg_%iRanks %#.2f\n", numranks, avg);
      printf ("Ring run time %f\n", RunTime);
      printf("<results> PASS Job concluded successfully\n");
   }

   MPI_Finalize();
   exit(0);
}
