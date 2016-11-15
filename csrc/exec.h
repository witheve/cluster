typedef void (*buildf)(block, bag, uuid, execf *, flushf *);

// we should instantiate an error object - someplace?
static void exec_error(block b, char *format, ...)
{
    prf ("error %s\n", format);
}

static boolean isreg(value k)
{
    if ((type_of(k) == register_space) && (k != etrue) && (k != efalse)) return true;
    return false;
}

static inline value lookup(value *r, value k)
{
    if (isreg(k)) {
        // good look keeping your sanity if this is a non-register value in this space
        return(r[toreg(k)]);
    }
    return k;
}

// xxx - should really actually coerce to a string
static inline estring lookup_string(value *r, value k)
{
    value f = lookup(r, k);
    if (type_of(f) == estring_space) return f;
    buffer out = allocate_buffer(transient, 10);
    print_value(out, r);
    // xxx - leave in a non-comparable space and promote only on demand
    return intern_buffer(out);
}

static inline void extract(vector dest, vector keys, registers r)
{
    for (int i = 0; i< vector_length(keys); i ++) {
        vector_set(dest, i, lookup(r, vector_get(keys, i)));
    }
}


static inline void store(value *r, value reg, value v)
{
    if (reg != register_ignore)
        r[toreg(reg)] = v;
}


static inline void copyout(value *r, vector keys, vector source)
{
    for (int i = 0; i< vector_length(keys); i++) {
        store(r, vector_get(keys, i), vector_get(source, i));
    }
}

static inline void copyto(value *d, value *s, vector keys)
{
    for (int i = 0; i< vector_length(keys); i++) {
        value k =  vector_get(keys, i);
        store(d, k, lookup(s, k));
    }
}


// should try to throw an error here for writing into a non-reg
static inline int reg(value n)
{
    return ((unsigned long) n - register_base);
}



typedef struct variable {
    estring name;
    int regnum;
    vector attributes; // an attribute is specific to an object
} *variable;


typedef closure(cardinality_handler, u64);
typedef closure(checker, variable, registers);

// need an attribute name for entity
typedef struct object {
    table attributes;
    multibag scopes;
    checker check;
    void (*cardinality)(object obj, variable v, cardinality_handler);
    void (*produce)(block bk, object obj, registers, variable v);
} *object;

typedef struct attribute {
    estring name;
    // avoiding additional dynamic typing, either free or v is true
    variable free;
    value v;
    object obj;
} *attributes;

static inline value alookup(registers r, attribute a)
{
    if (a->v != undefined) return a->v;
    return r[a->free->regnum];
}

static inline boolean bound(registers r, variable v)
{
    if (lookup(r, v) == undefined) return false;
    return true;
}
