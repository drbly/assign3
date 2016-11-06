#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

volatile int *mem;
sem_t *sem;

void work ( int me )
{
    int i, n;
    n = 10000;
    for ( i = 0; i < n; i++ ) {
        sem_wait ( sem );
        *mem = *mem + 1;
        sem_post ( sem );
        //usleep ( 100 );
    }
}

int main ( int argc, char **argv )
{
    int i, pid, kids, shmfd;

    kids = argc > 1 ? atoi(argv[1]) : 5;
    shmfd = shm_open ( "/brs_memory", O_RDWR | O_CREAT, 0666 );
    if ( shmfd < 0 ) {
        fprintf(stderr,"Could not create brs_memory\n");
        exit(1);
    }
    ftruncate ( shmfd, sizeof(int) );
    mem = (int *)mmap ( NULL, kids*sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0 );
    if ( mem < 0 ) {
        fprintf(stderr,"Could not map brs_memory\n");
        exit(1);
    }
    close ( shmfd );
    shm_unlink ( "/brs_memory" );

    sem = sem_open ( "brs_sem", O_CREAT, 0666, 1 );
    if ( sem == NULL ) {
        fprintf(stderr,"Could not create brs semaphore\n");
        exit(1);
    }
    sem_unlink ( "brs_sem" );
    mem[0] = 0;
    for ( i = 0; i < kids; i++ ) {
        pid = fork();
        if ( pid < 0 ) {
            fprintf(stderr,"fork failed at %d\n",i);
            exit(1);
        } else if ( pid > 0 ) {
            printf("parent: new child is %d\n",pid);
        } else {
            printf("child %d, parent is %d\n",i, getppid());
            work(i);
            exit(0);
        }
    } 
    for ( i = 0; i < kids; i++ ) wait(NULL);
    printf("mem[0] = %d\n",mem[0]);
    return 0;
}
