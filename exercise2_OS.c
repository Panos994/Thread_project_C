#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>

#define NUM_THREADS 4
#define FILE_LENGTH 2000

void handle_sigint(int signal);
void handle_sigterm(int signal);

pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

// Global variables
char data[FILE_LENGTH];
int alphabet[26]; // Stores the frequency of each letter
sem_t *sem;

// Function prototypes
void *read_thread(void *arg);

int main(int argc, char **argv)
{
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigterm);
    // Initialize semaphore
    sem = sem_open("/mysemaphore", O_CREAT | O_EXCL, 0600, 1);
    int pid;

    if ((pid = fork()) == -1)
    {
        perror("fork");
        exit(1);
    }
    if (pid != 0)
    {
        // parent
        int fd = open("data.txt", O_RDWR | O_CREAT, 0666);
        if (fd == -1)
        {
            perror("Error opening file");
            exit(1);
        }
        // Write to the file
        srand(time(NULL));
        for (int i = 0; i < FILE_LENGTH; i++)
        {
            data[i] = 'a' + rand() % 26;
        }
            write(fd, data, sizeof(data));

        waitpid(pid, NULL, 0);

        // Close the file
        close(fd);
    }
    else
    {

        sleep(3);

        {
            // This is the second child process
            // Initialize threads
            pthread_t threads[NUM_THREADS];
            int fd;
            if ((fd = open("data.txt", O_RDONLY)) == -1)
            {
                perror("open");
                exit(1);
            }
            for (int i = 0; i < NUM_THREADS; i++)
            {
                pthread_create(&threads[i], NULL, read_thread, &fd);
            }

            // Wait for threads to finish
            for (int i = 0; i < NUM_THREADS; i++)
            {
                pthread_join(threads[i], NULL);
            }

            // Print frequency of each letter
            for (int i = 0; i < 26; i++)
            {
                printf("%c: %d\n", 'a' + i, alphabet[i]);
            }
            close(fd);
        }
    }

    // Unlink semaphore
    sem_unlink("/mysemaphore");

    return 0;
}

void *read_thread(void *arg)
{
    int *fd = arg;
    int b;
    char buff[500];
    // Read portion of file assigned to this thread
    read(*fd, buff, FILE_LENGTH / NUM_THREADS);

    // Increment frequency of each letter in alphabet array
    for (int i = 0; i < FILE_LENGTH / NUM_THREADS; i++)
    {
        if (buff[i] >= 'a' && buff[i] <= 'z')
        {
            pthread_mutex_lock(&file_mutex);


	 sem_wait(sem); //Acquire semaphore
       

         
         
           alphabet[buff[i] - 'a']++;
            pthread_mutex_unlock(&file_mutex);
	 sem_post(sem); //release semaphore


        }
    }

    pthread_exit(NULL);
}

void handle_sigint(int sig)
{
    printf("SIGINT received, cleaning up and exiting...\n");
    // Perform cleanup tasks here
    // Unlink semaphore
    sem_unlink("/mysemaphore");
    exit(0);
}

void handle_sigterm(int sig)
{
    printf("SIGTERM received, cleaning up and exiting...\n");
    // Perform cleanup tasks here
    // Unlink semaphore
    sem_unlink("/mysemaphore");
    exit(0);
}
