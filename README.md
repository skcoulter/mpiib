mpiib
=====

This repo contains the code and gazebo-framework launch files to run native IB verbs tests.

Gazebo is a LANL-produced test framework.

  ibperf_agg, ring and seq:

  These tests are launched via MPI so that the tests can be scheduled and run via our automated scheduler.
  This method provides a real world view of what our users see.
  While MPI is used to set up and launch the tests, the results are purely native IB performance WiTHOUT the MPI layer.

mpiring:

This code is a simple MPI synthetic to get bi-directional results that include the MPI layer.

