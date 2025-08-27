
## xv6: Manipulate the scheduler from the default Round-Robin (RR) with Highest Response Ratio Next (HRRN)

This repository contains an xv6 (RISC-V) kernel modification that replaces the default Round-Robin (RR) scheduler with a Highest Response Ratio Next (HRRN) scheduler. The goal is educational: to learn how CPU scheduling works by implementing HRRN in `kernel/proc.c` and running the modified xv6 under WSL2 + QEMU.

----------

## Contents

-   `kernel/` — modified kernel sources (including `proc.c` changes)
       
      - The scheduler is implemented in `kernel/proc.c` — look for the `scheduler()` function and the `struct proc` definition. That's the entry point for changing how the kernel picks the next process to run. 
    
-   `user/` — userland test programs used to exercise scheduling
    

    

----------

## Overview

The default xv6 scheduler uses round-robin: it walks the process table and picks the next runnable process (preemptively) on each timer tick. HRRN is a non-preemptive scheduling discipline that chooses the runnable process with the highest **response ratio**:

```
ResponseRatio = (WaitingTime + EstimatedBurstTime) / EstimatedBurstTime
                 = 1 + (WaitingTime / EstimatedBurstTime)

```

This favors short jobs but raises the priority of long-waiting jobs to avoid starvation. Note: HRRN requires or assumes some estimate of a process's CPU burst time; in this repo we provide a simple estimation strategy (see Implementation).

----------

## Prerequisites (tested on WSL2 Ubuntu)

1.  Windows 10/11 with WSL2 enabled. Install an Ubuntu distribution .
    
2.  Development packages and RISC‑V toolchain & QEMU. :
    

```bash
# Install dependencies
sudo apt update && sudo apt upgrade -y
sudo apt install -y build-essential git gdb-multiarch
sudo apt install -y qemu-system-misc
sudo apt install -y qemu-system-riscv64
sudo apt autoremove -y
sudo apt install -y gcc-riscv64-unknown-elf


# Verify installation
qemu-system-riscv64 --version
riscv64-unknown-elf-gcc --version

```
(Depending on your distro you may prefer `riscv64-unknown-elf-*` toolchain, or the toolchain recommended by the xv6 repo you cloned.)

3.  A working `make` and `qemu-system-riscv64` in PATH.
    

----------

## Clone, build, and run xv6

```bash
# Clone MIT xv6-riscv (or my fork)
git clone https://github.com/mit-pdos/xv6-riscv.git
cd xv6-riscv

# (Optional)  copy kernel/ from  xv6-riscv modify from my repo

make clean
make
make qemu # or: make qemu SCHEDULER=HRRN if you added a make flag

```


----------


