#include <runtime.h>

static void do_concat(execf n, value dest, vector terms, heap h, value *r)
{
    buffer b = allocate_string(h);

    vector_foreach(terms, i)
        print_value_raw(b, lookup(r, i));

    store(r, dest, intern_string(bref(b, 0), buffer_length(b)));
    apply(n, h, r);
}


#if 0
static inline void output_split(execf n, buffer out, int ind,
                                heap h, value *r, value token, value index,
                                boolean bound_index, boolean bound_token)
{
    estring k = intern_buffer(out);
    if ((!bound_index || (ind == *(double *)lookup(r, index))) &&
        (!bound_token || (k == lookup(r, token)))){
        store(r, token, k) ;
        store(r, index, box_float(ind));
        //         apply(n, h, p, r);

    }
}
#endif

// split expands cardinality
static void do_split(value token, value text, value index, value by,
                     boolean bound_index, boolean bound_token,
                     heap h, value *r)
{
    buffer out = 0;
    int j = 0;
    int ind = 0;
    estring s = lookup(r, text);
    estring k = lookup(r, by);
    // utf8
    for (int i = 0; i < s->length; i++) {
        character si = s->body[i];
        character ki = k->body[j];

        if (!out) out = allocate_string(h);
        if (si == ki) {
            j++;
        } else {
            for (int z = 0; z < j; z++)
                string_insert(out, k->body[z]);
            j = 0;
            string_insert(out, si);
        }
        if (j == k->length) {
            j = 0;
            //            output_split(n, out, ++ind, h, p, r, token, index, bound_index, bound_token);
            buffer_clear(out);
        }
    }
    //    if (out && buffer_length(out))
    //        output_split(n, out, ++ind, h, p, r, token, index, bound_index, bound_token);
}


static void do_length(block bk, execf n, value dest, value src, heap h, value *r)
{
    value str = lookup(r, src);
    // this probably needs implicit coersion because
    if((type_of(str) == estring_space)) {
        store(r, dest, box_float(((estring)str)->length));
        apply(n, h, r);
    } else {
        exec_error(bk, "Attempt to get length of non-string", str);     \
    }
}


void register_string_builders(table builders)
{
    table_set(functions, intern_cstring("concat"), do_concat);
    table_set(functions, intern_cstring("split"), do_split);
    table_set(functions, intern_cstring("length"), do_length);
}
