# Parallel Cryptographic Hashing with Collatz Conjecture

## Overview

This project implements and evaluates a novel two-stage cryptographic hash that combines the chaotic step-counts of the Collatz Conjecture with the cryptographic strength of SHA-256. Each input integer is first mapped to an eight-byte “Collatz hash” (the XOR of the original number and its Collatz iteration count), then SHA-256 is applied to that intermediate—injecting extra entropy without altering standard security guarantees.

We provide four implementations:

- **Sequential** (`two_stage_sha256_sequential.c`)  
- **OpenMP** (`omp1.c`) — shared-memory multithreading  
- **MPI** (`mpi.c`) — distributed-memory message passing  
- **Hybrid MPI+OpenMP** (`hybrid.c`) — intra-node threading combined with inter-node distribution  

Comprehensive scaling studies on a 16-core Skylake cluster demonstrate:

- **Up to 6×** speedup with OpenMP (16 threads, ~90 % efficiency)  
- **Modest gains** up to 1.15× with MPI (4 ranks)  
- **3× combined** improvement with Hybrid (4 ranks × 4 threads)  

Stage-wise timing and Amdahl’s Law analysis reveal that **~65 %** of runtime is spent in Collatz sequence generation and **~35 %** in SHA-256 mixing.

---

## Prerequisites

- Linux or Unix-like OS  
- GCC 10.2+ (with OpenMP support)  
- OpenSSL (dev headers & libs)  
- OpenMPI 4.0.3+  
- `make`, `git`, Python 3, and standard build tools  

For optimal performance:

1. **Disable hyper-threading** (to avoid SMT sharing penalties)  
2. **Enable NUMA-aware allocation** (e.g. `numactl --interleave=all`)  
3. **Use a fast interconnect** (InfiniBand) for MPI runs  

---

## Repository Layout

```text
.
├── README.md
├── Makefile
├── bin/                      # Compiled binaries
├── src/
│   ├── sequential.c
│   ├── omp1.c
│   ├── mpi.c
│   └── hybrid.c
├── results/                  # Raw log files and CSVs
└── docs/
    ├── parallel-crypto-hash.tex
    └── figures/              # PDF/PNG for paper inclusion
```

---

## Building

From the project root, run:

```bash
make all
```

This will compile and place executables into `bin/`:

- `bin/sequential`  
- `bin/openmp`  
- `bin/mpi`  
- `bin/hybrid`  

You can also build individual targets:

```bash
make sequential   # two_stage_sha256_sequential
make openmp       # omp1
make mpi          # mpi
make hybrid       # hybrid
```

---

## Usage

All binaries accept optional start/end arguments; defaults are `start=1000000`, `end=2000000`.

### Sequential

```bash
./bin/sequential [start] [end]
```

### OpenMP

```bash
export OMP_NUM_THREADS=16
./bin/openmp [start] [end]
```

### MPI

```bash
mpirun -np 4 ./bin/mpi [start] [end]
```

### Hybrid MPI+OpenMP

```bash
export OMP_NUM_THREADS=4
mpirun -np 4 ./bin/hybrid [start] [end]
```

Each run prints:

1. **Stage 1** (Collatz) time  
2. **Stage 2** (SHA-256) time  
3. **Total** compute time  
4. **Final** 32-byte digest  

---

## Performance Snapshot

| Variant          | Resources       | Time (s) | Speedup  | Efficiency |
|------------------|-----------------|----------|----------|------------|
| Sequential       | 1 core          | 1.080    | 1.00×    | 100 %      |
| OpenMP           | 16 threads      | 0.179    | 6.03×    | 94 %       |
| MPI              | 4 ranks         | 0.929    | 1.16×    | 29 %       |
| Hybrid           | 4 ranks × 4 thr | 0.364    | 2.97×    | 19 %       |

Refer to the `docs/parallel-crypto-hash.tex` for full tables and graphs.

---

## Contributing

1. **Fork** the repository.  
2. **Clone** your fork and create a feature branch:  
   ```bash
   git clone https://github.com/YourUsername/collatz-sha256-parallel.git
   cd collatz-sha256-parallel
   git checkout -b feature/awesome-improvement
   ```
3. **Implement** your changes, run tests/benchmarks.  
4. **Push** and open a **Pull Request** with a clear description and performance impact.  
```
