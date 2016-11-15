typedef struct level {
    // this is the count of all of the leaves under this node,
    // the table entry count is the number of immediate children
    u64 count;
    table entries;
} *level;

struct edb {
    struct bag b;
    level eav;
    level ave;
    heap h;
    u64 count;
    vector includes; // an immutable set
};

static inline level allocate_level(edb e)
{
    level out = allocate(e->h, sizeof(struct level));
    out->count = 0;
    out->entries = create_value_table(e->h);
    return out;
}

static inline value level_find_create(edb e, level current, value key) {
    level next_level = value_table_find(current->entries, key);
    if(!next_level) {
        next_level = allocate_level(e);
        table_set(current->entries, key, next_level);
        current->count++;
    }
    return next_level;
}

#define level_foreach(_t, _k, _v)\
  if (_t) table_foreach(((level)_t)->entries, _k, _v)

#define level_find(_t, _k)\
    ({\
      void *x = 0 ;\
      if (_t) x = value_table_find(((level)_t)->entries, _k);\
      x;\
    })


// does this really need to return status?
static inline boolean level_set(level l, value key, void *x)
{
    table_set(l->entries, key, x);
    l->count++;
    return true;
}

typedef struct leaf {
    uuid u;
    uuid block_id;
    ticks t;
} *leaf;

// it may prove advantageous to change this encoding to allow for dont care states
// in addition to input and output
#define e_sig 0x04
#define a_sig 0x02
#define v_sig 0x01
#define s_eav 0x0
#define s_eAv (a_sig)
#define s_eAV (a_sig | v_sig)
#define s_Eav (e_sig)
#define s_EAv (e_sig | a_sig)
#define s_EAV (e_sig | a_sig | v_sig)

value lookupv(edb b, uuid e, estring a);
vector lookup_vector(heap h, edb b, uuid e, estring a);

int edb_size(edb b);
void destroy_bag(bag b);

// xxx - these iterators dont account for shadowing
#define edb_foreach(__b, __e, __a, __v, __block_id)   \
    level_foreach((__b)->eav, __e, __avl) \
    level_foreach(__avl, __a, __vl)\
    level_foreach(__vl, __v, __cv)\
    for(uuid __block_id = ((leaf)__cv)->block_id , __p = 0; !__p; __p++)

long count_of(edb b, value e, value a, value v);
edb create_edb(heap, vector inherits);

#define edb_foreach_av(__b, __e, __a, __v)\
    for(level __av = level_find((__b)->eav, __e); __av; __av = 0)  \
    level_foreach(__av, __a, __vl)\
    level_foreach(__vl, __v, __cv)

#define edb_foreach_ev(__b, __e, __a, __v)\
    for(level __avt = level_find((__b)->ave, __a); __avt; __avt = 0)  \
    level_foreach(__avt, __v, __ect)\
    level_foreach(__ect, __e, __cv)

#define edb_foreach_v(__b, __e, __a, __v)\
    for(level __av = level_find((__b)->eav, __e); __av; __av = 0)  \
    for(level __vv = level_find(__av, __a); __vv; __vv = 0)  \
    level_foreach(__vv, __v, __cv)

#define edb_foreach_e(__b, __e, __a, __v)\
    for(level __avt = level_find((__b)->ave, __a),\
              __et = __avt?level_find(__avt, __v):0; __et; __et = 0)   \
    level_foreach(__et, __e, __cv)

buffer edb_dump(heap, edb);


// this doesn't use includes - not sure if that makes sense:/
static inline vector lookup_array(heap h, edb b, uuid e)
{
    vector dest = allocate_vector(h, 10);
    value x;
    for (int i = 0; (x = lookupv(b, e, box_float(i))); i++)
        vector_insert(dest, x);
    return dest;
}

boolean edb_insert(edb, value, value, value, value); //eavb
string edb_dump_dot(edb s, uuid u);
