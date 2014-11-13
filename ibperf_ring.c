/* ---------------------------------------------------------------
 ** IB Performance testing
 ** FILE: ibperf_ring.c
 ** DESCRIPTION: This program runs client/server IB testing on  
 ** all the nodes in the job allocation using a ring algorithm
 ** thereby getting bi-directional bw numbers.
 **
 **  AUTHOR: Susan Coulter
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
   int port=18515;
   int dstport, dstrank, i, idx, myport, myrank, numranks, rc;
   double *gbuf, *gp;
   double avg, num, tot, EndTime, RunTime, StartTime;

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

   sprintf(test,"%s -p %i %s | /bin/grep %s | /bin/grep %s | /bin/grep -v QPN | /bin/awk '{print %s}'", cmd, dstport, nodes[dstrank], msg_size, num_runs, column);

   if ((fptest=popen(test,"r")) == NULL) {
      strncpy(exitmsg, "failed to open pipe to run test", sizeof exitmsg);
      bad_exit(exitmsg);
   }

   cmd[strcspn(cmd," ")] = '\0';
   fgets(result, 80, fptest);
   result[strlen(result)-1] = '\0';
   printf("<td> %s_%s_%s_From_%s_to_%s %s\n", cmd, num_runs, msg_size, nodes[myrank], nodes[dstrank], result);
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
      printf("<td> %s_%s_%s_RingAvg_%iRanks %#.2f\n", cmd, num_runs, msg_size, numranks, avg);
      printf ("Ring run time %f\n", RunTime);
      printf("<results> PASS Job concluded successfully\n");
   }

   sprintf(rm, "/bin/rm -f %s", nodefile);
   system(rm);

   MPI_Finalize();
/*   exit(0); */
}
