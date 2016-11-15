#include <runtime.h>
#include <unistd.h>
#include <stdio.h>

value lookupv(edb b, uuid e, estring a)
{
    level al = level_find(b->eav, e);
    table vl = level_find(al, a);
    level_foreach(vl, v, _)
        return v;

    vector_foreach(b->includes, i) {
        value x = lookupv(i, e, a);
        if (x) return x;
    }

    return(0);
}

static void lookup_vector_internal(vector dest, edb b, uuid e, estring a)
{
    table al = level_find(b->eav, e);
    table vl = level_find(al, a);
    level_foreach(vl, v, terminal)
        vector_insert(dest, v);

    vector_foreach(b->includes, i)
        lookup_vector_internal(dest, i, e, a);
}

vector lookup_vector(heap h, edb b, uuid e, estring a)
{
    vector dest = allocate_vector(h, 10);
    lookup_vector_internal(dest, b, e, a);
    return dest;
}


int edb_size(edb b)
{
    return b->count;
}

static CONTINUATION_1_5(edb_scan, edb, int, listener, value, value, value);
static void edb_scan(edb b, int sig, listener out, value e, value a, value v)
{
    vector_foreach(b->includes, i)
        edb_scan(i, sig, out, e, a, v);

    switch (sig) {
    case s_eav:
        level_foreach(b->eav, e, al) {
            level_foreach((table)al, a, vl) {
                level_foreach((table)vl, v, f) {
                    leaf final = f;
                    apply(out, e, a, v, final->block_id);
                }
            }
        }
        break;

    case s_EAV:
        {
            table al = level_find(b->eav, e);
            if(al) {
                table vl = level_find(al, a);
                if(vl) {
                    leaf final;
                    if ((final = level_find(vl, v)) != 0){
                        apply(out, e, a, v, final->block_id);
                    }
                }
            }
            break;
        }

    case s_EAv:
        {
            table al = level_find(b->eav, e);
            if(al) {
                table vl = level_find(al, a);
                if(vl) {
                    level_foreach(vl, v, f) {
                        leaf final = f;
                        if(final)
                            apply(out, e, a, v, final->block_id);
                    }
                }
            }
            break;
        }

    case s_Eav:
        {
            table al = level_find(b->eav, e);
            if(al) {
                level_foreach(al, a, vl) {
                    level_foreach((table)vl, v, f){
                        leaf final = f;
                        if(final)
                            apply(out, e, a, v, final->block_id);
                    }
                }
            }
            break;
        }

    case s_eAV:
        {
            table al = level_find(b->ave, a);
            if(al) {
                table vl = level_find(al, v);
                if(vl) {
                    level_foreach(vl, e, f) {
                        leaf final = f;
                        if(final)
                            apply(out, e, a, v, final->block_id);
                    }
                }
            }
            break;
        }

    case s_eAv:
        {
            table al = level_find(b->ave, a);
            if(al) {
                level_foreach(al, v, vl) {
                    level_foreach((table)vl, e, f) {
                        leaf final = f;
                        if(final)
                            apply(out, e, a, v, final->block_id);
                    }
                }
            }
            break;
        }

    default:
        prf("unknown scan signature:%x\n", sig);
    }
}

static CONTINUATION_1_5(edb_scan_sync, edb, int, listener, value, value, value);
static void edb_scan_sync(edb b, int sig, listener out, value e, value a, value v) {
  edb_scan(b, sig, out, e, a, v);
}

// should return status?
boolean edb_insert(edb b, value e, value a, value v, uuid block_id)
{
    leaf final;

    level el = level_find_create(b, b->eav, e);
    level al = level_find_create(b, el, a);

    if (!(final = level_find(al, v))){
        final = allocate(b->h, sizeof(struct leaf));
        final->block_id = block_id;
        level_set(al, v, final);

        // AVE
        level aal = level_find_create(b, b->ave, a);
        level avl = level_find_create(b, aal, v);
        level_set(avl, e, final);
        b->count++;
        return true;
    }
    return false;
}

static CONTINUATION_1_4(edb_prepare, edb, edb, edb, ticks, commit_handler);
static void edb_prepare(edb b, edb add, edb remove, ticks t, commit_handler h)
{
    //    edb_foreach(source, e, a, v, block_id)
    //        edb_insert(b, e, a, v, block_id);
    // activate the listeners
}

static int buffer_unicode_length(buffer buf, int start)
{
    int length = 0;
    int limit = buffer_length(buf);
    for (u32 x = start, q;
         (q = utf8_length(*(u32 *)bref(buf, x))),  x<limit;
         x += q) length++;
    return length;
}


edb create_edb(heap h, vector includes)
{
    edb b = allocate(h, sizeof(struct edb));
    b->b.scan = cont(h, edb_scan, b);
    b->b.listeners = allocate_table(h, key_from_pointer, compare_pointer);
    b->b.prepare = cont(h, edb_prepare, b);
    b->h = h;
    b->count = 0;
    b->eav = allocate_level(b);
    b->ave = allocate_level(b);
    if(includes != 0 ) {
      b->includes = includes;
    } else {
      b->includes = allocate_vector(h, 1);
    }

    return b;
}

static string dump_dot_internal(heap h, int *count, buffer dest, table visited, edb b, uuid n)
{
    buffer tag;
    if (!(tag = table_find(visited, n))) {
        buffer desc = allocate_buffer(h, 10);
        tag = aprintf(h, "%d", *count);
        *count = *count + 1;
        table_set(visited, n, tag);
        edb_foreach_av(b, n, a, v) {
            if (type_of(v) == uuid_space) {
                string target = dump_dot_internal(h, count, dest, visited, b, v);
                bprintf(dest, "%b -> %b [label=\"%r\"]\n", tag, target, a);
            } else bprintf(desc, "%r:%r\\n", a, v);
        }
        bprintf(dest, "%b [label = \"%b\"]\n", tag, desc);
    }
    return tag;
}


string edb_dump_dot(edb s, uuid u)
{
    buffer out = allocate_string(transient);
    int count = 0;
    table visited = create_value_table(transient);
    bprintf(out, "digraph f {\n");
    dump_dot_internal(transient, &count, out, visited, s, u);
    bprintf(out, "}\n");
    return out;
}


// there is also a reasonable role for a probe here
static void edb_produce(edb e, object obj, registers r, attribute a, consumer c)
{
    if (bound(r, object_attribute(obj, sym(self)))) {
        level al = level_find(e->eav, lookup(r, a->name));
        if (al) {
            level vl = level_find(al, a->name);
            level_foreach(vl, v, _) 
                apply(c, v);
        }
    } else {
        level al = level_find(e->ave, a->name);
        // xxx- i guess we aren't producing v here - i guess this is if we are looking for self?
        level_foreach(al, v, vl) { 
            level_foreach(vl, e, _) 
                apply(c, e);
        }
    }
}

static u64 edb_cardinality(edb b, object obj, registers r, attribute a)
{
    if (bound(r, object_attribute(obj, sym(self)))) {
        // the early intersection check is supposed to throw these out,
        // but lets ignore the early intersection check
        level al = level_find(b->eav, lookup(r, a->name));
        if (al) {
            level vl = level_find(al, a->name);
            if (vl) return vl->count;
        }
    } else {
        level al = level_find(b->ave, a->name);
        if (al) return al->count;
    }
    return 0;
}

string edb_dump(heap h, edb b)
{
    buffer out = allocate_string(h);
    level_foreach(b->eav, e, avl) {
        int start = buffer_length(out);
        bprintf(out, "%v ", e);
        int ind = buffer_unicode_length(out, start);
        int first =0;

        level_foreach((table)avl, a, vl) {
            int second = 0;
            int start = buffer_length(out);
            bprintf(out, "%S%v ", first++?ind:0, a);
            int ind2 = buffer_unicode_length(out, start) + ((first==1)?ind:0);
            level_foreach(vl, v, _)
                bprintf(out, "%S%v\n", second++?ind2:0, compress_fat_strings(v));
        }
    }
    return out;
}
