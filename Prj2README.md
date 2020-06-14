# README for Project 2, CS356
## Personal Information
Ziqi Zhao (赵梓淇, Bugen Zhao, 518030910211), F1803302, CS, SEIEE, SJTU.

## Report
Please refer to [`Report.pdf`](./Report.pdf)

## Directory Structure
```bash
.
├── common
│   └── syscall_num.h # macro of syscall numbers
├── goldfish -> ~/Android/kernel/goldfish # symbolic link to the kernel
├── hacking # my hacking sources
│   ├── include
│   │   └── linux
│   │       └── bz_mm_limits.h # definition of global vars & helper funcs
│   ├── mm
│   │   ├── bz_mm_limits.c  # body of helper funcs
│   │   ├── bz_oom_killer.c # the killer
│   │   ├── Makefile        # added objects of new sources
│   │   └── page_alloc.c    # added 2 loc to call the killer, at line 2448
│   └── Kconfig # added the killer as a feature
├── killer_test # my tester: correctness, race, performance, time
│   ├── jni
│   │   ├── Android.mk
│   │   └── killer_test.c
│   └── Makefile
├── mm_limit_syscall # syscall module: set, set_time, get
│   ├── Makefile
│   └── mm_limit_syscall.c
├── prj2_test # project tester
│   ├── jni
│   │   ├── Android.mk
│   │   └── prj2_test.c
│   └── Makefile
├── kernel_example.txt      # example output of the killer in kernel
├── Makefile                # Makefile for the project
├── output_testall.txt      # output of 'make testall'
├── output_test.txt         # output of 'make test'
├── Prj2+518030910211.tar   # hand-in archive
└── Prj2README.md           # this README

11 directories, 21 files
```

## Testscript
### Build the kernel
As you can see, the hacking sources to the kernel is all located at `hacking/`, which preserves the structure of the original kernel. I also make a symbolic link `goldfish` to the kernel. If you have the same structure as me, run `make rsync` to sync them, otherwise copy them manually to the kernel.

The killer is implemented as an **optional feature**, which requires you to run `make menuconfig` to enable it before building the kernel! Or the killer won't be built into the kernel.

### Run tests
All build and test commands are in `Makefile`. To run the test, you should determine some required arguments then execute the following commands. For more info about the variables, please refer to `Makefile`.

Please note that running the following tests will **overwrite** the output example in `output_test.txt` and `output_testall.txt`.

```bash
# Start the emulator
make emulator (AVD_NAME=xxx KERNEL_ZIMG=yyy)

# Build and run prj2_test
# It will also output in `output_test.txt`
make test (KID=xxx TOOLCHAIN=yyy)

# Build and run all tests, including my tester (may be slower)
# It will also output in `output_testall.txt`
make testall (KID=xxx TOOLCHAIN=yyy)

# Do some cleanup
make clean
```

### Output Examples
Please refer to `output_test.txt`, `output_testall.txt`, and `kernel_exmaple.txt` for the killer's output in kernel.
