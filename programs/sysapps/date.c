#include <stdio.h>
#include <time.h>

int main(int argc, char *argv[]) {
    char date[64];
    
    time_t curtime = time(NULL);
    strftime(date, sizeof(date), "%c", localtime(&curtime));
    puts(date);
    return 0;
}
