Fast Approximate String Matching with CUDA acceleration
========================================
Project by J George, S Vinayakumar, Ananth Dileepkumar


Description
========================================
Finds matches for pattern DNA sequence in the given FASTA format genome, 
allowing for k errors(substitutions, omissions, insertions)

Implemention based on Baeza Yates Navarro parallel NFA based approximate string matching

Optimized for NVIDIA Kepler architecture

Usage
=========================================
Build row_gpu.cu using nvcc: 
http://docs.nvidia.com/cuda/cuda-compiler-driver-nvcc

Run generated executable with target string in file "sequence.fasta" in 
the same folder

Pattern string is hardwired for now, needs to be edited in source and 
rebuilt every time.

Algorithm, results and discussion
=========================================
See report 614_2012C_approximate_string_matching_dileepkumar_george_binayakumar_evaluation


