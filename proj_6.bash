#!/bin/bash
#SBATCH -J proj_6
#SBATCH -A cs475-575
#SBATCH -p class
#SBATCH --constraint=v100
#SBATCH --gres=gpu:1
#SBATCH -o proj_6.out
#SBATCH -e proj_6.err
#SBATCH --mail-type=BEGIN,END,FAIL
#SBATCH --mail-user=regerj@oregonstate.edu

for g in 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 1048576 2097152 4194304 8388608
do
    #echo NUMT = $t
    for l in 8 16 32 64 128 256 512
    do
        g++ -DGLOBAL_SIZE=$g -DLOCAL_SIZE=$l project_6.cpp /usr/local/apps/cuda/10.1/lib64/libOpenCL.so.1.1 -o proj6 -w -lm -fopenmp
        ./proj6 mult
    done
done

for g in 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 1048576 2097152 4194304 8388608
do
    #echo NUMT = $t
    for l in 8 16 32 64 128 256 512
    do
        g++ -DGLOBAL_SIZE=$g -DLOCAL_SIZE=$l project_6.cpp /usr/local/apps/cuda/10.1/lib64/libOpenCL.so.1.1 -o proj6 -w -lm -fopenmp
        ./proj6 multAdd
    done
done

for g in 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 1048576 2097152 4194304 8388608
do
    for l in 32 64 128 256
    do
        g++ -DGLOBAL_SIZE=$g -DLOCAL_SIZE=$l project_6.cpp /usr/local/apps/cuda/10.1/lib64/libOpenCL.so.1.1 -o proj6 -w -lm -fopenmp
        ./proj6 multRed
    done
done