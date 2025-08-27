
## **How you can navigate and work with xv6 (Note: If WSL, QEMU , xv6 already exist) :**

1.  *open app (UBUNTU or WSL)*

  

&nbsp; Open WSL in Windows PowerShell

   Start WSL
       
     wsl

If running does not redirect to alex@ALEX:~$, executing the 
   
    cd 
command will switch to alex@ALEX:~$

2\.  *FROM alex@ALEX:~$.*

*Go into the xv6 folder*

&nbsp;  **cd ~/xv6-riscv**

&nbsp;Check the files

 &nbsp; ls

You should see kernel/, user/, Makefile, etc.


2\.  *Build and run xv6 in QEMU*

&nbsp;  **make qemu**

This will compile all the kernel and user programs and start xv6 in a terminal (nographic mode).

Inside xv6

&nbsp;You get a shell like this:

    xv6 kernel is booting
    init: starting sh
    
    $

  