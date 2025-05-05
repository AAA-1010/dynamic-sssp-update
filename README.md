# Dynamic SSSP Update (Phase 2)

**Overview**
Implement the Khanda et al. dynamic SSSP-update algorithm with:
- MPI for inter-node communication
- OpenMP for shared-memory parallelism
- (Bonus later: METIS graph partitioning + OpenCL)

**Repo layout**
- src/        → source code (sequential, MPI, OpenMP drivers)
- datasets/   → input graphs & generated change streams
- scripts/    → data prep & experiment-run scripts
- experiments/→ raw logs and final CSVs
- docs/       → performance report, slides, docs

**Quick start**
1. Install MPI (e.g. Open MPI) and a compiler with OpenMP support
2. mkdir build && cd build
3. cmake .. && make  # or use the provided Makefile
4. Run sequential: ./sssp_seq <graph> <changes>
5. Run MPI+OpenMP:
   mpirun -np 4 ./sssp_mpi_omp <graph> <changes> --threads 8
