#!/bin/bash

day=`/bin/date +%a`

if [ $day == "Sun" ]; then
  /usr/bin/sbatch --nodes=16 --time=20:00 --error=outerr.%j --output=outerr.%j /usr/projects/hpc3_infrastructure/testjobs/cron_scripts/weekly.sh wf send_bw;
  /usr/bin/sbatch --nodes=64 --time=30:00 --error=outerr.%j --output=outerr.%j /usr/projects/hpc3_infrastructure/testjobs/cron_scripts/weekly.sh wf send_bw;
fi

if [ $day == "Mon" ]; then
  /usr/bin/sbatch --nodes=16 --time=20:00 --error=outerr.%j --output=outerr.%j /usr/projects/hpc3_infrastructure/testjobs/cron_scripts/weekly.sh wf write_bw;
  /usr/bin/sbatch --nodes=64 --time=30:00 --error=outerr.%j --output=outerr.%j /usr/projects/hpc3_infrastructure/testjobs/cron_scripts/weekly.sh wf write_bw;
fi

if [ $day == "Tue" ]; then
  /usr/bin/sbatch --nodes=16 --time=20:00 --error=outerr.%j --output=outerr.%j /usr/projects/hpc3_infrastructure/testjobs/cron_scripts/weekly.sh wf send_lat;
  /usr/bin/sbatch --nodes=64 --time=30:00 --error=outerr.%j --output=outerr.%j /usr/projects/hpc3_infrastructure/testjobs/cron_scripts/weekly.sh wf send_lat;
fi

if [ $day == "Wed" ]; then
  /usr/bin/sbatch --nodes=16 --time=20:00 --error=outerr.%j --output=outerr.%j /usr/projects/hpc3_infrastructure/testjobs/cron_scripts/weekly.sh wf write_lat;
  /usr/bin/sbatch --nodes=64 --time=30:00 --error=outerr.%j --output=outerr.%j /usr/projects/hpc3_infrastructure/testjobs/cron_scripts/weekly.sh wf write_lat;
fi

if [ $day == "Thu" ]; then
  /usr/bin/sbatch --nodes=16 --time=20:00 --error=outerr.%j --output=outerr.%j /usr/projects/hpc3_infrastructure/testjobs/cron_scripts/weekly.sh wf read_bw;
  /usr/bin/sbatch --nodes=64 --time=30:00 --error=outerr.%j --output=outerr.%j /usr/projects/hpc3_infrastructure/testjobs/cron_scripts/weekly.sh wf read_bw;
fi

if [ $day == "Fri" ]; then
  /usr/bin/sbatch --nodes=16 --time=20:00 --error=outerr.%j --output=outerr.%j /usr/projects/hpc3_infrastructure/testjobs/cron_scripts/weekly.sh wf read_lat;
  /usr/bin/sbatch --nodes=64 --time=30:00 --error=outerr.%j --output=outerr.%j /usr/projects/hpc3_infrastructure/testjobs/cron_scripts/weekly.sh wf read_lat;
fi

if [ $day == "Sat" ]; then
  date=`/bin/date +%d`;
  if [ $date -lt 8 ]; then
    /usr/bin/sbatch --nodes=128 --time=03:00:00 --error=outerr.%j --output=outerr.%j /usr/projects/hpc3_infrastructure/testjobs/cron_scripts/weekly.sh wf send_bw;
    /usr/bin/sbatch --nodes=128 --time=03:00:00 --error=outerr.%j --output=outerr.%j /usr/projects/hpc3_infrastructure/testjobs/cron_scripts/weekly.sh wf send_lat;
  fi
  if [ $date -gt 7 ] && [ $date -lt 15 ]; then
    /usr/bin/sbatch --nodes=128 --time=03:00:00 --error=outerr.%j --output=outerr.%j /usr/projects/hpc3_infrastructure/testjobs/cron_scripts/weekly.sh wf write_bw;
    /usr/bin/sbatch --nodes=128 --time=03:00:00  --error=outerr.%j --output=outerr.%j /usr/projects/hpc3_infrastructure/testjobs/cron_scripts/weekly.sh wf write_lat;
  fi
  if [ $date -gt 14 ] && [ $date -lt 22 ]; then
    /usr/bin/sbatch --nodes=128 --time=03:00:00 --error=outerr.%j --output=outerr.%j /usr/projects/hpc3_infrastructure/testjobs/cron_scripts/weekly.sh wf read_bw;
    /usr/bin/sbatch --nodes=128 --time=03:00:00 --error=outerr.%j --output=outerr.%j /usr/projects/hpc3_infrastructure/testjobs/cron_scripts/weekly.sh wf read_lat;
  fi
fi
