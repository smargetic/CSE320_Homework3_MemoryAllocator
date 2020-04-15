#CSE320_Homework3

#The purpose of this assignment was to create a memory allocator for the x86-64 architecture.

#The memory allocator had the following constraints:

#Free lists segregated by size class, using first-fit policy within each size class.
#Immediate coalescing of blocks with adjacent free blocks.
#Boundary tags to support efficient coalescing.
#Block splitting without creating splinters.
#Allocated blocks aligned to "double memory row" (16-byte) boundaries.
#Free lists maintained using last in first out (LIFO) discipline.
#Obfuscation of block footers to detect attempts to free blocks not previously obtained via allocation.

#The created memory allocator was implememented through own versions of malloc, realloc, and free functions.
