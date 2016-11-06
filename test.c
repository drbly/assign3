#include <sys/times.h>
#include<stdlib.h>
#include<stdio.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>

double *a, *b, *c;
sem_t *sem;

int main(void)
{
	int i, j, k, ii, jj, kk, n, blockSize;
	int x = 1;
	int shmfd;
	
	pid_t childpid;
	int counter = 0;

	printf("Enter the value of n: ");
	scanf("%d", &n);
	printf("\n\n");

	blockSize = n / 2;

	//---------A---------
	shmfd = shm_open("/blydr_memory", O_RDWR | O_CREAT, 0666);
	if (shmfd < 0) {
		fprintf(stderr, "Could not create blydr_memory\n");
		exit(1);
	}
	ftruncate(shmfd, n*n * sizeof(double));
	a = (double *)mmap(NULL, n*n * sizeof(double), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
	if (a == NULL) {
		fprintf(stderr, "Could not map blydr_memory\n");
		exit(1);
	}
	close(shmfd);
	shm_unlink("/blydr_memory");



	//---------B---------
	shmfd = shm_open("/blydr_memory", O_RDWR | O_CREAT, 0666);
	if (shmfd < 0) {
		fprintf(stderr, "Could not create blydr_memory\n");
		exit(1);
	}
	ftruncate(shmfd, n*n * sizeof(double));
	b = (double *)mmap(NULL, n*n * sizeof(double), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
	if (b == NULL) {
		fprintf(stderr, "Could not map blydr_memory\n");
		exit(1);
	}
	close(shmfd);
	shm_unlink("/blydr_memory");



	//---------C---------
	shmfd = shm_open("/blydrC_memory", O_RDWR | O_CREAT, 0666);
	if (shmfd < 0) {
		fprintf(stderr, "Could not create blydr_memory\n");
		exit(1);
	}
	ftruncate(shmfd, n*n * sizeof(double));
	c = (double *)mmap(NULL, n*n * sizeof(double), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
	if (c == NULL) {
		fprintf(stderr, "Could not map blydr_memory\n");
		exit(1);
	}
	close(shmfd);
	shm_unlink("/blydr_memory");

	sem = sem_open("blydr_sem", O_CREAT, 0666, 1);
	if (sem == NULL) {
		fprintf(stderr, "Could not create blydr semaphore\n");
		exit(1);
	}
	sem_unlink("blydr_sem");

	//allocate arrays
	a = (double*)malloc(n*n * sizeof(double));
	b = (double*)malloc(n*n * sizeof(double));
	c = (double*)malloc(n*n * sizeof(double));

	/*
	for (i = 0; i<n; i++)
	{
		a[i] = malloc(sizeof(double)*n);
		b[i] = malloc(sizeof(double)*n);
		c[i] = malloc(sizeof(double)*n);
	}
	*/
	//populate arrays
	for (i = 0; i<n; i++)
	{
		x += 3;
		for (j = 0; j<n; j++)
			a[i *n + j] = x + (3 * j * i);
	}
	x = 1;
	for (i = 0; i<n; i++)
	{
		x += 3;
		for (j = 0; j<n; j++)
			b[i *n + j] = x + (j * 2 * i);
	}

	//print arrays
	printf("----A----");
	for (i = 0; i<n; i++)
	{
		printf("\n[%d]  ", i);
		for (j = 0; j < n; j++)
			printf("[%d][%d] (%4f)   ", i, j, a[i *n + j]);
	}
	printf("\n");

	printf("----B----");
	for (i = 0; i<n; i++)
	{
		printf("\n[%d]  ", i);
		for (j = 0; j < n; j++)
			printf("[%d][%d] (%4f)   ", i, j, b[i *n + j]);
	}
	printf("\n");
	

	//block multiplication
	

	for (ii = 0; ii < n; ii += blockSize) {
		for (jj = 0; jj < n; jj += blockSize) {
			for (kk = 0; kk < n; kk += blockSize) {

				counter++;
				printf("counter: %d\n", counter);
				childpid = fork();

				if (childpid >= 0) {
					if (childpid == 0) {  //child process
						printf("child processing...\n");

						for (i = ii; i < ii + (blockSize); i++) {
							for (j = jj; j < jj + (blockSize); j++) {
								for (k = kk; k < kk + (blockSize); k++) {

									sem_wait(sem);

									if (i == 0 && j == 0) {
										printf("c[%d][%d](%f) + a[%d][%d](%f) * b[%d][%d](%f)\n", i, j, c[i *n + j], i, k, a[i *n + k], k, j, b[k *n + j]);
									}

									c[i *n + j] = c[i *n + j] + (a[i *n + k] * b[k *n + j]);

									if (i == 0 && j == 0) {
										printf("c[%d][%d](%f)\n", i, j, c[i *n + j]);
									}
									
									sem_post(sem);

								}
							}
						}
						exit(0);
					}
					else if (childpid > 0) {  //parent
						//do nothing
					}

				}
				else {
					perror("what the fork happened\n");
					counter--;
				}
			}
		}
	}

	printf("counter at loop: %d\n", counter);

	for (i = 0; x < counter; i++) {
		wait(NULL);
	}

	//print output array
	printf("----A----");
	for (i = 0; i<n; i++)
	{
		printf("\n[%d]  ", i);
		for (j = 0; j < n; j++)
			printf("[%d][%d] (%2f)   ", i, j, a[i *n + j]);
	}
	printf("\n");

	printf("----B----");
	for (i = 0; i<n; i++)
	{
		printf("\n[%d]  ", i);
		for (j = 0; j < n; j++)
			printf("[%d][%d] (%2f)   ", i, j, b[i *n + j]);
	}
	printf("\n");

	printf("----C----");
	for (i = 0; i<n; i++)
	{
		printf("\n[%d]  ", i);
		for (j = 0; j < n; j++)
			printf("[%d][%d] (%2f)   ", i, j, c[i *n + j]);
	}
	printf("\n");

	return (0);
}