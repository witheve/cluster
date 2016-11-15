#include <runtime.h>

static CONTINUATION_2_2(do_sub_tail, value, vector, heap, value *);
static void do_sub_tail(value resreg,
                        vector outputs,
                        heap h, value *r)
{
    // just drop flush and remove on the floor
    table results = lookup(r, resreg);
    vector result = allocate_vector(results->h, vector_length(outputs));
    extract(result, outputs, r);
    table_set(results, result, etrue);
}

static void build_sub_tail(block bk, bag b, uuid n, execf *e, flushf *f)
{
    *e = cont(bk->h,
              do_sub_tail,
              blookupv(b, n, sym(pass)),
              blookup_vector(bk->h, b, n, sym(provides)));
}


typedef struct sub {
    value id;
    vector v;
    vector projection;
    vector outputs;
    vector ids;
    table ids_cache; //these persist for all time
    table results;
    execf leg, next;
    value resreg;
    heap resh;
    heap h;
    boolean id_collapse;
} *sub;


static void set_ids(sub s, vector key, value *r)
{
    vector k;

    if (!(k = table_find(s->ids_cache, key))) {
        int len = vector_length(s->ids);
        k = allocate_vector(s->h, len);
        for (int i= 0; i < len; i++)
            vector_set(k, i, generate_uuid());
        table_set(s->ids_cache, key, k);
    }
    copyout(r, s->ids, k);
}

static void subflush(sub s, flushf n)
{
    if (s->results){
        s->results = 0;
        destroy(s->resh);
    }
    apply(n);
    return;
}

static CONTINUATION_1_2(do_sub, sub, heap, value *);
static void do_sub(sub s, heap h, value *r)
{
    table res;
    extract(s->v, s->projection, r);
    vector key;

    if (!s->results) {
        s->resh = allocate_rolling(pages, sstring("sub-results"));
        s->results = create_value_vector_table(s->resh);
    }

    if (!(res = table_find(s->results, s->v))){
        res = create_value_vector_table(s->h);
        key = allocate_vector(s->h, vector_length(s->projection));
        extract(key, s->projection, r);
        store(r, s->resreg, res);
        if (s->id_collapse) {
            set_ids(s, key, r);
        } else{
            vector_foreach(s->ids, i)
                store(r, i, generate_uuid());
        }
        apply(s->leg, h, r);
        table_set(s->results, key, res);
    }

    // cross
    table_foreach(res, n, _) {
        copyout(r, s->outputs, n);
        apply(s->next, h, r);
    }
}


static void build_subproject(block bk, bag b, uuid n, execf *e, flushf *f)
{
    sub s = allocate(bk->h, sizeof(struct sub));
    s->h = bk->h;
    s->results = 0;
    s->ids_cache = create_value_vector_table(s->h);
    s->projection = blookup_vector(bk->h, b, n, sym(projection));
    s->v = allocate_vector(s->h, vector_length(s->projection));
    value leg = blookupv(b, n, sym(arm));
    s->outputs = blookup_vector(bk->h, b, n, sym(provides));
    s->resreg =  blookupv(b, n, sym(pass));
    s->ids = blookup_vector(bk->h, b, n, sym(ids));
    s->id_collapse = (blookupv(b, n, sym(id_collapse))==etrue)?true:false;
    *e = cont(s->h,
              do_sub,
              s);

}


static void do_time(block bk, execf n, value hour, value minute, value second, value frame, timer t,
                    heap h, value *r)
{
    unsigned int seconds, minutes,  hours;
    //    clocktime(bk->ev->t, &hours, &minutes, &seconds);
    value sv = box_float((double)seconds);
    value mv = box_float((double)minutes);
    value hv = box_float((double)hours);
    //    u64 ms = ((((u64)bk->ev->t)*1000ull)>>32) % 1000;
    //    value fv = box_float((double)ms);
    //    store(r, frame, fv);
    store(r, second, sv);
    store(r, minute, mv);
    store(r, hour, hv);
    apply(n, h, r);
}

static table builders;

extern void register_exec_expression(table builders);
extern void register_string_builders(table builders);
extern void register_aggregate_builders(table builders);
extern void register_edb_builders(table builders);


table builders_table()
{
    if (!builders) {
        builders = allocate_table(init, key_from_pointer, compare_pointer);
        table_set(builders, intern_cstring("subproject"), build_subproject);
        table_set(builders, intern_cstring("subtail"), build_sub_tail);
        //        table_set(builders, intern_cstring("choose"), build_choose);
        //        table_set(builders, intern_cstring("choosetail"), build_choose_tail);
        //        table_set(builders, intern_cstring("move"), build_move);
        //        table_set(builders, intern_cstring("not"), build_not);
        //        table_set(builders, intern_cstring("time"), build_time);
        //        table_set(builders, intern_cstring("random"), do_random);

        register_exec_expression(builders);
        register_string_builders(builders);
        register_aggregate_builders(builders);
        //        register_edb_builders(builders);
    }
    return builders;
}

void block_close(block bk)
{
    // who calls this and for what purpose? do they want a flush?
    destroy(bk->h);
}

block build_block(edb e, uuid u)
{
    heap h = allocate_rolling(pages, sstring("block"));
    edb_foreach_v(e, u, sym("object"), obj) {
        edb_foreach_v(e, obj, sym("attribute"), attr) {
            attribute a = allocate(h, sizeof(struct attribute));
            estring n = lookupv(e, attr, sym(name));
            value k = lookupv(e, attr, sym(value));
        }
    }
}
