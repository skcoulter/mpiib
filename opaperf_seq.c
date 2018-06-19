/* ---------------------------------------------------------------
 ** OPA PSM2 Performance testing
 ** FILE: opaperf_seq.c
 ** DESCRIPTION: This program runs client/server PSM2 testing on  
 ** all the nodes in the job allocation sequentially for
 ** uni-directional bw numbers.
 **
 **  AUTHOR: Susan Coulter
 **  
 **  MODIFICATION LOG:
 ** --------------------------------------------------------------- */

#include "mpi.h"
#include <float.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> 
#include <unistd.h>
#define BW_COLUMN		"2"
#define BW_MSG_SIZE		"65536"
#define BW_TITLE                "BW"
#define BW_UNITS                "MB/s"
#define COMM_PATTERN    	"Sequential"
#define DEFAULT_DIR		"/turquoise/usr/projects/systems/testjobs/bin/"
#define DEFAULT_TEST		"bw-mrate"
#define LAT_COLUMN		"2"
#define LAT_MSG_SIZE		"8"
#define LAT_TITLE               "Lat"
#define LAT_UNITS               "usec"
#define MAX_NODES		10000
#define MAX_ARGS		2
#define NODEFILE		"/turquoise/usr/projects/systems/testjobs/nodefiles/nodefile"

#define MPICHK(_ret_)                                                        \
{if (MPI_SUCCESS != (_ret_))                                                 \
{                                                                            \
   fprintf(stderr, "mpi success not returned...\n");                         \
   fflush(stderr);                                                           \
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
  exit(EXIT_FAILURE);

}

int main(argc,argv)
int argc;
char *argv[];
{
   FILE *fpcluster;
   FILE *fpdate;
   FILE *fpfcmd;
   FILE *fphost;
   FILE *fpnode;
   FILE *fptest;

   char cluster[12], cmd[50], column[3], date[30], fcmd[40], fresult[80], host[10], input[10], jobid[10], killcmd[45], logmsg[200], msg_size[8];
   char nodefile[80], outfile[80], result[80], rm[100], rundata[80], testbase[15], test[125], title[4], today[10], units[5];
   int client, gbuf_size, gnum, i, ibx, idx, myrank, numranks, rc, server;
   double *gbuf, *gp;
   double avg, num, tot, EndTime, RunTime, StartTime;

   time_t timet;
   struct tm tm_struct;
 
   MPI_Status status;
   MPI_Request sndrq;

/* get cluster name */

   if ((fpcluster=popen("/usr/projects/hpcsoft/utilities/bin/sys_name","r")) == NULL) {
      strncpy(exitmsg, "failed to get cluster name", sizeof exitmsg);
      bad_exit(exitmsg);
   }
   fgets(cluster, 12, fpcluster);
   cluster[strlen(cluster)-1] = '\0';

/* setup test - assume default   */
/*   if args, set as appropriate */

   strncpy(testbase, DEFAULT_TEST, sizeof testbase);
   strncpy(cmd, DEFAULT_DIR, sizeof cmd);
   strcpy(column, "$");

   if (argc > 1) {
       strncat(cmd, argv[1], sizeof cmd);
       strncpy(testbase, argv[1], sizeof testbase);

      if (strstr(cmd,"bw")) {
        strcat(column, BW_COLUMN);
        strncpy(msg_size, BW_MSG_SIZE, sizeof msg_size);
        strncpy(title, BW_TITLE, sizeof title);
        strncpy(units, BW_UNITS, sizeof units);
      }
      if (strstr(cmd,"lat")) {
        strcat(column, LAT_COLUMN);
        strncpy(msg_size, LAT_MSG_SIZE, sizeof msg_size);
        strncpy(title, LAT_TITLE, sizeof title);
        strncpy(units, LAT_UNITS, sizeof units);
      }

      if (!(!strstr(cmd,"bw-mrate") || !strstr(cmd,"latency"))) {
          strncpy(exitmsg, "Unknown test requested, stopping...", sizeof exitmsg);
          bad_exit(exitmsg);
      }

/* setup test specifics */

      for (idx=2; idx < argc; idx++) {

        if (strlen(argv[idx]) > 7) {
          strncpy(exitmsg, "Message size or number of runs > 7 digits, stopping...", sizeof exitmsg);
          bad_exit(exitmsg);
        }
        if (idx==2) strncpy(msg_size, argv[idx], sizeof msg_size);
        if (idx>MAX_ARGS) {
          strncpy(exitmsg, "Too many parameters, stopping...", sizeof exitmsg);
          bad_exit(exitmsg);
        }
      }
    } else {
      strncat(cmd, DEFAULT_TEST, sizeof cmd);
      strncpy(msg_size, BW_MSG_SIZE, sizeof msg_size);
      strcat(column, BW_COLUMN);
      strncpy(title, BW_TITLE, sizeof title);
      strncpy(units, BW_UNITS, sizeof units);
    }

   MPICHK(MPI_Init(&argc,&argv));
   MPICHK(MPI_Comm_size(MPI_COMM_WORLD,&numranks));
   MPICHK(MPI_Comm_rank(MPI_COMM_WORLD,&myrank));

/* run specific variables */

   MPI_Barrier(MPI_COMM_WORLD);

   sprintf(jobid, "%s", getenv("SLURM_JOBID"));
   sprintf(nodefile, "%s.%s", NODEFILE, getenv("SLURM_JOBID"));

   if ((fpdate=popen("/bin/date +\"%Y-%m-%d %T.%6N\"","r")) == NULL) {
      strncpy(exitmsg, "failed to get date", sizeof exitmsg);
      bad_exit(exitmsg);
   }
   fgets(date, 30, fpdate);
   date[strlen(date)-1] = '\0';

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

    sprintf(rundata, "%s Cluster=%s JobId=%s Test=%s Pattern=%s Ranks=%d MsgSize=%s", date, cluster, jobid, testbase, COMM_PATTERN, numranks, msg_size);

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

/* Run All Combinations Sequentially */
 
   client=ibx=0;
   while (client < numranks) {
      if (myrank != client) {

/* Start servers */

         sprintf(test,"hwloc-bind --cpubind node:1 %s >& /dev/null &", cmd); 
	 test[strlen(test)-1] = '\0';
         system(test);
      }

      system("sleep 1");
      MPI_Barrier(MPI_COMM_WORLD);

      server=0;
      while (server < numranks) {
        if (myrank == client && myrank != server) {
           sprintf(test,"hwloc-bind --cpubind node:1 %s %s | /bin/grep %s | /bin/awk '{print %s}'", cmd, nodes[server], msg_size, column);
           if ((fptest=popen(test,"r")) == NULL) {
              strncpy(exitmsg, "failed to open pipe to run test", sizeof exitmsg);
              bad_exit(exitmsg);
           }
           
           fgets(result, 80, fptest);
           result[strlen(result)-1] = '\0';
           sprintf(logmsg, "logger -t perftest %s FromNode=%s Avg%s=%s %s\n", rundata, nodes[client], title, result, units);
           system(logmsg);
           sprintf(logmsg, "logger -t perftest %s ToNode=%s Avg%s=%s %s\n", rundata, nodes[server], title, result, units);
           system(logmsg);

           num=0;
           num = atof(result);
           rnums[ibx] = num;
           ibx++;
        }
        system("sleep 1");
        MPI_Barrier(MPI_COMM_WORLD);
        server++;
      }
      system(killcmd);
      client++;
      system("sleep 1");
      MPI_Barrier(MPI_COMM_WORLD);
   }

   MPI_Barrier(MPI_COMM_WORLD);
   EndTime = MPI_Wtime();

   RunTime = EndTime - StartTime;

/* allocate buffer on rank 0
 *  * get stats and calc average */

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
      sprintf(logmsg, "logger -t perftest %s FullAvg%s=%#.2f %s\n", rundata, title, avg, units);
      system(logmsg);
      sprintf(logmsg, "logger -t perftest Runtime=%f secs\n", RunTime);
      system(logmsg);
   } 

   sprintf(rm, "/bin/rm -f %s", nodefile);
   system(rm);

   MPI_Finalize();
/*   exit(0); */
}
