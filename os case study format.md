
## **Factor I consider while choosing Operating system to use and manipulate function of the operating system for the case study for my operating system course**

- a basic, easy-to-use operating system where i can directly explore and manipulate core OS functions without too much complexity.
- a simulated educational OS:


## **How to present in case study**

Before  : describe xv6 default round-robin scheduling.
Change: explain that you added a priority field and modified the scheduler to pick by priority
Demo : show two processes with different priorities and how one gets more CPU time.

 Analysis: note trade-offs — starvation risk if low-priority processes never run.

  
###### &nbsp; *Case Study SAMPLE : xv6 Operating System*

1\.  *Introduction*

xv6 is a small, Unix-like teaching operating system developed at MIT for educational purposes. It is designed to run on the x86 architecture and is a re-implementation of the sixth edition Unix (Version 6). Because of its simplicity and clean structure, xv6 is widely used in operating system courses.

2\.  *Key Features*

Written in C (with a small amount of assembly).

Monolithic kernel — all OS services run in kernel space.

Supports basic process management, file system, and device drivers.

Minimal system calls (~20), making it easy to understand.

Runs inside QEMU or Bochs emulator / virtualbox — no risk to your main PC.

3\.  *Architecture*

- User Space:
   - User programs.
   - Shell for command execution.

- Kernel Space:
  - Process scheduling and management.
  - Memory allocation.
  - File system handling.
  - System call interface.

- Hardware Interface:
    - Drivers for console, disk, and timer.

4\.  *Basic Functions You Can Manipulate*
- Function Area Example Manipulations
  - Process Scheduling : Change the scheduler to prioritize certain processes.
  - System Calls :  Add a new system call, e.g., getuptime() to return system ticks.
  - File System : Modify file limits or add logging features.
  - Memory Management : Change the allocation policy or memory limits.
  - Shell Commands : Add a new built-in shell command.

5\. Example Experiment: 
  
6\.  *Advantages for Learning*
Very small and readable — kernel code can be understood in a week. 
Encourages experimentation without the complexity of modern OSes.
Matches academic OS theory to actual code.

7\.  *Conclusion*
xv6 is an excellent choice for understanding fundamental operating system concepts in a practical way. It is small enough for students to read the entire kernel, yet complete enough to run real programs. By modifying and experimenting with its functions, students gain hands-on experience with process management, file systems, and system calls.

  
  
  


  
  
  ###

**Conclusion** 

- RR: Fair but ignores waiting time; long-wait jobs may starve.
- HRRN: Favors long-wait jobs, so starvation is minimized.

     *Simplification* : xv6 doesn’t predict burst time, so we use a fixed guess (e.g., 1 tick).

Tracks enter_time to compute waiting time.

**Summary** :

“By switching from round-robin to HRRN, the scheduler dynamically prioritizes processes that have waited longer, improving responsiveness and fairness while still handling short jobs efficiently.”




