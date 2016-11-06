#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

int *mem;

int main ( int argc, char **argv )
{
    int i, pid, kids, shmfd;

    kids = argc > 1 ? atoi(argv[1]) : 5;
    shmfd = shm_open ( "/brs_memory", O_RDWR | O_CREAT, 0666 );
    if ( shmfd < 0 ) {
        fprintf(stderr,"Could not create brs_memory\n");
        exit(1);
    }
    ftruncate ( shmfd, kids*sizeof(int) );
    mem = (int *)mmap ( NULL, kids*sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0 );
    if ( mem == NULL ) {
        fprintf(stderr,"Could not map brs_memory\n");
        exit(1);
    }
    close ( shmfd );
    shm_unlink ( "/brs_memory" );

    for ( i = 0; i < kids; i++ ) mem[i] = i+1;
    for ( i = 0; i < kids; i++ ) {
        pid = fork();
        if ( pid < 0 ) {
            fprintf(stderr,"fork failed at %d\n",i);
            exit(1);
        } else if ( pid > 0 ) {
            printf("parent: new child is %d\n",pid);
        } else {
            usleep ( 1000 );
            printf("child %d, parent is %d\n",i, getppid());
            mem[i] = mem[i] * mem[i];
            exit(0);
        }
    } 
    for ( i = 0; i < kids; i++ ) wait(NULL);
    for ( i = 0; i < kids; i++ ) printf("mem[%d] = %d\n",i,mem[i]);
    return 0;
}
