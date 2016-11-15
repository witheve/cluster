#include <runtime.h>
#include <http/http.h>

// should be CAS2 maintained (doubly linked), or maybe lazy?
heap heap_list = 0;

thunk ignore;
static CONTINUATION_0_0(ignoro);
static void ignoro(){}

void heap_report()
{
    for ( heap i = heap_list; i ; i=i->next) {
        prf ("%b %dk\n", i->name?i->name: sstring("init"), i->allocated/1024);
    }
}


struct context *primary;
pthread_key_t pkey;

void init_runtime()
{
    // bootstrap
    heap trash = init_memory(4096);

    // xxx - move to core
    heap page_allocator = init_fixed_page_region(trash, allocation_space, allocation_space + region_size, 65536, false);
    pthread_key_create(&pkey, 0);
    primary = init_context(page_allocator);
    pthread_setspecific(pkey, primary);
    register_read_handler(primary->s, primary->wakeup[0], primary->self);
    ignore = cont(init, ignoro);

    init_estring();
    init_uuid();
    init_processes();
    init_station();

    float_heap = allocate_rolling(init_fixed_page_region(init,
                                                         float_space,
                                                         float_space + region_size,
                                                         pages->pagesize, false),
                                  sstring("efloat"));
}
