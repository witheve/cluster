#include <runtime.h>

static uuid bag_bag_id;

void merge_scan(vector scopes, int sig, listener result, value e, value a, value v)
{
#if 0
    vector_foreach(scopes, u) {
        bag b = table_find(ev->t_input, u);
        if(!b) continue;
        apply(b->scan, sig,
              cont(ev->working, shadow_p_by_t_and_f, ev, result),
              e, a, v);
    }

    multibag_foreach(ev->t_solution, u, b)
        apply(((bag)b)->scan, sig,
              cont(ev->working, shadow_t_by_f, ev, result),
              e, a, v);

    multibag_foreach(ev->last_f_solution, u, b)
        apply(((bag)b)->scan, sig,
              cont(ev->working, shadow_f_by_p_and_t, ev, result),
              e, a, v);
#endif
}

static void run_block(block bk)
{
    heap bh = allocate_rolling(pages, sstring("block run"));
    //    ev->block_t_solution = 0;
    //    ev->block_f_solution = 0;
    //    ev->non_empty = false;
    ticks start = rdtsc();
    value *r = allocate(bh, (bk->regs + 1)* sizeof(value));

    apply(bk->head, bh, r);
    // arrange for flush
    //    ev->cycle_time += rdtsc() - start;

    //    if (ev->non_empty) {
    //        multibag_foreach(ev->block_f_solution, u, b)
    //            merge_multibag_bag(ev, &ev->f_solution, u, b);
        // is this really merge_multibag_bag?
    //        multibag_foreach(ev->block_t_solution, u, b)
    //            merge_multibag_bag(ev, &ev->t_solution_for_f, u, b);
    //    }

    destroy(bh);
}

extern string print_dot(heap h, block bk, table counters);

void simple_commit_handler(multibag backing, multibag m, closure(done,boolean))
{
    // xxx - clear out the new bags before anything else
    // this is so process will have them?
    if (m) {
        bag bdelta = table_find(m, bag_bag_id);
        bag b = table_find(backing, bag_bag_id);
        if (b && bdelta) apply(b->prepare, (edb)bdelta, 0, 0, (commit_handler)ignore);
    }

    multibag_foreach(m, u, b) {
        bag bd;
        if (u != bag_bag_id) {
            // xxx - we were soft creating implicitly
            // defined bags in the persistent state..but
            // we'll just going to throw them away
            if ((bd = table_find(backing, u)))
                apply(bd->prepare, b, 0, 0, (commit_handler)ignore);
        }
    }
    apply(done, true);
}

#if 0
static CONTINUATION_3_1(fp_complete, evaluation, vector, ticks, boolean);
static void fp_complete(evaluation ev, vector counts, ticks start_time, boolean status)
{
    ticks end_time = now();

    ticks handler_time = end_time;
    // counters? reflection? enable them
    apply(ev->complete, ev->t_solution, ev->last_f_solution, true);

    prf ("fixedpoint %v in %t seconds, %V iterations, %d changes to global, %d maintains, %t seconds handler\n",
         ev->name,
         end_time-start_time, 
         counts,
         multibag_count(ev->t_solution),
         multibag_count(ev->last_f_solution),
         now() - end_time);

    destroy(ev->working);
}
#endif
