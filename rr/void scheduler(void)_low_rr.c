void
scheduler(void)
{
    struct proc *p;
    struct cpu *c = mycpu();
    c->proc = 0;

    for(;;){
        intr_on();           // Enable interrupts
        intr_off();          // Disable temporarily to protect proc list

        int found = 0;
        for(p = proc; p < &proc[NPROC]; p++) {
            acquire(&p->lock);

            if(p->state == RUNNABLE) {

                // Switch to the process
                p->state = RUNNING;
                c->proc = p;
                swtch(&c->context, &p->context);

                // Update runtime after CPU slice
                p->ttime++;
                c->proc = 0;
                found = 1;

                // Safe waiting time calculation
                if (ticks >= p->ctime + p->ttime)
                    p->waiting_time = ticks - p->ctime - p->ttime;
                else
                    p->waiting_time = 0;  // avoid negative values

                // Print info only for user processes (skip init and sh)
                if(p->name[0] != 0 &&
                   strncmp(p->name, "init", 4) != 0 &&
                   strncmp(p->name, "sh", 2) != 0)
                {
                    printf("PID: %d Name: %s Waiting: %lu Runtime: %lu\n",
       p->pid, p->name, p->waiting_time, p->ttime);

                }
            }

            release(&p->lock);
        }

        if(found == 0){
            // Nothing runnable, wait for interrupt (like timer tick)
            asm volatile("wfi");
        }
    }
}