mpiib
=====

This repo contains the code and gazebo-framework launch files to run native IB verbs tests.

Gazebo is a LANL-produced test framework found at https://github.com/losalamos/Gazebo

  ibperf_agg, ring and seq:

  These tests are launched via MPI so that the tests can be scheduled and run via our automated scheduler.
  This method provides a real world view of what our users see.
  While MPI is used to set up and launch the tests, the results are purely native IB performance WiTHOUT the MPI layer. 
  
  Each test can take up to 3 parameters, test, message size and number of iterations.
  If no test is specified, the default is ib_read_bw.
  The message size and number of iterations use the defaults specified for the given tests.

mpiring:

This code is a simple MPI synthetic to get bi-directional results that include the MPI layer.

