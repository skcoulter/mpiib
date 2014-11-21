/* ---------------------------------------------------------------
 ** IB Performance testing
 ** FILE: ibperf_agg.c
 ** DESCRIPTION: This program runs client/server IB testing on  
 ** all the nodes in the job allocation in an aggregate manner.
 ** If N nodes in the allocation, N-1 nodes communicate with the 
 ** same single node; for each node in the allocation.
 **
 **  AUTHOR: Susan Coulter
 **  Copyright (C) 2014  Susan Coulter
 **
 ** Unless otherwise indicated, this information has been authored by an employee
 ** or employees of the Los Alamos National Security, LLC (LANS), operator of the
 ** Los Alamos National Laboratory under Contract No.  DE-AC52-06NA25396 with the
 ** U.S. Department of Energy.  The U.S. Government has rights to use, reproduce,
 ** and distribute this information. The public may copy and use this information
 ** without charge, provided that this Notice and any statement of authorship are
 ** reproduced on all copies. Neither the Government nor LANS makes any warranty,
 ** express or implied, or assumes any liability or responsibility for the use of
 ** this information.
 ** 
 ** This program has been approved for release from LANS by LA-CC Number 10-066,
 ** being part of the HPC Operational Suite.
 ** 
 ** --------------------------------------------------------------- */

#include "mpi.h"
#include <float.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h> 
#include <sys/time.h> 
#define BW_COLUMN	"4"
#define BW_MSG_SIZE	"65536"
#define DEFAULT_TEST	"ib_read_bw"
#define LAT_COLUMN	"5"
#define LAT_MSG_SIZE	"8"
#define MAX_NODES	10000
#define MAX_ARGS	3
#define NODEFILE	"/yellow/users/markus/mu_gazebo/test_exec/ibperf_ring/src/nodefile"
#define READ_NUM_RUNS	"1000"
#define WRITE_NUM_RUNS	"5000"

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

   char cmd[40], column[2], host[10], input[10], msg_size[8], num_runs[8], nodefile[80], result[80], rm[100], test[50];
   double *gbuf, *gp;
   double avgtot, avg, num, rfrom, EndTime, RunTime, StartTime;
   int dfltport=18515;
   int client, clnport, eidx, i, ibx, idx, ridx;
   int myrank, numranks, rc, server, svrport;

   struct timeval tv;

   MPI_Status status;
   MPI_Request sndrq;

/* setup test - assume default   */
/*   if args, set as appropriate */

   strncpy(cmd, DEFAULT_TEST, sizeof cmd);
   strcpy(column, "$");
   if (argc > 1) {
      strncpy(cmd, argv[1], sizeof cmd);
      if (strstr(cmd,"ib_read"))
        strncpy(num_runs, READ_NUM_RUNS, sizeof num_runs);
      if (strstr(cmd,"ib_write"))
        strncpy(num_runs, WRITE_NUM_RUNS, sizeof num_runs);

      if (strstr(cmd,"bw")) {
        strcat(column, BW_COLUMN);
        strncpy(msg_size, BW_MSG_SIZE, sizeof msg_size);
      }
      if (strstr(cmd,"lat")) {
        strcat(column, LAT_COLUMN);
        strncpy(msg_size, LAT_MSG_SIZE, sizeof msg_size);
      }

      if (!(!strcmp(cmd,"ib_read_bw") || !strcmp(cmd,"ib_write_bw") || !strcmp(cmd,"ib_read_lat") || !strcmp(cmd,"ib_write_lat"))) {
          strncpy(exitmsg, "Unknown test requested, stopping...", sizeof exitmsg);
          bad_exit(exitmsg);
      }

      for (idx=2; idx < argc; idx++) {
        if (strlen(argv[idx]) > 7) {
          strncpy(exitmsg, "Message size or number of runs > 7 digits, stopping...", sizeof exitmsg);
          bad_exit(exitmsg);
        }
        if (idx==2) strncpy(msg_size, argv[idx], sizeof msg_size);
        if (idx==3) strncpy(num_runs, argv[idx], sizeof num_runs);
        if (idx>MAX_ARGS) {
          strncpy(exitmsg, "Too many parameters, stopping...", sizeof exitmsg);
          bad_exit(exitmsg);
        }
      }
    } else {
      strncpy(msg_size, BW_MSG_SIZE, sizeof msg_size);
      strncpy(num_runs, READ_NUM_RUNS, sizeof num_runs);
      strcat(column, BW_COLUMN);
    }

/* build full test command */

   strcat(cmd, " -s ");
   strncat(cmd, msg_size, sizeof msg_size);
   strcat(cmd, " -n ");
   strncat(cmd, num_runs, sizeof num_runs);

   MPICHK(MPI_Init(&argc,&argv));
   MPICHK(MPI_Comm_size(MPI_COMM_WORLD,&numranks));
   MPICHK(MPI_Comm_rank(MPI_COMM_WORLD,&myrank));

/* run specific variables */
   
   sprintf(nodefile, "%s.%s", NODEFILE, getenv("SLURM_JOBID"));

   double rto[numranks],tot[numranks];
   string nodes[numranks];

/* initialize arrays and counters to 0 */

   avgtot = 0;
   rfrom = 0;
   idx=0;
   while (idx < numranks) {
      rto[idx] = tot[idx] = 0;
      idx++;
   }

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
         if ((fpnode=fopen(nodefile,"w"))==NULL) {
           strncpy(exitmsg, "Failure opening NODEFILE for WRITE", sizeof exitmsg);
           bad_exit(exitmsg);
         }
         fprintf(fpnode,"%s\n",host); 
         fflush(fpnode);
         fclose(fpnode);
         MPI_Barrier(MPI_COMM_WORLD);
      } else if (myrank == idx) {
         MPI_Barrier(MPI_COMM_WORLD);
         if ((fpnode=fopen(nodefile,"a"))==NULL) {
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

   if ((fpnode=fopen(nodefile,"r"))==NULL) {
      strncpy(exitmsg, "Failure opening NODEFILE for READ", sizeof exitmsg);
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

   fclose(fpnode);

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
           sprintf(test,"%s -p %i %s | /bin/grep %s | /bin/grep %s | /bin/grep -v QPN | /bin/awk '{print %s}'", cmd, clnport, nodes[server], msg_size, num_runs, column);
           if ((fptest=popen(test,"r")) == NULL) {
              strncpy(exitmsg, "failed to open pipe to run test", sizeof exitmsg);
              bad_exit(exitmsg);
           }
           
           fgets(result, 80, fptest);
           result[strlen(result)-1] = '\0';
           num=0;
           num = atof(result);
           rfrom = rfrom + num;  
           rto[server] = rto[server] + num;  
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

   rfrom = rfrom / (numranks-1);

/* DEBUG

   printf("\nRank %i reporting: Avgerage of %#.2f\n\n", myrank, rfrom);
   idx=0;
   while (idx < numranks) {
      printf("Rank %i reporting rto[%i] = %#.2f\n", myrank, idx, rto[idx]);
      idx++;
   }
*/

/* get and print client data */

   if (myrank == 0) 
      gbuf = (double *)malloc(numranks * sizeof(double));

   MPICHK(MPI_Gather(&rfrom, 1, MPI_DOUBLE, gbuf, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD));
 
   cmd[strcspn(cmd," ")] = '\0';
   if (myrank == 0) {
      idx=0;
      gp = gbuf;
      while (idx < numranks) {
         printf("<td> %s_%s_%s_AggAvgFrom_%s_%iRanks %#.2f\n", cmd, num_runs, msg_size, nodes[idx], numranks, *gp); 
         avgtot = avgtot + *gp;
         idx++;
         gp++;
      }
   }

/* get and print server data */

   if (myrank ==0) 
      gbuf = (double *)malloc((numranks * numranks) * sizeof(double));

   MPICHK(MPI_Gather(rto, numranks, MPI_DOUBLE, gbuf, numranks, MPI_DOUBLE, 0, MPI_COMM_WORLD));

   if (myrank == 0) {
      idx=eidx=ridx=0;
      gp = gbuf;
      while (idx < numranks*numranks) {
         if (eidx == (numranks)) {
            eidx=0;
            ridx++;
         }
         if (eidx != ridx) {
           tot[eidx] = tot[eidx] + *gp;
         } 
         avgtot = avgtot + *gp;
         eidx++;
         idx++;
         gp++;
      }

      idx=0;
      while (idx < numranks) {
         printf("<td> %s_%s_%s_AggAvgTo_%s_%iRanks %#.2f\n", cmd, num_runs, msg_size, nodes[idx], numranks, tot[idx]/(numranks-1));
         idx++;
      }

      avg = avgtot / ((numranks*(numranks-1)) + numranks);
      printf("<td> %s_%s_%s_AggAvgInd_%iRanks %#.2f\n", cmd, num_runs, msg_size, numranks, avg);
      printf("<td> %s_%s_%s_AggAvgTot_%iRanks %#.2f\n", cmd, num_runs, msg_size, numranks, avg * (numranks-1));
      printf("Aggregate run time %f\n", RunTime);
      printf("<results> PASS Job concluded successfully\n");

      sprintf(rm, "/bin/rm -f %s", nodefile);
      system(rm);
   } 
 
   MPI_Finalize();
/*   exit(0); */
}
