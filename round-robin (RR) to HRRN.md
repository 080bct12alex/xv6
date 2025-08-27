

## **round-robin (RR) to HRRN**
 Run xv6 with WSL2/QEMU and manipulate the scheduler from the default round-robin (RR) to HRRN (Highest Response Ratio Next).

##### **Here’s a clean summary of what you need and how to structure it for your case study:**

1️⃣ *Default behavior in xv6*

The scheduler() function in kernel/proc.c loops through the proc table.
   Picks the first process in RUNNABLE state.
Gives it a fixed tick (quantum), then moves to the next process.

This is round-robin scheduling. ✅

  
2️⃣ HRRN logic

HRRN selects the process with the highest response ratio:

Waiting Time → time process has been in RUNNABLE state (wtime)
Burst Time → estimated CPU time needed (can use a fixed guess in xv6)

  
3️⃣ ***Steps to implement HRRN in xv6***

a) *Add fields in proc.h*

  

uint rtime; // total time process has run

  

uint wtime; // total waiting time in RUNNABLE state

  

uint enter\_time; // last time process became RUNNABLE  (optional)

  
  
  

b) Track when a process becomes runnable (optional)

In *kernel/proc.c*, inside **wakeup()**:

    p->enter\_time = ticks; // record the moment it becomes RUNNABLE
or 
   
    clkintr

c) *Update waiting/runtime counters*  

In scheduler() before selecting a process or clkintr  

d) *Replace round-robin selection*

e) *Test*

Create a couple of test programs in **user/** that loop and sleep.

Add them to 
   
    UPROGS 
in 

    user/Makefile.

  
 Run:

     make clean && make qemu

In xv6 shell, launch multiple processes and observe scheduling.

**4️⃣ Case study presentation**

**Before: show default round-robin code snippet and explain and show equal time slices.**

**After: show HRRN logic, explain and show waiting time boosting and reduced starvation.**

