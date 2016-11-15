#include <runtime.h>

// leaf aggregator functions are still synchronous
static table aggbuilders;

typedef closure(aggflush, int);
typedef closure(aggexec, int);

static void sort_flush(pqueue q, value out, heap h, execf n, value *r)
{
    value i;
    int count;
    while ((i = pqueue_pop(q))) {
        // if we dont do the denorm trick, these should at least be findable and resuable
        store(r, out, box_float(count++));
        apply(n, h, r);
    }
}

// we're suposed to have multiple keys and multiple sort orders, ideally
// just generate a comparator over r
static CONTINUATION_6_2(do_sort,
                        execf, 
                        table *, value, value, vector, vector,
                        heap, value *);
static void do_sort(execf n, 
                    table *targets, value key, value out, vector proj, vector pk,
                    heap h, value *r)
{
    extract(pk, proj, r);
    pqueue x;
    if (!(x = table_find(*targets, pk))) {
        x = allocate_pqueue(h, order_values);
        // make a new key idiot
        table_set(*targets,pk, x);
    }
    pqueue_insert(x, lookup(r, key));
}

static void build_sort(block bk, bag b, uuid n, execf *e, flushf *f)
{
    vector groupings = blookup_vector(bk->h, b, n, sym(groupings));
    if (!groupings) groupings = allocate_vector(bk->h, 0);
    vector pk = allocate_vector(bk->h, vector_length(groupings));
    table *targets = allocate(bk->h, sizeof(table));
    *targets = create_value_vector_table(bk->h);

    /*   *e =  cont(bk->h,
               do_sort,
               cfg_next(bk, b, n),
               targets,
               blookupv(b, n, sym(value)),
               blookupv(b, n, sym(return)),
               groupings,
               pk); */
}


typedef struct join_key{
    value index;
    estring token;
    estring with;
} *join_key;


static boolean order_join_keys(void *a, void *b)
{
    join_key ak = a, bk = b;
    // sort value?
    return *(double*)ak->index < *(double*)bk->index;
}

static void join_flush(value out, pqueue q, execf n, heap h, value *r)
{
    buffer composed = allocate_string(h);
    join_key jk;
    while ((jk = (join_key)pqueue_pop(q))){
        buffer_append(composed, jk->token->body, jk->token->length);
        buffer_append(composed, jk->with->body, jk->with->length);
    }
    store(r, out, intern_buffer(composed));
    apply(n, h, r);
}

static CONTINUATION_4_2(do_join, pqueue, value, value, value,
                        heap, value *);
static void do_join(pqueue q, value token, value index, value with,
                    heap h, value *r)
{
    join_key jk = allocate(h, sizeof(struct join_key));
    jk->index = lookup(r, index);
    jk->token = lookup_string(r, token);
    jk->with = lookup_string(r, with);
    pqueue_insert(q, jk);
}

 static execf build_join(heap h, bag b, uuid n)
 {
    return cont(h,
                do_join,
                allocate_pqueue(h, order_join_keys),
                blookupv(b, n, sym(token)),
                blookupv(b, n, sym(index)),
                blookupv(b, n, sym(with)));
}

typedef double (*dubop)(double, double);

// these should use generic comparator
static double op_min(double a, double b)
{
    return (a<b)?a:b;
}
static double op_max(double a, double b)
{
    return (a>b)?a:b;
}
static double op_sum(double a, double b)
{
    return a+b;
}


//static CONTINUATION_2_3(simple_agg_flush, value, value, heap, value *);
static void simple_agg_flush(value x, value dst, heap h, value *r)
{
    store(r, dst, box_float(*(double *)x));
}

static CONTINUATION_3_2(do_simple_agg, dubop,  double *, value,
                        heap, value *);
static void do_simple_agg(dubop op, double *x, value src,
                          heap h, value *r)
{
    *x = op(*x, *(double *)lookup(r, src));
}

static void build_simple_agg(heap h, bag b, uuid n, execf *e, execf *f)
{
    vector groupings = blookupv(b, n,sym(groupings));
    dubop op;
    double *x = allocate(h, sizeof(double *));
    value type = blookupv(b, n, sym(type));

    if (type == sym(max)) op = op_max;
    if (type == sym(min)) op = op_min;
    if (type == sym(sum)) op = op_sum;
    /*
    *e = cont(h,
              do_simple_agg,
              op,
              x,
              blookupv(b, n, sym(value)));
    *f = cont(h,
              simple_agg_flush,
              x,
              blookupv(b, n, sym(return)));
    */
}

typedef struct subagg {
    heap phase;
    vector projection;
    vector groupings;
    table proj;
    table group;
    vector key;
    vector gkey;
    value pass;
    int regs;
} *subagg;


static CONTINUATION_3_2(do_subagg_tail,
                        execf, value, vector,
                        heap, value *);
static void do_subagg_tail(execf next, value pass,
                           vector produced,
                           heap h, value *r)
{
    subagg sag =  lookup(r, pass);
    extract(sag->gkey, sag->groupings, r);
    vector cross = table_find(sag->group, sag->gkey);
    // cannot be empty
    vector_foreach(cross, i) {
        copyto(i, r, produced);
        apply(next, h, i);
    }
}

static void build_subagg_tail(block bk, bag b, uuid n, execf *e, flushf *f)
{
    vector groupings = blookup_vector(bk->h, b, n,sym(groupings));
    // apparently this is allowed to be empty?
    if (!groupings) groupings = allocate_vector(bk->h,0);
    table* group_inputs = allocate(bk->h, sizeof(table));
    *group_inputs = create_value_vector_table(bk->h);

    vector v = allocate_vector(bk->h, groupings?vector_length(groupings):0);
    /*    *e = cont(bk->h,
              do_subagg_tail,
              cfg_next(bk, b, n),
              blookupv(b, n,sym(pass)),
              blookupv(b, n,sym(provides)));*/
}

static void subagg_flush(subagg sag, flushf next)
{
    if (sag->phase) destroy(sag->phase);
    sag->phase = 0;
}


static CONTINUATION_2_2(do_subagg,
                        execf, subagg,
                        heap, value *);

static void do_subagg(execf next, subagg sag,
                      heap h, value *r)
{
    if (!sag->phase) {
        sag->phase = allocate_rolling(pages, sstring("subagg"));
        sag->proj =  create_value_vector_table(sag->phase);
        sag->group =  create_value_vector_table(sag->phase);
    }

    extract(sag->key, sag->projection, r);
    if (!table_find(sag->proj, sag->key)) {
        vector key = allocate_vector(sag->phase, vector_length(sag->projection));
        extract(key, sag->projection, r);
        table_set(sag->proj, key, (void *)1);
        store(r, sag->pass, sag);
        apply(next, h, r);
    }

    vector cross;
    extract(sag->gkey, sag->groupings, r);
    if (!(cross = table_find(sag->group, sag->gkey))) {
        cross = allocate_vector(sag->phase, 5);
        vector key = allocate_vector(sag->phase, vector_length(sag->groupings));
        extract(key, sag->groupings, r);
        table_set(sag->group, sag->gkey, cross);
    }

    value *cr = allocate(sag->phase, sag->regs * sizeof(value));
    memcpy(cr, r,  sag->regs * sizeof(value));
    vector_insert(cross, cr);
}

// subagg and subaggtail are an oddly specific instance of a general cross
// function and a general project function. there a more general compiler
// model which obviates the need for this
static void build_subagg(block bk, bag b, uuid n, execf *e, flushf *f)
{
    subagg sag = allocate(bk->h, sizeof(struct subagg));
    sag->phase = 0;
    sag->proj = 0;
    sag->group = 0;
    sag->projection = blookup_vector(bk->h, b, n, sym(projection));
    sag->groupings = blookup_vector(bk->h, b, n, sym(groupings));
    sag->key = allocate_vector(bk->h, vector_length(sag->projection));
    sag->gkey = allocate_vector(bk->h, vector_length(sag->groupings));
    sag->pass = blookupv(b, n,sym(pass));
    sag->regs = bk->regs;
    *e = cont(bk->h,
              do_subagg,
              *e,
              sag);
}

static void flush_grouping(heap h,
                           table *groups,
                           vector grouping,
                           int regs,
                           vector pk,
                           flushf f)
{
    value *r = allocate(h, regs*sizeof(value));
    execf x;
    // wtf are you?
    if (!(x = table_find(*groups, pk))) {
        // x needs to contain flusho
        table_foreach(*groups, pk, x) {
            // allocate r
            copyout(r, grouping, pk);
            apply((execf)x, h, r);
        }
        apply(f);
    }
}

typedef closure(aggbuilder, heap, execf *);

static CONTINUATION_6_2(exec_grouping,
                        heap, aggbuilder, execf, table*, vector, vector,
                        heap, value *);
static void exec_grouping(heap bh, aggbuilder b, execf n, table *groups, vector groupings, vector pk,
                           heap h, value *r)
{
    extract(pk, groupings, r);
    execf x;

    if (!*groups)
        *groups = create_value_vector_table(bh);

    if (!(x = table_find(*groups, pk))) {
        vector new_pk = allocate_vector(h, vector_length(groupings));
        extract(new_pk, groupings, r);
        apply(b, h, &x);
        table_set(*groups, new_pk, x);
    }
    apply(x, h, r);
    apply(n, h, r);
}

static void build_grouping(block bk, bag b, uuid n, execf *e, flushf *f)
{
    aggbuilder ba = table_find(aggbuilders, blookupv(b, n, sym(type)));
    vector groupings = blookup_vector(bk->h, b, n, sym(groupings));
    table *groups = allocate(bk->h, sizeof(struct table));
    *groups = 0;
    /*    *e = cont(bk->h,exec_grouping,
              bk->h, ba, cfg_next(bk, b, n),
              groupings,
              allocate_vector(bk->h, vector_length(groupings)));*/
}

void register_aggregate_builders(table builders)
{
    aggbuilders = create_value_table(init);
    table_set(builders, intern_cstring("subagg"), build_subagg);
    table_set(builders, intern_cstring("subaggtail"), build_subagg_tail);
    table_set(builders, intern_cstring("sum"), build_grouping);
    table_set(builders, intern_cstring("join"), build_grouping);
    table_set(builders, intern_cstring("sort"), build_grouping);

    // aggbuilders get called on a case by case basis by
    // the grouper
    table_set(aggbuilders, intern_cstring("sum"), build_simple_agg);
    table_set(aggbuilders, intern_cstring("min"), build_simple_agg);
    table_set(aggbuilders, intern_cstring("max"), build_simple_agg);
    table_set(aggbuilders, intern_cstring("join"), build_join);
    table_set(aggbuilders, intern_cstring("sort"), build_sort);
}
