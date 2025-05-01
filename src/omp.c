#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <omp.h>

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

void print_sha256_digest(const unsigned char *digest, FILE *out) {
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        fprintf(out, "%02x", digest[i]);
    }
}

int main() {
    const unsigned long long start = 1000000;
    FILE *out = fopen("64_omp_results.txt", "w");
    if (!out) {
        perror("fopen");
        return EXIT_FAILURE;
    }

    for (unsigned long long end = 2000000; end <= 15000000; end += 1000000) {
        int total_numbers = (int)(end - start + 1);
        fprintf(out, "==== Range: %llu to %llu ====\n", start, end);
        fprintf(out, "Total numbers: %d\n", total_numbers);

        // Allocate
        unsigned long long *collatz_hashes = malloc(total_numbers * sizeof(*collatz_hashes));
        unsigned char (*sha256_digests)[SHA256_DIGEST_LENGTH] =
            malloc(total_numbers * SHA256_DIGEST_LENGTH);
        if (!collatz_hashes || !sha256_digests) {
            fprintf(stderr, "Allocation failed\n");
            return EXIT_FAILURE;
        }

        // Stage 1
        double t1 = omp_get_wtime();
        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < total_numbers; i++) {
            collatz_hashes[i] = collatz_hash(start + i);
        }
        t1 = omp_get_wtime() - t1;
        fprintf(out, "[Stage 1] Collatz hashing time: %.6f s\n", t1);

        // Stage 2
        double t2 = omp_get_wtime();
        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < total_numbers; i++) {
            final_mix_with_sha256(collatz_hashes[i], sha256_digests[i]);
        }
        t2 = omp_get_wtime() - t2;
        fprintf(out, "[Stage 2] SHA-256 mixing time: %.6f s\n", t2);

        // Final digest
        unsigned char final_digest[SHA256_DIGEST_LENGTH] = {0};
        for (int i = 0; i < total_numbers; i++) {
            for (int j = 0; j < SHA256_DIGEST_LENGTH; j++) {
                final_digest[j] ^= sha256_digests[i][j];
            }
        }
        fprintf(out, "Final Two-Stage SHA-256 Mixed Hash:\n");
        print_sha256_digest(final_digest, out);
        fprintf(out, "\n");

        // Summary
        fprintf(out, "Total parallel time: %.6f s\n", t1 + t2);
        fprintf(out, "====================================\n\n");

        free(collatz_hashes);
        free(sha256_digests);
    }

    fclose(out);
    return 0;
}

