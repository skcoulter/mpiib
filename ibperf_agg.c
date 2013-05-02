/* ---------------------------------------------------------------
 ** IB Performance testing
 ** FILE: ib_bw.c
 ** DESCRIPTION: This program runs client/server IB testing on  
 ** all the nodes in the job allocation.  Aggregate
 **
 **  AUTHOR: Susan Coulter
 **  DATE: 2013-03-07
 ** --------------------------------------------------------------- */

#include "mpi.h"
#include <float.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h> 
#define NODEFILE      "/usr/projects/systems/skc/cj_gazebo/test_exec/ibperf_agg/src/nodefile"
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
   int dfltport=18515;
   int client, clnport, gbuf_size, gnum, ibx, idx, myrank, numranks, rc, server, svrport;
   double *gbuf, *gp;
   double aggtot, avg, num, tot, EndTime, RunTime, StartTime;

   MPI_Status status;
   MPI_Request sndrq;

/* can override default ib test */

   if ( argc == 2 ) {
     strncpy(cmd, argv[1], sizeof cmd);
   } else {
     strncpy(cmd, "ib_read_bw", sizeof cmd);
   }
 
   MPICHK(MPI_Init(&argc,&argv));
   MPICHK(MPI_Comm_size(MPI_COMM_WORLD,&numranks));
   MPICHK(MPI_Comm_rank(MPI_COMM_WORLD,&myrank));

/* run specific variables */

   gnum=0;
   gnum = numranks - 1;
   gbuf_size = numranks * gnum;
   double rnums[gnum];
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

/* Run All To One Aggregate */
 
/* launch simultaneous servers */

   server=ibx=0;
   while (server < numranks) {
      if (myrank == server) {
         idx=0;
         while (idx < numranks) {
            if (myrank != idx) {
               svrport = dfltport + idx;
               sprintf(test,"%s -p %i >& /dev/null &", cmd, svrport);
               system(test);
            }
            idx++;
         }
      }
      system("sleep 1");
      MPI_Barrier(MPI_COMM_WORLD);

/* clients should run close to simultaneously */

      client=0;
      while (client < numranks) {
        if (myrank == client && myrank != server) {
           clnport = dfltport + client;
           sprintf(test,"%s -p %i %s | /bin/grep 65536 | /bin/sed 's/[ \t]*$//' | /bin/sed 's/65536.* //'", cmd, clnport, nodes[server]);
           if ((fptest=popen(test,"r")) == NULL) {
              strncpy(exitmsg, "failed to open pipe to run test", sizeof exitmsg);
              bad_exit(exitmsg);
           }
           
           fgets(result, 80, fptest);
           result[strlen(result)-1] = '\0';
          
           printf("<td> %s_From_%s_to_%s_%i_ranks %s\n", cmd, nodes[client], nodes[server], numranks, result);
           num=0;
           num = atof(result);
           rnums[ibx] = num;  
           clnport++;
           ibx++;
        }
        client++;
      }
      system("sleep 1");
      server++;
      MPI_Barrier(MPI_COMM_WORLD);
   }

   MPI_Barrier(MPI_COMM_WORLD);
   EndTime = MPI_Wtime();

   RunTime = EndTime - StartTime;

/* allocate buffer on rank 0
 * get stats and calc average */

   if (myrank ==0) 
      gbuf = (double *)malloc(numranks*gnum*sizeof(double));

   MPICHK(MPI_Gather(rnums, gnum, MPI_DOUBLE, gbuf, gnum, MPI_DOUBLE, 0, MPI_COMM_WORLD));

   if (myrank == 0) {
      idx=0;
      gp = gbuf;
      while (idx < gbuf_size) {
         tot = tot + *gp;
         idx++;
         gp++;
      }
      avg = tot / idx;
      aggtot = avg * gnum;
      printf("<td> %s_AggAvgInd_%iRanks %#.2f\n", cmd, numranks, avg);
      printf("<td> %s_AggAvgTot_%iRanks %#.2f\n", cmd, numranks, aggtot);
      printf("Aggregate run time %f\n", RunTime);
      printf("<results> PASS Job concluded successfully\n");
   } 

   MPI_Finalize();
   exit(0);
}
