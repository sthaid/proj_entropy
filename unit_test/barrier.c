#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>

// -----------------  PTHREAD BARRIER IMPLEMENTATION  ----------------------

#define pthread_barrier_init        Pthread_barrier_init
#define pthread_barrier_destroy     Pthread_barrier_destroy
#define pthread_barrier_wait        Pthread_barrier_wait
#define pthread_barrier_t           Pthread_barrier_t
#define pthread_barrierattr_t       Pthread_barrierattr_t

typedef void * Pthread_barrierattr_t;
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    uint64_t count;
    uint64_t current;;
} Pthread_barrier_t;

int Pthread_barrier_wait(Pthread_barrier_t *barrier);
int Pthread_barrier_destroy(Pthread_barrier_t *barrier);
int Pthread_barrier_init(Pthread_barrier_t *barrier, const Pthread_barrierattr_t *attr, unsigned count);

int Pthread_barrier_init(Pthread_barrier_t *barrier,
    const Pthread_barrierattr_t *attr, unsigned count)
{
    pthread_mutex_init(&barrier->mutex, NULL);
    pthread_cond_init(&barrier->cond, NULL);
    barrier->count = count;
    barrier->current = 0;
    return 0;
}

int Pthread_barrier_wait(Pthread_barrier_t *barrier)
{
    uint64_t done;

    pthread_mutex_lock(&barrier->mutex);
    done = barrier->current / barrier->count + 1;
    barrier->current++;
    if ((barrier->current % barrier->count) == 0) {
        pthread_cond_broadcast(&barrier->cond);
    } else {
        do {
            pthread_cond_wait(&barrier->cond, &barrier->mutex);
        } while (barrier->current / barrier->count != done);
    }
    pthread_mutex_unlock(&barrier->mutex);
    return 0;
}

int Pthread_barrier_destroy(Pthread_barrier_t *barrier)
{
    // xxx should delete the mutex and cond
    return 0;
}

// -----------------  PTHREAD BARRIER TEST  --------------------------------

pthread_barrier_t b;

int max_thread=4;
int64_t global_count;
void *  thread(void * cx);

int main()
{
    int i;
    pthread_t thread_id;
    struct rlimit rl;

    // init core dumps
    rl.rlim_cur = RLIM_INFINITY;
    rl.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &rl);

    pthread_barrier_init(&b,NULL,max_thread);

    for (i = 0; i < max_thread; i++) {
        pthread_create(&thread_id, NULL, thread, (void*)(int64_t)i);
    }

    pause();

    return 0;
}


void *  thread(void * cx)
{
    int id = (int64_t)cx;
    int64_t my_count = 0;

    printf("STARTING %d\n", id);
    pthread_barrier_wait(&b);

    while (true) {
        pthread_barrier_wait(&b);

        if (id == 0) {
            global_count++;
            if ((global_count % 100000) == 0) {
                printf("%ld\n", global_count);
            }
        }

        pthread_barrier_wait(&b);

        my_count++;

        if (my_count != global_count) {
            printf("ERROR my_count %ld  global_count %ld\n", my_count, global_count);
            exit(1);
        }
    }

    return NULL;
}

