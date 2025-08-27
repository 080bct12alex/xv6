

## Step by step to use xv6

  

  Windows 11 → xv6 (RISC-V) → manipulate OS functions for case study.

  

⦁ Stepwise view

  

Windows → WSL2 → QEMU → xv6 → user programs

  

Windows 11 → runs WSL2 Ubuntu

  

WSL2 → runs QEMU emulator

  

QEMU → boots xv6 kernel

  

xv6 → runs shell and user programs inside QEMU

  



A) Set up xv6 on Windows 11 using WSL2

i) Install WSL + Ubuntu

  

Open PowerShell (Admin) → run:

  

    wsl --install

  

To check/set versions later:

  

    wsl -l -v
then

    wsl --set-default-version 2

  
  

Reboot if asked, then run WSL in PowerShell or launch the Ubuntu app from Start (download from Microsoft store) or WSL app

and create your Linux username/password.

  

ii) Install toolchain in Ubuntu (inside WSL)

   *Update base system*

     sudo apt update && sudo apt upgrade -y

  *Build tools & debugger*

     sudo apt install -y build-essential git gdb-multiarch



 *QEMU (RISC-V)*

-  (one of these will exist depending on your Ubuntu version)

       sudo apt install -y qemu-system-riscv64 
      or
       
       sudo apt install -y qemu-system-misc

  

 *RISC-V cross-compiler ( bare-metal “newlib”)*

     sudo apt install -y gcc-riscv64-unknown-elf

  

⦁ qemu-system-riscv64/qemu-system-misc give QEMU with the RISC-V system emulator.

⦁ gcc-riscv64-unknown-elf is the cross-compiler package Ubuntu provides.

⦁ RISC-V cross-compiler for RISC-V bare-metal “newlib” toolchain (needed by xv6)

  
  
*Verify the two key tools installed correctly:*

     qemu-system-riscv64 --version

     riscv64-unknown-elf-gcc --version

  

If qemu-system-riscv64 isn’t found, install the explicit package qemu-system-riscv64.

  
  

iii) Get xv6 (RISC-V) and run it

In Ubuntu (WSL)

    cd ~

    git clone https://github.com/mit-pdos/xv6-riscv.git

    cd xv6-riscv

  

 *Build and boot xv6 in QEMU (text mode)*

    make qemu

  

⦁ (needs RISC-V “newlib” toolchain + QEMU)

  
  

You’ll see:

  

     xv6 kernel is booting

     init: starting sh

     $

  

This ( $ ) is the xv6 shell. — you can run programs, modify kernel, and test syscalls.

Run a few commands to test:

$ ls

$ echo hello

$ usertests

  
  

(QEMU exit: Ctrl-a, then x.)

  
  

NOTE : xv6 is a teaching OS, modeled after Unix V6. It does not have a graphical interface, only a text-mode console.

For simple gui

i. X Server in Windows

If you want to run Linux GUI apps via WSL, you’d need an X server like VcXsrv or Windows’ built-in WSLg.

  

Install VcXsrv and start it.

  

Export the display in WSL:

  

    export DISPLAY=:0

  

Then run QEMU with GUI.

  

⦁ xv6-riscv includes its own kernel, so we don’t need extra firmware.

  
  

 Run xv6 in terminal mode (text-based)

make qemu-terminal

or

make qemu

  

# Run xv6 in GUI mode (WSLg)

    make qemu-gui

  
  
  
  

2) Your first tiny change (hello program)

  

A super quick “I changed xv6!” exercise:

  

Create a user program:

  

    nano user/hello.c

  
  

Paste:

  

  #include "kernel/types.h"

#include "user/user.h"

  

int main(void) {

printf("hello xv6\n");

exit(0);

}

  
  

Register it in the user build (tell xv6 to compile it). Open:

  

    nano user/Makefile

  
  

Find the line starting with UPROGS= and append _hello somewhere in the list, e.g.:

  

    UPROGS= ... _echo _ls _cat _sh _hello

  
  

Rebuild & run:

  

    make clean

    make qemu

  
  At the $ prompt:

  

    $ hello

 hello xv6

  
 3) Things to try

Tweak the scheduler to print run counts.

  

Add a new system call and call it from a user program.

  

Explore the xv6 book while you read the code—it’s short and super helpful.

   pdos.csail.mit.edu

  

