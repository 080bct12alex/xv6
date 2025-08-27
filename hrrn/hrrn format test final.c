#include <stdio.h>
#include <string.h>

int num_digits(int n) {
    int count = 0;
    if(n == 0) return 1;
    while(n) { count++; n /= 10; }
    return count;
}

int main() {
    int pids[]     = {45, 678, 88 , 8};
    char *names[]  = {"sum", "fact", "fib","i am happy"};
    int burst[]    = {10, 100, 1,5};
    int waiting[]  = {100, 0, 100,5};
    int runtime[]  = {100, 100, 0,5};
    double rr[]    = {1.00, 1.00, 1.00,1.75};
    int n = 4;
    
    int round_max = 7;   // width of Round column
    int pid_max = 3;
    int name_max = 10;
    int burst_max = 5;
    int wait_max = 7;
    int rt_max = 4;

    printf("\nRound  PID  Name       Burst  Waiting  Runtime  RR    Pick\n");
    printf("---------------------------------------------------------\n");
for(int j = 0; j < 3; j++){
    for(int i = 0; i < n; i++){
             // Round (manual spacing)
        if(i == 0) {
            printf("100");  // tick number
            for(int s = 0; s < round_max - num_digits(100); s++) printf(" ");
        } else {
            for(int s = 0; s < round_max; s++) printf(" "); // blank for same tick
        }

        // PID
        printf("%d", pids[i]);
        for(int s = 0; s < pid_max - num_digits(pids[i]) + 2; s++) printf(" ");

        // Name
        printf("%s", names[i]);
        for(int s = 0; s < name_max - strlen(names[i]) + 2; s++) printf(" ");

        // Burst
        printf("%d", burst[i]);
        for(int s = 0; s < burst_max - num_digits(burst[i]) + 2; s++) printf(" ");

        // Waiting
        printf("%d", waiting[i]);
        for(int s = 0; s < wait_max - num_digits(waiting[i]) + 4; s++) printf(" ");

        // Runtime
        printf("%d", runtime[i]);
        for(int s = 0; s < rt_max - num_digits(runtime[i]) + 2; s++) printf(" ");

        // RR
        printf("%.2f", rr[i]);
        printf(" "); 

        // Pick (mark first process)
        if(i == 0) printf("  <---");

        printf("\n");
        
    }
    printf("\n");
}

    return 0;
}
