#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <CL/cl.h>

// http://csrc.nist.gov/publications/fips/fips180-2/fips180-2.pdf

// swap byte endian
uint32_t swapE32(uint32_t val) {
    uint32_t x = val;
    x = (x & 0xffff0000) >> 16 | (x & 0x0000ffff) << 16;
    x = (x & 0xff00ff00) >>  8 | (x & 0x00ff00ff) <<  8;
    return x;
}

uint64_t swapE64(uint64_t val) {
    uint64_t x = val;
    x = (x & 0xffffffff00000000) >> 32 | (x & 0x00000000ffffffff) << 32;
    x = (x & 0xffff0000ffff0000) >> 16 | (x & 0x0000ffff0000ffff) << 16;
    x = (x & 0xff00ff00ff00ff00) >>  8 | (x & 0x00ff00ff00ff00ff) <<  8;
    return x;
}

// print hex numbers
void hex(void* buffer, size_t len) {
    for(size_t i = 0; i < len; i++) {
        printf("%02x", ((char*)buffer)[i] & 0xff);
        if(i % 4 == 3)
            printf(" ");
    }
}

void hexOutput(void* buffer, size_t len) {
    hex(buffer, len);
    printf("\n");
}

int sha256(char* msg, cl_context context, cl_command_queue q, cl_kernel kernel) {
    size_t len = strlen(msg);

    // 5.1.1
    uint64_t l = len * sizeof(char) * 8;
    size_t k = (448 - l - 1) % 512;
    if(k < 0) k += 512;
    assert((l+1+k) % 512 == 448);

    size_t msgSize = l + 1 + k + 64;

    char* msgPad = (char*)calloc((msgSize / 8), sizeof(char));
    memcpy(msgPad, msg, len);
    msgPad[len] = 0x80;
    l = swapE64(l);
    memcpy(msgPad+(msgSize/8)-8, &l, 8);

    // 5.2.1
    size_t N = msgSize / 512;

    // 6.2
    uint32_t* M = (uint32_t*)msgPad;

    for(size_t i = 0; i < N * 16; i++) {
        M[i] = swapE32(M[i]);
    }

    cl_int ret;
    cl_mem in = clCreateBuffer(context, CL_MEM_READ_ONLY, 64 * N * sizeof(uint32_t), NULL, &ret);
    cl_mem out = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 8 * sizeof(uint32_t), NULL, &ret);

    clEnqueueWriteBuffer(q, in, CL_TRUE, 0, 64 * N * sizeof(uint32_t), M, 0, NULL, NULL);

    clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&in);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&out);

    size_t totalSize = 1;
    size_t groupSize = 1;
    clEnqueueNDRangeKernel(q, kernel, 1, NULL, &totalSize, &groupSize, 0, NULL, NULL);

    // result
    uint32_t H[8];
    clEnqueueReadBuffer(q, out, CL_TRUE, 0, 8 * sizeof(uint32_t), H, 0, NULL, NULL);

    for(size_t i = 0; i < 8; i++) {
        H[i] = swapE32(H[i]);
        hex(&H[i], 4);
    }
    printf("\n");

    free(msgPad);
    return 0;
}

int main(int argc, char* argv[]) {
    if(argc < 2) {
        printf("Usage: sha256 <string>\n");
        return 0;
    }

    // load file
    FILE* f = fopen("sha256.cl", "r");
    if(!f) {
        printf("Could not load sha256.cl!\n");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long clLen = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* clFile = (char*)malloc(clLen);
    fread(clFile, 1, clLen, f);

    // get system info
    cl_platform_id  platformID = NULL;
    cl_device_id    deviceID = NULL;
    cl_uint         numPlatforms, numDevices;
    cl_int          ret;

    clGetPlatformIDs(1, &platformID, &numPlatforms);
    clGetDeviceIDs(platformID, CL_DEVICE_TYPE_DEFAULT, 1, &deviceID, &numDevices);

    // OpenCL context and command queue
    cl_context context = clCreateContext(NULL, 1, &deviceID, NULL, NULL, &ret);
    cl_command_queue q = clCreateCommandQueue(context, deviceID, 0, &ret);

    // compile
    cl_program program = clCreateProgramWithSource(context, 1, (const char**)&clFile,
            (const size_t*)&clLen, &ret);
    clBuildProgram(program, 1, &deviceID, NULL, NULL, NULL);
    cl_kernel k = clCreateKernel(program, "sha256", &ret);


    int result = sha256(argv[1], context, q, k);

    clFlush(q);
    clFinish(q);
    clReleaseKernel(k);
    clReleaseProgram(program);
    clReleaseCommandQueue(q);
    clReleaseContext(context);

    return result;
}
