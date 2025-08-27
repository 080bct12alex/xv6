// helper: count digits
int num_digits(int n) {
    int count = 0;
    if(n == 0) return 1;
    while(n) { count++; n /= 10; }
    return count;
}

// helper: check if there is at least one user process
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

// helper: print user process stats table
void print_proc_stats(void) {
    if(has_user_proc() == 0)
        return;  // no user processes, skip printing

    struct proc *p;
    static int header_printed = 0;
    int pid_max = 3, name_max = 12, wait_max = 10;

    if(!header_printed){
        printf("\nPID  Name         Waiting  Runtime\n");
        printf("----------------------------------\n");
        header_printed = 1;
    }

    for(p = proc; p < &proc[NPROC]; p++){
        acquire(&p->lock);
        if(p->name[0] != 0 &&
           strncmp(p->name, "init", 4) != 0 &&
           strncmp(p->name, "sh", 2) != 0 &&
           (p->state == RUNNABLE || p->state == RUNNING)) {

            uint64 wait_time = (ticks >= p->ctime + p->ttime)
                               ? ticks - p->ctime - p->ttime
                               : 0;

            // PID
            printf("%d", p->pid);
            for(int s = 0; s < pid_max - num_digits(p->pid) + 2; s++)
                printf(" ");

            // Name
            printf("%s", p->name);
            for(int s = 0; s < name_max - strlen(p->name) + 2; s++)
                printf(" ");

            // Waiting
            printf("%lu", wait_time);
            for(int s = 0; s < wait_max - num_digits(wait_time) + 2; s++)
                printf(" ");

            // Runtime
            printf("%lu\n", p->ttime);
        }
        release(&p->lock);
    }
}

void scheduler(void)
{
    struct proc *p;
    struct cpu *c = mycpu();
    c->proc = 0;

    for(;;){
        intr_on();
        intr_off();

        int found = 0;
        for(p = proc; p < &proc[NPROC]; p++){
            acquire(&p->lock);
            if(p->state == RUNNABLE){
                p->state = RUNNING;
                c->proc = p;

                swtch(&c->context, &p->context);

                p->ttime++;  // increment runtime

                c->proc = 0;
                found = 1;
            }
            release(&p->lock);
        }

        // Only print table if user process exists
        print_proc_stats();

        if(found == 0){
            asm volatile("wfi");
        }
    }
}
