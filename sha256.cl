// 3.2
uint rotr(uint x, uint n) {
    return (x >> n) | (x << (32-n));
}

uint shr(uint x, uint n) {
    return x >> n;
}

// 4.1.2
uint Ch(uint x, uint y, uint z) {
    return (x & y) ^ (~x & z);
}
uint Maj(uint x, uint y, uint z) {
    return (x & y) ^ (x & z) ^ (y & z);
}
uint ep0(uint x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}
uint ep1(uint x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}
uint sig0(uint x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ shr(x, 3);
}
uint sig1(uint x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ shr(x, 10);
}

__constant uint K[] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// 5.3.2
__constant uint origH[] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

__kernel void sha256(__global const uint *M, __global const uint *N, __global uint *ret) {
    uint v[8];
    uint W[64];
    uint H[8];
    uint T1, T2;

    for(uint i = 0; i < 8; i++) {
        H[i] = origH[i];
    }

    for(uint i = 0; i < N[0]; i++) {
        // 1
        for(uint t = 0; t < 16; t++) {
            W[t] = M[i*16 + t];
        }
        for(uint t = 16; t < 64; t++) {
            W[t] = sig1(W[t-2]) + W[t-7] + sig0(W[t-15]) + W[t-16];
        }

        // 2
        for(uint t = 0; t < 8; t++) {
            v[t] = H[t];
        }

        // 3
        for(uint t = 0; t < 64; t++) {
            T1 = v[7] + ep1(v[4]) + Ch(v[4], v[5], v[6]) + K[t] + W[t];
            T2 = ep0(v[0]) + Maj(v[0], v[1], v[2]);

            v[7] = v[6];
            v[6] = v[5];
            v[5] = v[4];
            v[4] = v[3] + T1;
            v[3] = v[2];
            v[2] = v[1];
            v[1] = v[0];
            v[0] = T1 + T2;
        }

        for(uint t = 0; t < 8; t++) {
            H[t] += v[t];
        }
    }

    for(uint i = 0; i < 8; i++) {
        ret[i] = H[i];
    }
}
