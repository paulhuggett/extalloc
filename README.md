# extalloc

![GitHub](https://img.shields.io/github/license/paulhuggett/extalloc)
[![Linux and macOS Build Status](https://api.travis-ci.com/paulhuggett/extalloc.svg?branch=master)](https://travis-ci.com/paulhuggett/extalloc)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=paulhuggett_extalloc&metric=alert_status)](https://sonarcloud.io/dashboard?id=paulhuggett_extalloc)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/paulhuggett/extalloc.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/paulhuggett/extalloc/context:cpp)

## Table of Contents

*   [Introduction](#introduction)
*   [Tools](#tools)
    *   [mem\_stress](#mem_stress)
    *   [mmap\_stress](#mmap_stress)


## Introduction

A storage allocator using external metadata. Many dynamic storage allocation scheme store their metadata — the collection of allocated and free blocks — within the blocks themselves. In contrast, extalloc stores this information externally: currently in a pair of `std::map<>` instances.

## Tools

In addition to the unit tests, a pair of tools are included to work the allocator code reasonably hard and shake out any bugs.

### mem_stress

A tool which exercises the extalloc library by randomly allocating, freeing, and reallocating blocks of memory.

### mmap_stress

This tool makes the both the stored data and the allocator’s metadata persistent.  It stresses the allocator in the same manner as `mem_stress` above: 

-   It allocates a random number of blocks of random size and fills each with a random value.
-   It frees a random selection of the allocated blocks.

These steps are repeated many times. Next it randomly changes the size of a number of the allocated blocks. Finally, a random number of blocks are freed and the state saved to disk. At each step, the contents of the blocks are checked against the tools expectations.

The tool creates three files:

| File Name        | Description   |
| ---------------- | ------------- |
| `./blocks.alloc` | This holds the `mmap_stress` tool’s own data. This tracks its state: the list of active allocations, their size, and the value with which they have each been filled. |
| `./map.alloc`    | Records the allocator’s metadata. |
| `./store.alloc`  | The stored data. This is a 1 MiB memory-mapped block which is created as the program begins. |

A command such as the following will execute the tool repeatedly: the files produced by one run priming the tests for the next. 

~~~~bash
$ for ctr in {0..99}; do mmap_stress; done
~~~~

This will produce output along the lines of:

~~~~
On start: 0 allocated bytes (0 allocations), 1048576 free bytes (1 blocks).
Allocate checks: ................
Realloc checks: ................
On start: 33792 allocated bytes (270 allocations), 1014784 free bytes (260 blocks).
Allocate checks: ................
Realloc checks: ................
On start: 391467 allocated bytes (3022 allocations), 657109 free bytes (1492 blocks).
Allocate checks: ................
Realloc checks: ................
On start: 90027 allocated bytes (713 allocations), 958549 free bytes (626 blocks).
~~~~
… and so on.
