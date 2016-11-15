// would be nice to abstract this further away from pthreads
#include <pthread.h>

extern struct context *primary;
typedef u64 tid;
extern volatile tid thread_count;

typedef struct context {
    tid myself;
    timers t;
    heap page_heap;
    heap h;
    heap short_lived;
    selector s;
    queue *queues;
    thunk self;
    // pipe per queue? queue as pipe?
    descriptor wakeup[2];
    pthread_t p;
    thunk start;
} *context;

context init_context();

// i'd really prefer not to include this everywhere, but it seems
// stupid to fight pthreads about maintaining tls
#include <pthread.h>

extern pthread_key_t pkey;
#define tcontext() ((context)pthread_getspecific(pkey))
#define transient (tcontext()->short_lived)
context thread_init(heap page_heap, thunk start);
void schedule_remote(tid target, thunk t);
