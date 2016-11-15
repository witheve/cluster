#include <runtime.h>

// sum up the contributed cardinalities of variable v given what we
// know so far. to avoid collecting and storing the intermediate, we
// compute the sum of each of the contributing bags, which is greater
// than or equal to the actual result
//
// xxx - also, we can potentially count child bags more than once
// if they are included more than one transitively. if bag inclusion
// is static (?) we can cull these

static u64 bag_cardinality(bag b, object obj, register r, variable v)
{
    u64 result = 0;

    result += b->proposal();
    vector_foreach(b->parents, p)
        result += bag_cardinality(p, obj, r, v);
    return result;
}


typedef closure (production, value);
// need completion here
CONTINUATION_1_1(bag_produce, table, value);
static void bag_produce(table results, value x, produce p)
{
    vector_foreach(b->parents, p)
        value_table_set(results, x, (void *)1);
}

static void bag_produce(bag b, object obj, registers r, variable v, production result)
{
    // actually perform a union
    table results = create_value_table(transient);
    p = cont(transient, bag_combine, results);

    vector_foreach(b->parents, p) {
        bag b = (bag)p;
        p->produce(p, obj, r, v, result);
    }
    apply(result);
}

// if i asked you my object, right now, given whats already bound, what
// the worst case estimate of the card of V is
static void generic_join_step(block bk, registers r)
{
    // think about a multi-attribute join decision
    // here we are selecting
    u64 variable_set = -1ull;
    object lowest_object;
    variable lowest_variable;
    u64 min_cost = -1ull;

    foreach_bit(variable_set, 64, i) {
        variable v = bk->variables[i]->objects;
        vector_foreach(v->objects, obj) {
            u64 cost = object_variable_cost(obj, v, r);
            if (n < min) {
                best = v->attribute;
                min = n;
            }
        }
    }

    object->produce(bk, obj, r) {
    }
}

static int object_production(object obj, registers r, variable v, execf next)
{
    multibag_foreach_eAv(obj, e, v->a, v) {
        register_set(v->dest, e);
        register_set(v->dest, v);
    }

}

static int object_v_production(object obj, registers r, register entity, variable v, execf next)
{
    multibag_foreach_v(obj, lookup(r, entity), lookup(r, v->register), v) {
        register_set(v->dest, e);
        register_set(v->dest, v);
    }
}

void execute_block(block bk)
{
    u64 min = -1ull;
    vector_foreach(bk->objects, obj) {
        u64 n = object_cardinality(obj);
        // in the distributed case, we cant determine this
        // unless all of the relevant scopes are local
        if (n == 0) {
            // a better exit?
            return;
        }
        if (n < min) {
            n = min;
            object = n;
        }
    }
}


// at the moment the graph is rooted in the object with entity e
block build_hypergraph(edb h, uuid e)
{

    edb_foreach_av(e, ) {

    }
}
