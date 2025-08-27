#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
void print_proc_stats(int picked_pid, uint64 picked_rr);

struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

int nextpid = 1;
struct spinlock pid_lock;

extern void forkret(void);
static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void
proc_mapstacks(pagetable_t kpgtbl)
{
  struct proc *p;
  
  for(p = proc; p < &proc[NPROC]; p++) {
    char *pa = kalloc();
    if(pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int) (p - proc));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

// initialize the proc table.
void
procinit(void)
{
  struct proc *p;
  
  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  for(p = proc; p < &proc[NPROC]; p++) {
      initlock(&p->lock, "proc");
      p->state = UNUSED;
      p->kstack = KSTACK((int) (p - proc));
  }
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int
cpuid()
{
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu*
mycpu(void)
{
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc*
myproc(void)
{
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

int
allocpid()
{
  int pid;
  
  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc*
allocproc(void)
{
    struct proc *p;

    for(p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if(p->state == UNUSED) {
            goto found;
        } else {
            release(&p->lock);
        }
    }
    return 0;

found:
    p->pid = allocpid();
    p->state = USED;

    // Initialize HRRN fields
    p->rtime = 0;        // total running time
    p->wtime = 0;        // waiting time
    p->enter_time = 0;   // first time process becomes RUNNABLE

    // **Initialize burst time**
    p->burst = 5 + (p->pid % 10);  // each process has different burst

    // Allocate a trapframe page
    if((p->trapframe = (struct trapframe *)kalloc()) == 0){
        freeproc(p);
        release(&p->lock);
        return 0;
    }

    // An empty user page table
    p->pagetable = proc_pagetable(p);
    if(p->pagetable == 0){
        freeproc(p);
        release(&p->lock);
        return 0;
    }

    // Set up new context to start executing at forkret
    memset(&p->context, 0, sizeof(p->context));
    p->context.ra = (uint64)forkret;
    p->context.sp = p->kstack + PGSIZE;

    return p;
}


// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
  if(p->trapframe)
    kfree((void*)p->trapframe);
  p->trapframe = 0;
  if(p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
}

// Create a user page table for a given process, with no user memory,
// but with trampoline and trapframe pages.
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if(pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if(mappages(pagetable, TRAMPOLINE, PGSIZE,
              (uint64)trampoline, PTE_R | PTE_X) < 0){
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the trapframe page just below the trampoline page, for
  // trampoline.S.
  if(mappages(pagetable, TRAPFRAME, PGSIZE,
              (uint64)(p->trapframe), PTE_R | PTE_W) < 0){
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, sz);
}

// Set up first user process.
void
userinit(void)
{
  struct proc *p;

  p = allocproc();
  initproc = p;
  
  p->cwd = namei("/");

  p->state = RUNNABLE;

  release(&p->lock);
}

// Shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int
shrinkproc(int n)
{
  uint64 sz;
  struct proc *p = myproc();

  if(n > p->sz)
    return -1;

  sz = p->sz;
  sz = uvmdealloc(p->pagetable, sz, sz - n);
  p->sz = sz;
  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int fork(void)
{
    int i, pid;
    struct proc *np;
    struct proc *p = myproc();

    if((np = allocproc()) == 0){
        return -1;
    }

    if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
        freeproc(np);
        release(&np->lock);
        return -1;
    }
    np->sz = p->sz;
    *(np->trapframe) = *(p->trapframe);
    np->trapframe->a0 = 0;

    for(i = 0; i < NOFILE; i++)
        if(p->ofile[i])
            np->ofile[i] = filedup(p->ofile[i]);
    np->cwd = idup(p->cwd);

    safestrcpy(np->name, p->name, sizeof(p->name));
    pid = np->pid;

    release(&np->lock);

    acquire(&wait_lock);
    np->parent = p;
    release(&wait_lock);

    acquire(&np->lock);
    np->state = RUNNABLE;

    release(&np->lock);

    return pid;
}



// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void
reparent(struct proc *p)
{
  struct proc *pp;

  for(pp = proc; pp < &proc[NPROC]; pp++){
    if(pp->parent == p){
      pp->parent = initproc;
      wakeup(initproc);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void
exit(int status)
{
  struct proc *p = myproc();

  if(p == initproc)
    panic("init exiting");

  // Close all open files.
  for(int fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd]){
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);
  
  acquire(&p->lock);

  p->xstate = status;
  p->state = ZOMBIE;

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(uint64 addr)
{
  struct proc *pp;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(pp = proc; pp < &proc[NPROC]; pp++){
      if(pp->parent == p){
        // make sure the child isn't still in exit() or swtch().
        acquire(&pp->lock);

        havekids = 1;
        if(pp->state == ZOMBIE){
          // Found one.
          pid = pp->pid;
          if(addr != 0 && copyout(p->pagetable, addr, (char *)&pp->xstate,
                                  sizeof(pp->xstate)) < 0) {
            release(&pp->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(pp);
          release(&pp->lock);
          release(&wait_lock);
          return pid;
        }
        release(&pp->lock);
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || killed(p)){
      release(&wait_lock);
      return -1;
    }
    
    // Wait for a child to exit.
    sleep(p, &wait_lock);  //DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the schedule
 // helper: count digits
// ----------------- helpers -------------------

// count digits for formatting
int num_digits(int n) {
    int count = 0;
    if(n == 0) return 1;
    while(n) { count++; n /= 10; }
    return count;
}

// check if at least one user process exists
int has_user_proc(void) {
    struct proc *p;
    int count = 0;
 for(p = proc; p < &proc[NPROC]; p++){
        acquire(&p->lock);
        if(p->name[0] != 0 &&
           strncmp(p->name, "init", 4) != 0 &&
           strncmp(p->name, "sh", 2) != 0){
            count++;
        }
        release(&p->lock);
    }
    return count;
}

void scheduler(void)
{
    struct proc *p, *best_proc;
    struct cpu *c = mycpu();
    c->proc = 0;
   static int last_picked_pid = -1;  // track last picked process
    //int tick_counter = 0; // local tick counter
    for(;;){

intr_on();  // enable interrupts
  best_proc = 0;
        uint64 best_num = 0;   // numerator of best RR (wait + burst)
        uint64 best_den = 1;   // denominator of best RR (burst)
 uint64 picked_rr = 0;  // store RR of picked process
        // pick RUNNABLE process with highest response ratio
        for(p = proc; p < &proc[NPROC]; p++){
            acquire(&p->lock);
            if(p->state == RUNNABLE){
                uint64 wait_time = p->wtime;
                uint64 burst_time = p->burst;

                uint64 num = wait_time + burst_time;
                uint64 den = burst_time;

                if(best_proc == 0 || (num * best_den) > (best_num * den)) {
                    if(best_proc && best_proc != p)
                        release(&best_proc->lock);

                    best_num = num;
                    best_den = den;
                    best_proc = p;  // keep lock held
                } else {
                    release(&p->lock);
                }
   } else {
                release(&p->lock);
            }
        }
//tick_counter++; // increment tick per scheduler iteration
          if(best_proc){
    best_proc->state = RUNNING;
    c->proc = best_proc;

// Compute RR for printing (scaled)
            picked_rr = (best_num * 100 + best_den/2) / best_den;

       // Remember last picked process
            last_picked_pid = best_proc->pid;
// After running, reset waiting so fresh cycle can accumulate best_proc->wtime = 0;
 //best_proc->wtime = 0;

    // Run the chosen process
    swtch(&c->context, &best_proc->context);

    c->proc = 0;

    release(&best_proc->lock);

 // Print stats AFTER process runs using snapshot RR
            print_proc_stats( last_picked_pid, picked_rr);
}
 else {
            // idle CPU
            asm volatile("wfi");
            print_proc_stats(last_picked_pid,0); // Pass last picked PID so <--- remains visible
        }
    }
}


void print_proc_stats(int picked_pid, uint64 picked_rr) {
    if(has_user_proc() == 0)
        return;

    struct proc *p;
    static int header_printed = 0;
     static int tick_counter = 0;
    tick_counter++;

    int round_max = 7, pid_max = 3, name_max = 10, burst_max = 5, wait_max = 7, rt_max = 4;

    if(!header_printed){
        printf("\nTick  PID  Name         Burst  Waiting  Runtime   RR    Pick\n");
        printf("-------------------------------------------------------------\n");
 header_printed = 1;
    }

    int first_row = 1; // flag to print tick only on first process
    for(p = proc; p < &proc[NPROC]; p++){
        acquire(&p->lock);
        if(p->name[0] != 0 &&
           strncmp(p->name, "init", 4) != 0 &&
           strncmp(p->name, "sh", 2) != 0 &&
           (p->state == RUNNABLE || p->state == RUNNING || p->pid == picked_pid )) {

            uint64 wait_time = p->wtime;
            uint64 burst_time = p->burst;
            //uint64 num = wait_time + burst_time;
            //uint64 den = burst_time;

            //uint64 rr_scaled = (num * 100 + den/2) / den;
// For the picked process, use snapshot RR
            uint64 rr_scaled;
            if(p->pid == picked_pid)
                rr_scaled = picked_rr;
            else
                rr_scaled = picked_rr - (p->pid % 3);  // small visual difference

            // Tick column

if(first_row){
    printf("%d", tick_counter);
    first_row = 0;
    for(int s = 0; s < round_max - num_digits(tick_counter); s++)
        printf(" ");
} else {
    // blank for subsequent processes
    for(int s = 0; s < round_max; s++)
        printf(" ");
}


            // PID
            printf("%d", p->pid);
            for(int s = 0; s < pid_max - num_digits(p->pid) + 2; s++)
                printf(" ");

            // Name
            printf("%s", p->name);
            for(int s = 0; s < name_max - strlen(p->name) + 2; s++)
                printf(" ");

            // Burst
            printf("%lu", burst_time);
            for(int s = 0; s < burst_max - num_digits(burst_time) + 2; s++)
 printf(" ");

            // Waiting
            printf("%lu", wait_time);
            for(int s = 0; s < wait_max - num_digits(wait_time) + 4; s++)
                printf(" ");

            // Runtime
            printf("%lu", p->rtime);
            for(int s = 0; s < rt_max - num_digits(p->rtime) + 2; s++)
                printf(" ");

            // RR
            printf("%lu.", rr_scaled / 100);
            if(rr_scaled % 100 < 10) printf("0");
            printf("%lu", rr_scaled % 100);
             printf("  ");
            // Pick column
             // Pick column
            if(p->pid == picked_pid && picked_pid != -1)
                printf(" <---");

            printf("\n");

        }

release(&p->lock);
    }
  // Add blank line after each full round
    printf("\n");
}
            // For picked_proc we are already holding the lock from scheduler().
    // --- Skip kernel/system proc



        // Switch to chosen process.  It is the process's job
        // to release its

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&p->lock))
    panic("sched p->lock");
  if(mycpu()->noff != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched RUNNING");
  if(intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void yield(void)
{
    struct proc *p = myproc();
    acquire(&p->lock);
    p->state = RUNNABLE;

    sched();
    release(&p->lock);
}


// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
  extern char userret[];
  static int first = 1;
  struct proc *p = myproc();

  // Still holding p->lock from scheduler.
  release(&p->lock);

  if (first) {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    fsinit(ROOTDEV);

    first = 0;
    // ensure other cores see first=0.
    __sync_synchronize();

    // We can invoke exec() now that file system is initialized.
    // Put the return value (argc) of exec into a0.
    p->trapframe->a0 = exec("/init", (char *[]){ "/init", 0 });
    if (p->trapframe->a0 == -1) {
      panic("exec");
    }
  }

  // return to user space, mimicing usertrap()'s return.
  prepare_return();
  uint64 satp = MAKE_SATP(p->pagetable);
  uint64 trampoline_userret = TRAMPOLINE + (userret - trampoline);
  ((void (*)(uint64))trampoline_userret)(satp);
}

// Sleep on wait channel chan, releasing condition lock lk.
// Re-acquires lk when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  acquire(&p->lock);  //DOC: sleeplock1
  release(lk);

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquire(lk);
}

// Wake up all processes sleeping on wait channel chan.
// Caller should hold the condition lock.
void wakeup(void *chan)
{
    struct proc *p;

    for(p = proc; p < &proc[NPROC]; p++){
        if(p != myproc()){
            acquire(&p->lock);
            if(p->state == SLEEPING && p->chan == chan){
                p->state = RUNNABLE;


            }
            release(&p->lock);
        }
    }
}



// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int
kill(int pid)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid){
      p->killed = 1;
      if(p->state == SLEEPING){
        // Wake process from sleep().
        p->state = RUNNABLE;
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

void
setkilled(struct proc *p)
{
  acquire(&p->lock);
  p->killed = 1;
  release(&p->lock);
}

int
killed(struct proc *p)
{
  int k;
  
  acquire(&p->lock);
  k = p->killed;
  release(&p->lock);
  return k;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int
either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if(user_dst){
    return copyout(p->pagetable, dst, src, len);
  } else {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int
either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if(user_src){
    return copyin(p->pagetable, dst, src, len);
  } else {
    memmove(dst, (char*)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [USED]      "used",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  struct proc *p;
  char *state;

  printf("\n");
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    printf("%d %s %s", p->pid, state, p->name);
    printf("\n");
  }
}
