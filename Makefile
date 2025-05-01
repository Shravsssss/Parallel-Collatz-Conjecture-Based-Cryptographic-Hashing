# Makefile for Parallel Cryptographic Hashing with Collatz Conjecture

# Directories
SRCDIR := src
BINDIR := bin

# Compilers and flags
CC       := gcc
MPI_CC   := mpicc
CFLAGS   := -O3 -march=native
OMPFLAGS := -fopenmp
MPIFLAGS := -O3 -march=native

# Libraries
SSL_LIBS := -lssl -lcrypto

# Targets
TARGETS := sequential openmp mpi hybrid

.PHONY: all clean

all: $(BINDIR) $(TARGETS)

# Ensure bin directory exists
$(BINDIR):
	mkdir -p $(BINDIR)

# Sequential implementation
sequential: $(BINDIR)/sequential

$(BINDIR)/sequential: $(SRCDIR)/sequential.c | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $< $(SSL_LIBS)

# OpenMP implementation
openmp: $(BINDIR)/openmp

$(BINDIR)/openmp: $(SRCDIR)/omp.c | $(BINDIR)
	$(CC) $(CFLAGS) $(OMPFLAGS) -o $@ $< $(SSL_LIBS)

# MPI implementation
mpi: $(BINDIR)/mpi

$(BINDIR)/mpi: $(SRCDIR)/mpi.c | $(BINDIR)
	$(MPI_CC) $(MPIFLAGS) -o $@ $< $(SSL_LIBS)

# Hybrid MPI+OpenMP implementation
hybrid: $(BINDIR)/hybrid

$(BINDIR)/hybrid: $(SRCDIR)/hybrid.c | $(BINDIR)
	$(MPI_CC) $(MPIFLAGS) $(OMPFLAGS) -o $@ $< $(SSL_LIBS)

clean:
	rm -rf $(BINDIR)
