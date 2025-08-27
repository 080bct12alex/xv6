
**Manipulatable functions:**
- Process scheduling.
in xv6, process scheduling is one of the easiest and most visible kernel functions you can manipulate for a case study.

 Here’s how to do it step-by-step so it runs in QEMU and you can show a clear before/after result.
    
   Where scheduling happens in xv6
In xv6 (RISC-V), the scheduler lives in:

    kernel/proc.c

  The key function is:

    void scheduler(void)

 It loops forever, picking the next RUNNABLE process and switching to it.

  
## Is round robin is default in xv6 ?
Yes ✅ — Round Robin is the default scheduling policy in xv6.

 Here’s why:
 - In scheduler() (inside kernel/proc.c), xv6 loops linearly through the process table.
It picks the first process it finds in the RUNNABLE state and gives it a fixed time slice (usually 1 tick in xv6).
After that tick, the timer interrupt triggers a context switch, and the scheduler moves on to the next runnable process in the list.
Because it cycles through the process list and gives each runnable process the same quantum, this is exactly round-robin scheduling.


  ## ***Basic manipulation idea to change the scheduling policy from the default round-robin to something***

 - We can change the scheduling policy from the default round-robin to something like:
     - Priority-based (run lowest-numbered priority first).
     - Longest-waiting process first (run process that’s been waiting the longest).
     - Favor a specific process name (e.g., always run hello before others).

  
  
## **change the scheduling policy from the default round-robin to hrrn**

- switching xv6 from round-robin to HRRN (Highest Response Ratio Next) for case study, and it’s can be a nice example of modifying an OS scheduling algorithm.

 Step-by-step changes so it works in QEMU:

1\. What is HRRN?

HRRN selects the process with the highest response value.

Waiting Time = time the process has been waiting in the ready queue.
Burst Time = estimated CPU time needed (in our simple case, we can use a fixed guess).
       
       In xv6, we don’t have explicit burst predictions, 
       so we’ll store:
       When the process became RUNNABLE.
       A fixed or simple guess for burst time (say burst = 5 ticks    
       for all processes).

  2\. Files to edit

 We’ll mainly work in:
- kernel/proc.h → add new fields.
- kernel/proc.c → update scheduler() and track waiting times.

 3\. Add fields for HRRN in proc.h

Open:
          
      nano kernel/proc.h

 
Inside ***struct proc*** (*before the closing brace*):
    
    uint rtime; // total time process has run (ticks)
    uint wtime; // total waiting time in RUNNABLE state
    uint enter_time; // time process last became RUNNABLE

 
4\. Track when a process becomes runnable

In ***kernel/proc.c***, find *wakeup1(struct proc \*p)*:

Right before:
     
     p->state = RUNNABLE;
add:

     p->enter_time = ticks; // record time it became runnable

OR using **clkintr**
  
 5\. Update runtime and waiting time
  We’ll use *scheduler()* to increment these counters.
- In scheduler() before selecting a process:

  // Update wtime and rtime for each process

  for(struct proc \*pp = proc; pp < \&proc\[NPROC]; pp++) {
acquire(\&pp->lock);

  if(pp->state == RUNNING) {
  pp->rtime++;
} 
else if(pp->state == RUNNABLE) {
pp->wtime = ticks - pp->enter\_time;
}

  release(\&pp->lock);
}

  
  
  

6\. Change scheduling to HRRN

Replace the round-robin selection loop in scheduler() with:

    struct proc \*chosen = 0;

    double best\_ratio = -1.0;

     for(struct proc \*pp = proc; pp < \&proc\[NPROC]; pp++) {

       acquire(\&pp->lock);

     if(pp->state == RUNNABLE) {

       int burst = 5; // fixed guess

       double rr = ((double)pp->wtime + burst) / burst;

      if(rr > best\_ratio) {

        best\_ratio = rr;

       if(chosen) release(&chosen->lock);

         chosen = pp;

      continue;

        }

      }

        release(\&pp->lock);
     }

     if(chosen) {

        chosen->state = RUNNING;

        c->proc = chosen;

        swtch(\&c->scheduler, chosen->context);
        switchkvm();

        c->proc = 0;

        release(\&chosen->lock);

       }

*Key differences:*

- We compute RR for each runnable process.
- Pick the one with the highest RR.
- Use a fixed burst time guess.

  
 
 7\. Reset waiting time when scheduled

  When you actually switch to the chosen process, set:

    chosen->wtime = 0;
    chosen->enter\_time = ticks; // reset start time for next wait

8\. Build and run

    make clean && make qemu

Run a few processes and see that short-wait jobs don’t necessarily run first — long-wait jobs will get boosted by HRRN.

## *ALSO WE CAN USE priority scheduling*
- using priority scheduling because it’s simple and easy to demonstrate.

Step-by-step: Add priority scheduling

1️⃣ Add a priority field to each process

 File: 
      
      kernel/proc.h
Find:

    struct proc {

Add inside:

    int priority; // lower value = higher priority

  2️⃣ Initialize default priority

   File:
      
      kernel/proc.c

  

Find 
            
      allocproc() 
and after:

     p->state = USED;

Add:

     p->priority = 10; // default medium priority

3️⃣ Add a system call to set priority

   a) Declare the syscall number
           
   File: 
            
       kernel/syscall.h:

  
\#define SYS\_setpriority <NEXT\_NUMBER>

Replace <NEXT\_NUMBER> with the next unused number after the last one.

b) Add to syscall table

File: 
     
     kernel/syscall.c:

extern uint64 sys\_setpriority(void);

\[SYS\_setpriority] sys\_setpriority,

  
c) Implement the syscall

File:
   
     kernel/sysproc.c:
     
uint64

sys\_setpriority(void)

{

int pr;

if(argint(0, \&pr) < 0)

return -1;

myproc()->priority = pr;

return 0;

}


d) Expose to user space

  File:
     
      user/usys.pl 
 add:

    entry("setpriority");

File:
 
     user/user.h 
 add:

    int setpriority(int);

 
4️⃣ Change the scheduler to use priority

File: *kernel/proc.c* → in scheduler() replace the inner loop that picks a process:

struct proc \*p;

struct proc \*chosen = 0;

for(p = proc; p < \&proc\[NPROC]; p++) {

if(p->state == RUNNABLE) {

if(chosen == 0 || p->priority < chosen->priority) {

chosen = p;

}

}

}

if(chosen) {

p = chosen;

c->proc = p;

switchuvm(p);
p->state = RUNNING;

swtch(\&c->scheduler, p->context);

switchkvm();

c->proc = 0;

}

Now, it always picks the lowest priority number first.

5️⃣ Test in QEMU

Create a test program:

    nano user/testprio.c

  \#include "kernel/types.h"
\#include "user/user.h"

int main(void) {

printf("PID %d starting with priority 5\\n", getpid());
setpriority(5);

for(int i = 0; i < 10; i++) {

printf("PID %d loop %d\\n", getpid(), i);

sleep(10);

}
exit(0);

}

Add 

    UPROGS = \_testprio 

to 
  
    user/Makefile.

 
Rebuild and run:

    make clean && make qemu

 
In xv6 shell:
$ testprio \&
$ testprio \&
$ setpriority 1 // (if you write another small program to set priority of PID 1)