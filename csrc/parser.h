
// a new_parser binds a closure and a result, and returns
// a parser
typedef closure new_parser;
typedef closure result;
typedef closure parser; // (parser_handler, result, token)
typedef closure parser_handler; // (parser, token)


#define parse_failed false

#include <utility/wrap.h>
   
/*
 * sequence (and)
 */

// what about next token?
#define error_exit(__c, __v)\
    if (!__v) {papply(__c, __v); return;}


// sequence shouldn't construct its results, eliminiating
// the need for sequence extract..use result filters instead

#define sequence(__h, ...)                                       \
    closure(__h, sequence_internal, __h,                         \
            set_type(build_vector_internal(__h, ##__VA_ARGS__,   \
                                           END_OF_ARGUMENTS),    \
                     t_fifo))                                     \


static inline void each_sequence();
  
static inline parser next_sequence(heap h,
                                   fifo terms, 
                                   result r,
                                   vector intermediate)
{
    result e = closure(h, each_sequence, h, terms, intermediate, r);
    new_parser p = pop(terms);
    if (p == EMPTY) return(apply(r, intermediate));
    return(reclosure(h, p, e));
}


// last element of v is the result function
static inline void sequence_internal(heap h,
                                     vector terms, 
                                     result done,
                                     parser_handler next,
                                     value t)
{
    papply(next_sequence(h, terms, done, 
                         allocate_fifo(h)), 
           next,
           t);
}

static inline void each_sequence(heap h, 
                                 vector terms,
                                 vector intermediate,
                                 result r, 
                                 parser_handler next_token,
                                 value v)
{
    error_exit(r, v);
    push(intermediate, v);
    // wtf?
    closure s = next_sequence(h, terms, r, intermediate);
    papply(next_token, s);
}

static void extract_done(result done,
                         u32 index, 
                         parser_handler next,
                         value v)
{
    apply(done, next, v?get(v, index):v);
}


static void extract_first(heap h,
                          vector terms,
                          u32 count,
                          result done,
                          parser_handler next,
                          value c)
{
    result e = closure(h, extract_done, done, count);
    sequence_internal(h, terms, e, next, c);
}

#define sequence_extract(__h, __i, ...)\
    ({                                  \
        vector terms = build_vector_internal(__h, ##__VA_ARGS__,        \
                                             END_OF_ARGUMENTS);         \
        u32 count = box_u32(__h, __i);                                  \
        set_type(terms, t_fifo);                                        \
        new_parser n = closure(__h, extract_first, __h,           \
                               terms, count);\
        n;\
     })


/*
 * optional
 */
static inline void optional_end(result done,
                                value passed, 
                                parser_handler next,
                                value v)
{
    apply(done, next, v?v:passed);
}


static inline void optional(heap h,
                            value def,
                            new_parser k,
                            result done,
                            parser_handler next,
                            value t)
{
    apply(wrap_unwind(h, k, 
                      closure(h, optional_end, done, def)),
          next, t);
}
                        

/*
 * list of items separated by a fixed token
 *
 * letrec would make this pretty trivial to
 * construct
 */

static void repeat_sep_each_done (heap h,
                                  vector r,
                                  new_parser separator,
                                  new_parser n,
                                  result done, 
                                  closure myself, 
                                  parser_handler next,
                                  value v)
{
    error_exit(v, done);

    push(r, v);
    closure b = sequence_extract(h, 1, separator, n);
    apply(next, closure(h, optional, h, false, b, myself));
}

static inline void repeat_separator(heap h,
                                    new_parser k,
                                    new_parser separator,
                                    result done,
                                    parser_handler next,
                                    value v)
{
    closure d = closure(h, repeat_sep_each_done,
                        h, 
                        allocate_vector(h),
                        separator,
                        k, 
                        done,
                        MYSELF);
    
    optional(h, false, k, d, next, v);
}

static void repeat_each_done (heap h,
                              vector r,
                              new_parser n,
                              result done, 
                              closure myself, 
                              parser_handler next, 
                              value v)
{
    error_exit(done, v);
    push(r, v);
    apply(next, closure(h, optional, h, false, n, myself));
}

static void repeat(heap h,
                   new_parser k,
                   result done,
                   parser_handler next,
                   value v)
{

    closure d = closure(h, repeat_each_done,
                        h, 
                        allocate_vector(h),
                        k, 
                        done,
                        MYSELF);
    
    optional(h, false, k, d, next, v);
}
  
/*
 * oneof
 */

static void oneof_internal();

static void oneof_endbranch(heap h, 
                            vector terms,
                            result done, 
                            parser_handler next,
                            value v)
{
    if (v == parse_failed) {
        apply(next, closure(h, oneof_internal, h, terms, done));
    } else {
        apply(done, next, v);
    } 
}

static void oneof_internal(heap h,
                           vector terms,
                           result done,
                           parser_handler next,
                           ctoken ct)
{
    new_parser i;
    
    if ((i = pop(terms)) == EMPTY) {
        apply(done, next, parse_failed);
    } else {
        apply(wrap_unwind(h, i, 
                          closure(h, oneof_endbranch, h, terms, done)),
              next, 
              ct);
    }
}

#define oneof(__h, ...)                                    \
    make_closure(__h, oneof_internal, __h,                 \
                 build_vector_internal(__h, ##__VA_ARGS__, \
                                       END_OF_ARGUMENTS),  \
                 END_OF_ARGUMENTS)                         \


/*
 * shim for result wrapping
 *
 * - this seems to be identity?
 */
static inline void apply_filter(result filter, 
                                result done, 
                                value v)
{
    if(v) 
        apply(filter, done, v);
    else
        apply(done, v);
}

static inline void result_filter(heap h,
                                 result filter,
                                 new_parser n, 
                                 result done, 
                                 parser_handler next,
                                 value c)
{
    papply(n,
           closure(h, apply_filter, filter, done), 
           next, 
           c);
}

buffer_handler allocate_dispatch(heap h, parser start);
