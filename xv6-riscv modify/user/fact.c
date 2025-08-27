#include "kernel/types.h"
#include "user/user.h"

int main() {
    //int pid = getpid();
    int sum = 0;

    // Compute sum with CPU-consuming busy loop
    for(int i = 1; i <= 500; i++){
        sum += i;
           for(int j=0;j<8500000;j++);
    }

    // Print the result
   //printf("PID %d: Sum of 1 to 300 is %d\n", pid, sum);

    // Small delay to avoid interleaving with scheduler table
       //sleep(40); // adjust as needed

    exit(0);
}





