#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <mpi.h>

unsigned long long collatz_steps(unsigned long long n) {
    unsigned long long steps = 0;
    while (n != 1) {
        n = (n % 2 == 0) ? n / 2 : 3 * n + 1;
        steps++;
    }
    return steps;
}

unsigned long long collatz_hash(unsigned long long n) {
    return n ^ collatz_steps(n);
}

void final_mix_with_sha256(unsigned long long input, unsigned char *output) {
    unsigned char data[sizeof(unsigned long long)];
    memcpy(data, &input, sizeof(input));
    SHA256(data, sizeof(data), output);
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const unsigned long long global_start = 1000000;
    FILE *out = NULL;
    if (rank == 0) {
        out = fopen("64_mpi_results.txt", "w");
        if (!out) { perror("fopen"); MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE); }
    }

    for (unsigned long long global_end = 2000000; global_end <= 15000000; global_end += 1000000) {
        unsigned long long global_range = global_end - global_start + 1;

        // determine local subrange
        unsigned long long base = global_range / size;
        unsigned long long rem  = global_range % size;
        unsigned long long local_start = global_start + rank * base + (rank < rem ? rank : rem);
        unsigned long long local_count = base + (rank < rem ? 1 : 0);

        // Stage 1: Collatz
        MPI_Barrier(MPI_COMM_WORLD);
        double t1_start = MPI_Wtime();
        unsigned long long *local_collatz = malloc(local_count * sizeof(*local_collatz));
        for (unsigned long long i = 0; i < local_count; i++) {
            local_collatz[i] = collatz_hash(local_start + i);
        }
        double t1 = MPI_Wtime() - t1_start;

        // Stage 2: SHA-256
        MPI_Barrier(MPI_COMM_WORLD);
        double t2_start = MPI_Wtime();
        unsigned char (*local_digests)[SHA256_DIGEST_LENGTH] =
            malloc(local_count * SHA256_DIGEST_LENGTH);
        for (unsigned long long i = 0; i < local_count; i++) {
            final_mix_with_sha256(local_collatz[i], local_digests[i]);
        }
        double t2 = MPI_Wtime() - t2_start;

        // Local combine of digests
        unsigned char local_final[SHA256_DIGEST_LENGTH] = {0};
        for (unsigned long long i = 0; i < local_count; i++)
            for (int j = 0; j < SHA256_DIGEST_LENGTH; j++)
                local_final[j] ^= local_digests[i][j];

        // Reduce stage times and final digests
        double total_t1 = 0.0, total_t2 = 0.0;
        MPI_Reduce(&t1, &total_t1, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&t2, &total_t2, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

        unsigned char *all_local = NULL;
        if (rank == 0) {
            all_local = malloc(size * SHA256_DIGEST_LENGTH);
        }
        MPI_Gather(local_final, SHA256_DIGEST_LENGTH, MPI_UNSIGNED_CHAR,
                   all_local, SHA256_DIGEST_LENGTH, MPI_UNSIGNED_CHAR,
                   0, MPI_COMM_WORLD);

        // Rank 0 writes results
        if (rank == 0) {
            // global combine
            unsigned char global_final[SHA256_DIGEST_LENGTH] = {0};
            for (int r = 0; r < size; r++)
                for (int j = 0; j < SHA256_DIGEST_LENGTH; j++)
                    global_final[j] ^= all_local[r*SHA256_DIGEST_LENGTH + j];

            fprintf(out, "==== Range: %llu to %llu ====\n", global_start, global_end);
            fprintf(out, "Stage 1 (Collatz) total time: %.6f s\n", total_t1);
            fprintf(out, "Stage 2 (SHA-256) total time: %.6f s\n", total_t2);
            fprintf(out, "Combined compute time     : %.6f s\n\n", total_t1 + total_t2);
            fprintf(out, "Final digest: ");
            for (int j = 0; j < SHA256_DIGEST_LENGTH; j++)
                fprintf(out, "%02x", global_final[j]);
            fprintf(out, "\n\n");

            free(all_local);
        }

        free(local_collatz);
        free(local_digests);
    }

    if (rank == 0) fclose(out);
    MPI_Finalize();
    return 0;
}

