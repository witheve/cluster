#include <runtime.h>
#include <exec.h>


static vector scopes_uuid_set(block bk, vector scopes)
{
    if (vector_length(scopes) == 0) return 0;
    vector out = allocate_vector(bk->h, vector_length(scopes));
    vector_foreach(scopes, i) {
        // we're going to soft create these scopes, but the uuids
        // remain unreferrable to the outside world
        if (isreg(i)) {
            vector_insert(out, i);
        } else {
            uuid u = table_find(bk->scopes, i);
            if (!u) {
                uuid u = generate_uuid();
                prf("Unable to find context: %v. New id: %v\n", i, u);
                table_set(bk->scopes, i, u);
            }
            vector_insert(out, u);
        }
    }
    return out;
}

typedef struct insert  {
    block bk;
    execf n;
    bag target;
    heap *h;
    value e, a, v;
} *insert;

static CONTINUATION_1_3(do_insert, insert, 
                        heap, multibag, value *);

// this should pass the commit changes
static void do_insert(insert ins,
                      heap h, multibag evaluation, value *r)
{
    // should be a single scope?
    multibag_insert(evaluation, 
                    ins->target,
                    lookup(r, ins->e),
                    lookup(r, ins->a),
                    lookup(r, ins->v),
                    ins->bk->id);
    
}


static void build_mutation(block bk, bag b, uuid n, execf *e, flushf *f)
{
    value mt = blookupv(b, n, sym(mutateType));
    insert ins = allocate(bk->h, sizeof(struct insert));

    if (mt == sym(bind)) {
        ins->h = &bk->h;
        // bind 
        //        ins->target = &bk->f_solution;
    } else if (mt == sym(commit)) {
        ins->h = &bk->h;
        //        ins->target = bk->default_commit_target;
    } else {
        prf("unknown mutation scope: %v\n", mt);
    }

    vector name_scopes = blookup_vector(bk->h, b, n, sym(scopes));

    ins->bk = bk;
    ins->n =  cfg_next(bk, b, n);
    ins->e = blookupv(b, n, sym(e));
    ins->a = blookupv(b, n, sym(a));
    ins->v = blookupv(b, n, sym(v));
    ins->scopes = vector_length(name_scopes)?scopes_uuid_set(bk, name_scopes):bk->default_insert_scopes;
    *e = cont(bk->h, do_insert, ins);
}


static void build_insert(block bk, bag b, uuid n, execf *e, flushf *f)
{
    build_mutation(bk, b, n, e, f);
}

static void build_remove(block bk, bag b, uuid n, execf *e, flushf *f)
{
    prf("whoops, remove under construction\n");
}

static CONTINUATION_4_4(each_t_solution_remove,
                        evaluation, heap, uuid, multibag *,
                        value, value, value, uuid);
static void each_t_solution_remove(evaluation ev, heap h, uuid u, multibag *target,
                                   value e, value a, value v, uuid block_id)
{
    //        multibag_insert(target, h, u, e, a, v, block_id);
}

static CONTINUATION_3_4(each_t_remove,
                        heap, uuid, multibag *,
                        value, value, value, uuid);
static void each_t_remove(heap h, uuid u, multibag *target,
                          value e, value a, value v, uuid block_id)
{
    prf("remove has been deconstructed into a single crumb and a smear of aoli\n");
}

static CONTINUATION_8_3(do_set, block, execf,
                        vector, value, value, value, value,
                        heap, value *);
static void do_set(block bk, execf n,
                   vector scopes, value mt,
                   value e, value a, value v,
                   heap h,  value *r)
{
    value ev = lookup(r, e);
    value av=  lookup(r, a);
    value vv=  lookup(r, v);
    boolean should_insert = true;
    bag b;
    multibag *target;

    if (mt == sym(bind)) {
        target = &bk->block_f_solution;
    } else {
        target = &bk->block_t_solution;
    }

    vector_foreach(scopes, u) {
        if (vv != register_ignore)
            multibag_insert(target, bk->h, u, ev, av, vv, bk->name);

        if ((b = table_find(bk->t_input, u))) {
            apply(b->scan, s_EAv,
                  cont(h, each_t_remove, bk->, bk->working, u, target),
                  ev, av, 0);
        }
    }
    apply(n, h, r);
}

static void build_set(block bk, bag b, uuid n, execf *e, flushf *f)
{
    vector name_scopes = blookup_vector(bk->h, b, n,sym(scope));

    *e = cont(bk->h,
              do_set,
              bk,
              cfg_next(bk, b, n),
              vector_length(name_scopes)?scopes_uuid_set(bk, name_scopes):bk->default_insert_scopes,
              blookupv(b, n,sym(mutateType)),
              blookupv(b, n,sym(e)),
              blookupv(b, n,sym(a)),
              blookupv(b, n,sym(v)));
}

static CONTINUATION_6_3(do_erase, execf, block,
                        vector, value, value,
                        heap, value *);
static void do_erase(execf n, block bk, vector scopes, value mt, value e,
                     heap h, value *r)
{
    bag b;
    multibag *target;
    value ev = lookup(r, e);
    if (mt == sym(bind)) {
        target = &bk->block_f_solution;
    } else {
        target = &bk->block_t_solution;
    }

    vector_foreach(scopes, u) {
        // xxx - this can be done in constant time rather than
        // the size of the object. the attribute tables are also
        // being left behind below, which will confuse generic join
        if ((b = table_find(bk->t_input, u))) {
            apply(b->scan, s_Eav,
                  cont(h, each_t_remove, bk->, bk->working, u, target),
                  ev, 0, 0);
        }
        if (bk->t_solution && (b = table_find(bk->t_solution, u))) {
            apply(b->scan, s_Eav,
                  cont(h, each_t_solution_remove, bk->, bk->working, u, target),
                  ev, 0, 0);
        }
    }
    apply(n, h, r);
}

static void build_erase(block bk, bag b, uuid n, execf *e, flushf *f)
{
    vector name_scopes = blookup_vector(bk->h, b, n,sym(scopes));

    *e = cont(bk->h, do_erase,
              cfg_next(bk, b, n),
              bk,
              vector_length(name_scopes)?scopes_uuid_set(bk, name_scopes):bk->default_insert_scopes,
              blookupv(b, n,sym(mutateType)),
              blookupv(b, n,sym(e)));
}

extern void register_edb_builders(table builders)
{
    table_set(builders, intern_cstring("insert"), build_insert);
    table_set(builders, intern_cstring("remove"), build_remove);
    table_set(builders, intern_cstring("set"), build_set);
    table_set(builders, intern_cstring("scan"), build_scan);
    table_set(builders, intern_cstring("erase"), build_erase);
}
