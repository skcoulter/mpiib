#!/bin/bash
#SBATCH --qos=high
#SBATCH --workdir=/usr/projects/hpc3_infrastructure/testjobs/results

pfx=`/usr/bin/hostname | /usr/bin/cut -c1-2`

if [ $pfx == "wf" ] || [ $pfx == "pi" ] || [ $pfx == "ls" ]; then
  source /usr/share/Modules/init/bash;
fi

if [ $pfx == "gr" ] || [ $pfx == "sn" ] || [ $pfx == "ko" ] || [ $pfx == "ba" ]; then
  source /usr/share/lmod/lmod/init/bash;
  source /etc/bashrc;
fi

module load gcc
module load openmpi

mpirun --map-by ppr:1:node /usr/projects/hpc3_infrastructure/testjobs/$1/ibperf_agg ib_$2; 
mpirun --map-by ppr:1:node /usr/projects/hpc3_infrastructure/testjobs/$1/ibperf_rng ib_$2;
mpirun --map-by ppr:1:node /usr/projects/hpc3_infrastructure/testjobs/$1/ibperf_seq ib_$2;

