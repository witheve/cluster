typedef table multibag;

#define multibag_foreach_ev(__m, __e, __a, __v)

#define multibag_foreach(__m, __u, __b)  if(__m) table_foreach(__m, __u, __b)

static inline void merge_multibag_bag(heap h, table *d, uuid u, bag s)
{
    edb bd;
    if (!*d) *d = create_value_table(h);


    if (!(bd = table_find(*d, u))) {
        table_set(*d, u, s);
    } else {
        edb_foreach((edb)s, e, a, v, bku) {
            edb_insert(bd, e, a, v, bku);
        }
    }
}

// should these guys really reconcile their differences
static inline int multibag_count(table m)
{
    int count = 0;
    multibag_foreach(m, u, b)
        count += edb_size(b);
    return count;
}

static inline void multibag_set(multibag *mb, heap h, uuid u, bag b)
{
    if (!*mb) (*mb) = create_value_table(h);
    table_set(*mb, u, b);
}


static inline void multibag_insert(multibag *mb, heap h, uuid u, value e, value a, value v, uuid block_id)
{
    bag b;

    // prf("insert: %v %v %v %v %d\n", u, e, a, compress_fat_strings(v), m);

    if (!*mb) (*mb) = create_value_table(h);
    if (!(b = table_find((*mb), u)))
        table_set(*mb, u, b = (bag)create_edb(h, 0));

    // an edb onl thing
    //    apply(b->insert, e, a, v, m, block_id);
}

static void multibag_print(multibag x)
{
    table_foreach(x, u, b){
        prf("%v:\n", u);
        prf("%b\n", edb_dump(init, (edb)b));
    }
}
