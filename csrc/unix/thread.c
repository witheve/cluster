#include <unix_internal.h>
#include <pthread.h>
#include <signal.h>

#define MAX_THREADS 10

volatile u64 thread_count;
// xxx - these should definiately* be cache line aligned
struct context contexts[MAX_THREADS];

static void *start_thread(void *z)
{
    context c = z;
    pthread_setspecific(pkey, c);
    register_read_handler(c->s, c->wakeup[0], c->self);
    prf("thread started\n");
    apply(c->start);
    unix_wait();
    return 0;
}

context thread_init(heap page_heap, thunk start)
{
    context c = init_context(page_heap);
    c->start = start;
    pthread_create(&c->p, 0, start_thread, c);
    return c;
}

static context io_thread = 0;

static CONTINUATION_4_0(io_write, descriptor, buffer, tid, thunk);

static void io_write(descriptor d, buffer b, tid result, thunk t)
{
    write(d, bref(b, 0), buffer_length(b));
    thread_send(result, t);
}

void asynch_write(descriptor d, buffer b, thunk finished)
{
    if (!io_thread)
        io_thread = thread_init(pages, ignore);

    schedule_remote(io_thread->myself, cont(init, io_write, d, b, tcontext()->myself, finished));
}

static CONTINUATION_1_0(check_queues, context);
static void check_queues(context c)
{
    tid myself = tcontext()->myself;
    for (int i = 0; i < thread_count; i++) {
        thunk t;
        if ((i != myself) && (t = qpoll(c->queues+i)))
            apply(t);
    }
    unsigned char z;
    read(c->wakeup[0], &z, 1);
    register_read_handler(c->s, c->wakeup[0], c->self);
}

void schedule_remote(tid target, thunk t)
{
    tid myself = tcontext()->myself;
    enq(contexts[target].queues+myself, t);
    // should use atomics to coalesce these writes to ourselves
    write(contexts[target].wakeup[1], ".", 1);
}

// pages now threadsafe
context init_context(heap page_allocator)
{
    // no heap name because init dep issues
    heap h = allocate_rolling(page_allocator, 0);
    h->name = wrap_buffer(h, "init", 4);
    tid myself =  fetch_and_add(&thread_count, 1);
    context c = contexts + myself;
    signal(SIGPIPE, SIG_IGN);
    // put a per thread freelist on top of
    c->h = h;
    c->myself = myself;
    c->page_heap = page_allocator;
    c->s = select_init(h);
    c->short_lived = allocate_rolling(c->page_heap, wrap_buffer(h, "transient", 9));
    c->t = initialize_timers(allocate_rolling(page_allocator, wrap_buffer(h, "transient", 6)));
    // xxx - allocation scheme for these queue sets
    c->queues = allocate(h, 10 * sizeof(queue));

    memset(c->queues, 0, 10 * sizeof(queue));
    for(int i = 0 ; i < myself; i++) {
        c->queues[i] = allocate_queue(h, 8);
        contexts[i].queues[myself] = allocate_queue(h, 8);
    }
    c->self = cont(h, check_queues, c);
    pipe(c->wakeup);
    return c;
}
