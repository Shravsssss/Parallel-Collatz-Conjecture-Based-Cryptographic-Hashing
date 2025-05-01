#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <time.h>

// Compute the number of steps in the Collatz sequence until 1 is reached.
unsigned long long collatz_steps(unsigned long long n) {
    unsigned long long steps = 0;
    while (n != 1) {
        n = (n % 2 == 0) ? n / 2 : 3 * n + 1;
        steps++;
    }
    return steps;
}

// Stage 1: Compute a basic Collatz-based hash by XOR-ing the number with its step count.
unsigned long long collatz_hash(unsigned long long n) {
    return n ^ collatz_steps(n);
}

// Stage 2: Apply additional cryptographic mixing using SHA-256.
void final_mix_with_sha256(unsigned long long input, unsigned char *output) {
    unsigned char data[sizeof(unsigned long long)];
    memcpy(data, &input, sizeof(unsigned long long));
    SHA256(data, sizeof(unsigned long long), output);
}

// Helper function to print a SHA-256 digest in hexadecimal.
void print_sha256_digest(const unsigned char *digest, FILE *out) {
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        fprintf(out, "%02x", digest[i]);
    }
}

int main() {
    const unsigned long long start = 1000000;
    FILE *out = fopen("all_results.txt", "w");
    if (!out) {
        perror("fopen");
        return EXIT_FAILURE;
    }

    for (unsigned long long end = 2000000; end <= 15000000; end += 1000000) {
        fprintf(out, "==== Range: %llu to %llu ====\n", start, end);

        int total_numbers = (int)(end - start + 1);
        fprintf(out, "Total numbers: %d\n", total_numbers);

        // Allocate memory for Collatz hashes
        unsigned long long *collatz_hashes = malloc(total_numbers * sizeof(unsigned long long));
        if (!collatz_hashes) {
            fprintf(stderr, "Memory allocation error for collatz_hashes!\n");
            fclose(out);
            return EXIT_FAILURE;
        }

        // Stage 1
        fprintf(out, "[Stage 1] Computing Collatz-based hashes...\n");
        clock_t s1 = clock();
        for (int i = 0; i < total_numbers; i++) {
            collatz_hashes[i] = collatz_hash(start + i);
        }
        clock_t e1 = clock();
        double t1 = (double)(e1 - s1) / CLOCKS_PER_SEC;
        fprintf(out, "[Stage 1] Time: %.6f s\n", t1);

        // Allocate memory for SHA-256 digests
        unsigned char (*sha256_digests)[SHA256_DIGEST_LENGTH] =
            malloc(total_numbers * SHA256_DIGEST_LENGTH);
        if (!sha256_digests) {
            fprintf(stderr, "Memory allocation error for sha256_digests!\n");
            free(collatz_hashes);
            fclose(out);
            return EXIT_FAILURE;
        }

        // Stage 2
        fprintf(out, "[Stage 2] Applying SHA-256 mixing...\n");
        clock_t s2 = clock();
        for (int i = 0; i < total_numbers; i++) {
            final_mix_with_sha256(collatz_hashes[i], sha256_digests[i]);
        }
        clock_t e2 = clock();
        double t2 = (double)(e2 - s2) / CLOCKS_PER_SEC;
        fprintf(out, "[Stage 2] Time: %.6f s\n", t2);

        // Combine final digest
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
        fprintf(out, "Timing Summary:\n");
        fprintf(out, "  Stage 1 (Collatz) : %.6f s\n", t1);
        fprintf(out, "  Stage 2 (SHA-256) : %.6f s\n", t2);
        fprintf(out, "  Total             : %.6f s\n", t1 + t2);
        fprintf(out, "====================================\n\n");

        free(collatz_hashes);
        free(sha256_digests);
    }

    fclose(out);
    return 0;
}

