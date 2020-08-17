# memory_tracker

## allocations.c
  
Use to trace (de-)allocations done by malloc family of routines, which covers malloc, calloc, realloc, memalign, mmap, free, munmap.
But it does not cover allocation done by libc internally using mmap. To trace that, use mmaptracker.c

Allocation traces are generated in a file which can be configured using env variable `ALLOCATION_TRACE`.

## mmaptracker.c

Traces (de-)allocations done through (un)mmap system call by intercepting the calls using a library for intercepting system calls,
which is https://github.com/pmem/syscall_intercept. This library relies on `libcapstone` for disassembling.
Since this program traces mmap allocations by intercepting system call, it would be able to trace all the allocations whether by libc or the
application or another library. That implies some of the allocations would overlap with those traced by `allocations.c`.
To find only the allocations done by libc internally using mmap, use `mmapmatch.sh` as mentioned below.

Allocation traces are generated in a file which can be configured using env variable `MMAP_TRACE`.

## active_allocations.c

This program processes allocation traces generated by `allocations.c` or `mmaptracer.c` and generate a list of active allocations
(that is, allocations that have not been freed yet).

## Usage:

Make sure `libcapstone` and `libsyscall_intercept` are installed.

`libsyscall_intercept` can be installed by following build process outline in https://github.com/pmem/syscall_intercept.

On ubuntu `libcapstone` can be installed using following command:

  `apt install -y libcapstone-dev`

Generate the executables by running `make`. This would generate two libraries:
1. `liballoctracker.so` from `allocations.c`
2. `libmmaptracker.so` from `mmaptracker.c`

and an executable `active_allocations`.

To use the libraries for tracing allocations, use it as follows:
```
export ALLOCATION_TRACE="alloc_trace.txt"
export MMAP_TRACE="mmap_trace.txt"
LD_PRELOAD=<path to libmmaptracker.so>:<path to liballoctracker.so> <executable>
```

Once the program has completed, the trace files need to be processed using `active_allocations` as:

```
/home/ashu/data/memory_tracker/active_allocations ${ALLOCATION_TRACE} active_allocations.txt &> active_allocations.log
/home/ashu/data/memory_tracker/active_allocations ${MMAP_TRACE} active_mmap.txt &> active_mmap.log
```

From the traces generated by `mmaptracker.c`, we only need to find the allocations done by libc internally using mmap, because rest of
the allocations are already covered by `allocations.c`.
To do this run `mmapmatch.sh` in the same directory where `active_allocations.txt` and `active_mmap.txt` are present.
This script would append the allocations done by libc internally using mmap to `active_allocations.txt`.
So at the end `active_allocations.txt` would have the trace of all allocations.
