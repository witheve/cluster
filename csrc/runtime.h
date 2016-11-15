#include <math.h>
#include <core/core.h>
#include <unix/unix.h>
#include <types.h>

u64 key_of(value);
boolean equals(value, value);

typedef value eboolean;
#define undefined ((void *)0)

extern heap efence;

void print(buffer, value);

typedef struct bag *bag;

void init_runtime();

void error(char *);

string aprintf(heap h, char *fmt, ...);
void bbprintf(string b, string fmt, ...);

#define pages (tcontext()->page_heap)
#define init (tcontext()->h)

typedef closure(execf, heap, value *);
typedef closure(flushf);

typedef struct object *object;
typedef struct variable *variable;

typedef struct block {
    heap h;
    execf head;
    int regs;
    table scopes;
    bag default_write;
    vector default_read;
    object *objects;
    variable *variables;
    uuid id;
} *block;

typedef struct edb *edb;
typedef closure(commit, boolean);
// if prepared is false then commit doens't have to be anything
typedef closure(commit_handler, commit, boolean);

typedef closure(preparer, edb, edb, ticks, commit_handler);
typedef closure(listener, value, value, value, uuid);
typedef closure(scanner, int, listener, value, value, value);
typedef closure(consumer, value);


typedef struct attribute *attribute;
typedef value *registers;

struct bag {
    scanner scan;
    preparer prepare;
    table listeners;
    ticks last_commit;
    vector parents;
    u64 (*cardinality)(bag b, object obj, registers, attribute);
    void (*produce)(bag b, object obj, attribute a, consumer p);
};

#include <edb.h>
#include <multibag.h>

#define def(__s, __v, __i)  table_set(__s, intern_string((unsigned char *)__v, cstring_length((char *)__v)), __i);

void print_value(buffer, value);

static value compress_fat_strings(value v)
{
    if (type_of(v) == estring_space) {
        estring x = v;
        if (x->length > 64) return(sym(...));
    }
    return v;
}


typedef closure(error_handler, char *, bag, uuid);

typedef void (*commit_function)(multibag backing, multibag delta, closure(finish, boolean));

extern table functions;
table builders_table();
block build(bag b, uuid root);
table start_fixedpoint(heap, table, table, table);

extern char *pathroot;

bag compile_eve(heap h, buffer b, boolean tracing);

void block_close(block);
bag init_request_service();

bag filebag_init(buffer);
extern thunk ignore;

static void get_stack_trace(string *out)
{
    void **stack = 0;
    asm("mov %%rbp, %0": "=rm"(stack)::);
    while (*stack) {
        stack = *stack;
        void * addr = *(void **)(stack - 1);
        if(addr == 0) break;
        bprintf(*out, "0x%016x\n", addr);
    }
}

// need the various bits of the evalution
void merge_scan(vector scopes, int sig, listener result, value e, value a, value v);

typedef struct process_bag *process_bag;
process_bag process_bag_init(multibag, boolean);

typedef closure(object_handler, edb, uuid);
object_handler create_json_session(heap h, bag over, endpoint down);

bag connect_postgres(station s, estring user, estring password, estring database);
bag env_init();
bag start_log(bag base, char *filename);


static inline value blookupv(bag b, value e, value a)
{
    return lookupv((edb)b, e, a);
}

static inline vector blookup_vector(heap h, bag b, value e, value a)
{
    return lookup_vector(h, (edb)b, e, a);
}
bag connect_postgres(station s, estring user, estring password, estring database);
bag env_init();
bag start_log(bag base, char *filename);
void serialize_edb(buffer dest, edb db);
bag udp_bag_init();
bag timer_bag_init();

station create_station(unsigned int address, unsigned short port);
void init_station();
extern heap station_heap;

#include <exec.h>

#define object_attribute(__obj, __x) (value_table_find(__obj->attributes, __x))
