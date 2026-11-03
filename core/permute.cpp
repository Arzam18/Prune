#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

#include "NNUE.hpp"
#include "simd_definitions.hpp"

static constexpr bool isPW = true;
static constexpr bool isMergedKingPlanes = true;
constexpr int RawInputSize = PSQ_SIZE + isMergedKingPlanes * 64;
#ifdef ARCHSIZE
constexpr int simdSize = ARCHSIZE / 16;
#else
constexpr int simdSize = nb<16>;
#endif
constexpr int mask = simdSize * 2 - 1;
constexpr int LaneSize = 128 / 16;
constexpr int nbLane = simdSize * 2 / LaneSize;

#if defined(PRUNE_ANDROID_NEON_LAYOUT)
static_assert(simdSize == 8,
              "Android AArch64 NEON model conversion must use the 128-bit SIMD layout");
static_assert(sizeof(simd<8>) == 16 && sizeof(simd<16>) == 16 && sizeof(simd<32>) == 16,
              "Android AArch64 NEON vectors must be 128 bits");
#endif

const auto unpacked = [] {
    array<int8_t, simdSize * 2> res{};
    for (int i = 0; i < simdSize * 2; i++) {
        res[i] = i % simdSize / LaneSize * 2 * LaneSize + i % LaneSize + (i / simdSize) * LaneSize;
    }
    return res;
}();

int permute(int n) {
    return (n & ~mask) | unpacked[n & mask];
}

template <int input, int output, typename wtype>
struct layer {
    alignas(64) wtype weights[input][output];
    alignas(64) int32_t bias[output];
};

template <bool isPermuted>
struct inputlayer {
    alignas(64) int16_t psqweights[nbInputBuckets][isPermuted ? PSQ_SIZE : RawInputSize][L1];
    alignas(64) int8_t threatweights[PP_SIZE + THREAT_SIZE][L1];
    alignas(64) int16_t biases[L1];
};

struct lastLayers {
    layer<L1 * (2 - isPW), L2, int8_t> l1;
    layer<L2*(dualact + 1), L3, int32_t> l2;
    layer<L3, 1, int32_t> l3;
};

template <bool isPermuted>
struct nn {
    inputlayer<isPermuted> FT;
    lastLayers laterLayers[BUCKET];
};

int col(const int& square) {
    return square & 7;
}

int row(const int& square) {
    return square >> 3;
}

static bool getFileSize(FILE* file, long& size) {
    if (fseek(file, 0, SEEK_END) != 0)
        return false;
    size = ftell(file);
    if (size < 0)
        return false;
    return fseek(file, 0, SEEK_SET) == 0;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s input-network.bin output-model.bin\n", argv[0]);
        return EXIT_FAILURE;
    }

    auto nn_in = make_unique<nn<false>>();
    auto nn_out = make_unique<nn<true>>();

    FILE* fin = fopen(argv[1], "rb");
    if (!fin) {
        perror("Cannot open input NNUE network");
        return EXIT_FAILURE;
    }

    long inputSize = 0;
    if (!getFileSize(fin, inputSize)) {
        fprintf(stderr, "Cannot determine input NNUE network size\n");
        fclose(fin);
        return EXIT_FAILURE;
    }

    const size_t expectedInputSize = sizeof(*nn_in);
    if (static_cast<unsigned long long>(inputSize) !=
        static_cast<unsigned long long>(expectedInputSize)) {
        fprintf(stderr,
                "NNUE network size mismatch: got %ld bytes, expected %zu bytes "
                "(wrong or incompatible network format)\n",
                inputSize, expectedInputSize);
        fclose(fin);
        return EXIT_FAILURE;
    }

    if (fread(&nn_in->FT, sizeof(nn_in->FT), 1, fin) != 1) {
        fprintf(stderr, "Failed to read NNUE feature-transformer data\n");
        fclose(fin);
        return EXIT_FAILURE;
    }

    for (int ob = 0; ob < BUCKET; ob++) {
        if (fread(&nn_in->laterLayers[ob], sizeof(nn_in->laterLayers[ob]), 1, fin) != 1) {
            fprintf(stderr, "Failed to read NNUE output bucket %d\n", ob);
            fclose(fin);
            return EXIT_FAILURE;
        }
        memcpy(&nn_out->laterLayers[ob], &nn_in->laterLayers[ob],
               sizeof(nn_in->laterLayers[ob]));
    }

    if (fclose(fin) != 0) {
        perror("Failed to close input NNUE network");
        return EXIT_FAILURE;
    }

    for (int ib = 0; ib < nbInputBuckets; ib++)
        for (int i = 0; i < RawInputSize; i++)
            for (int k = 0; k < L1; k++) {
                const int pieceType = i / 64 % 6;
                const int color = i / 64 / 6;
                const int pos = i % 64;
                if (pieceType != KING || (col(pos) > 3 && color) ||
                    (col(pos) <= 3 && (inputBuckets[col(pos) | (row(pos) << 2)] != ib) == color)) {
                    nn_out->FT.psqweights[ib][i - 6 * 64 * (pieceType == KING && color)][k] =
                        nn_in->FT.psqweights[ib][i][permute(k)];
                }
            }

    for (int i = 0; i < THREAT_SIZE + PP_SIZE; i++)
        for (int k = 0; k < L1; k++)
            nn_out->FT.threatweights[i][k] = nn_in->FT.threatweights[i][permute(k)];

    for (int k = 0; k < L1; k++)
        nn_out->FT.biases[k] = nn_in->FT.biases[permute(k)];

    FILE* fout = fopen(argv[2], "wb");
    if (!fout) {
        perror("Cannot open output NNUE model");
        return EXIT_FAILURE;
    }

    bool writeOk = fwrite(&nn_out->FT, sizeof(nn_out->FT), 1, fout) == 1;
    for (int i = 0; writeOk && i < BUCKET; i++)
        writeOk = fwrite(&nn_out->laterLayers[i], sizeof(nn_out->laterLayers[i]), 1, fout) == 1;

    if (fflush(fout) != 0)
        writeOk = false;
    if (fclose(fout) != 0)
        writeOk = false;

    if (!writeOk) {
        fprintf(stderr, "Failed to write complete permuted NNUE model\n");
        return EXIT_FAILURE;
    }

    printf("NNUE conversion OK: input=%zu bytes, output=%zu bytes, SIMD=%d-bit\n",
           expectedInputSize, sizeof(*nn_out), simdSize * 16);
    return EXIT_SUCCESS;
}
