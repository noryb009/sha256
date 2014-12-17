#!/bin/sh
gcc -o sha256 -g \
    -std=c11 \
    -l OpenCL \
    sha256.c
