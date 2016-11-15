#include <runtime.h>
#include <math.h>

table functions;

static value toggle (value x)
{
    if (x == efalse) return etrue;
    return efalse;
}

static void do_equal(block bk, execf n, value a, value b, heap h, value *r)
{
    value ar = lookup(r, a);
    value br = lookup(r, b);
    if (!value_equals(ar, br)) return;
}

#define DO_UNARY_TRIG(__name, __op)                                                                           \
    static void __name (block b, value dest, value a, heap h,  value *r)             \
    {                                                                                                         \
        value ar = lookup(r, a);                                                                              \
        if ((type_of(ar) != float_space )) {                                                                  \
            exec_error(b, "attempt to do math on non-number", a);  \
        } else {                                                                                              \
            double deg = *(double *)ar;                                                                       \
            double rad = deg * M_PI/180.0;                                                                    \
            r[reg(dest)] = box_float(__op(rad));                                                              \
        }                                                                                                     \
    }

#define DO_UNARY_BOOLEAN(__name, __op)                                                                        \
    static void __name (block b, execf n, value dest, value a, heap h,  value *r)             \
    {                                                                                                         \
        value ar = lookup(r, a);                                                                              \
        if ((ar != etrue) && (ar != efalse)) {                                                                \
            exec_error(b, "attempt to flip non boolean", a);                                              \
        } else {                                                                                              \
            r[reg(dest)] = __op(ar);                                                                          \
        }                                                                                                     \
    }


#define DO_UNARY_NUMERIC(__name, __op)                                                                        \
    static void __name (block bk, execf n, value dest, value a, heap h,   value *r)           \
    {                                                                                                         \
        value ar = lookup(r, a);                                                                              \
        if ((type_of(ar) != float_space )) {                                                                  \
            exec_error(bk, "attempt to do math on non-number", a);                                        \
        } else {                                                                                              \
            r[reg(dest)] = box_float(__op(*(double *)ar));                                                    \
        }                                                                                                     \
    }


#define DO_BINARY_NUMERIC(__name, __op)                                                                        \
    static void __name (block bk, execf n, value dest, value a, value b, heap h,  value *r)    \
    {                                                                                                          \
        value ar = lookup(r, a);                                                                               \
        value br = lookup(r, b);                                                                               \
        if ((type_of(ar) != float_space ) || (type_of(br) != float_space)) {                                   \
            exec_error(bk, "attempt to " #__name " non-numbers", a, b);                                    \
            prf("UHOH %v, %v\n", ar, br);                                                                      \
        } else {                                                                                               \
            r[reg(dest)] = box_float(*(double *)ar __op *(double *)br);                                        \
        }                                                                                                      \
    }

#define DO_BINARY_BOOLEAN(__name, __op)                                                                        \
    static void __name (block bk, execf n, value dest, value a, value b, heap h,   value *r)   \
    {                                                                                                          \
        value ar = lookup(r, a);                                                                               \
        value br = lookup(r, b);                                                                               \
                                                                                                               \
        if ((type_of(ar) == float_space ) && (type_of(br) == float_space)) {                                   \
            r[reg(dest)] = (*(double *)ar __op *(double *)br) ? etrue : efalse;                                \
        } else if ((type_of(ar) == estring_space ) && (type_of(br) == estring_space)) {                        \
            r[reg(dest)] = (ar __op br) ? etrue : efalse;                                                      \
        } else if ((type_of(ar) == uuid_space ) && (type_of(br) == uuid_space)) {                              \
            r[reg(dest)] = (ar __op br) ? etrue : efalse;                                                      \
        } else if ((ar == etrue || ar == efalse ) && (br == etrue || br == efalse)) {                          \
            r[reg(dest)] = (ar __op br) ? etrue : efalse;                                                      \
        } else {                                                                                               \
            exec_error(bk, "attempt to " #__op " different types", a, b);                                  \
        }                                                                                                      \
    }


DO_UNARY_TRIG(do_sin, sin)
DO_UNARY_TRIG(do_cos, cos)
DO_UNARY_TRIG(do_tan, tan)
DO_UNARY_BOOLEAN(do_toggle, toggle)
DO_UNARY_NUMERIC(do_floor, floor)
DO_UNARY_NUMERIC(do_ceil, ceil)
DO_UNARY_NUMERIC(do_round, round)
DO_BINARY_NUMERIC(do_plus, +)
DO_BINARY_NUMERIC(do_minus, -)
DO_BINARY_NUMERIC(do_multiply, *)
DO_BINARY_NUMERIC(do_divide, /)
DO_BINARY_BOOLEAN(do_less_than, <)
DO_BINARY_BOOLEAN(do_less_than_or_equal, <=)
DO_BINARY_BOOLEAN(do_greater_than, >)
DO_BINARY_BOOLEAN(do_greater_than_or_equal, >=)
DO_BINARY_BOOLEAN(do_is_equal, ==)
DO_BINARY_BOOLEAN(do_not_equal, !=)

static void do_is (block bk, execf n, value dest, value a, heap h,  value *r)
{
    r[reg(dest)] = lookup(r, a);
    apply(n, h, r);
}


static void do_mod (block bk, execf n, value dest, value a, value b, heap h,  value *r)
{
    value ar = lookup(r, a);
    value br = lookup(r, b);
    if ((type_of(ar) != float_space ) || (type_of(br) != float_space)) {
        exec_error(bk, "attempt to modulo non-numbers", a, b);
    } else {
        r[reg(dest)] = box_float(fmod(*(double *)ar, *(double *)br));
        apply(n, h, r);
    }
}

static void do_abs (block bk, execf n, value dest, value a, heap h,  value *r)
{
    value ar = lookup(r, a);
    if (type_of(ar) != float_space) {
        exec_error(bk, "attempt to abs non-number", a);
        prf("UHOH %v\n", ar);
    } else {
        double val = *(double *)ar;
        r[reg(dest)] = box_float(val < 0 ? -val : val);
        apply(n, h, r);
    }
}


static void do_range(block bk, execf n, value dest, value min, value max, heap h,  value *r)
{
  value min_r = lookup(r, min);
  value max_r = lookup(r, max);
  if ((type_of(min_r) != float_space) || (type_of(max_r) != float_space)) {
    exec_error(bk, "attempt to do range over non-number(s)", min, max);
  } else {
    for(double i = *(double *)min_r, final = *(double *)max_r; i < final; i++) {
      r[reg(dest)] = box_float(i);
      apply(n, h, r);
    }
  }
}


// currently the parameters of these things are in the bootstrap database
// it would be nice if we could fix C inline data declarations
void register_exec_expression(table builders)
{
    table_set(functions, intern_cstring("plus"), do_plus);
    table_set(functions, intern_cstring("minus"), do_minus);
    table_set(functions, intern_cstring("multiply"), do_multiply);
    table_set(functions, intern_cstring("divide"), do_divide);
    table_set(functions, intern_cstring("less_than"), do_less_than);
    table_set(functions, intern_cstring("less_than_or_equal"), do_less_than_or_equal);
    table_set(functions, intern_cstring("greater_than"), do_greater_than);
    table_set(functions, intern_cstring("greater_than_or_equal"), do_greater_than_or_equal);
    table_set(functions, intern_cstring("equal"), do_equal);
    table_set(functions, intern_cstring("not_equal"), do_not_equal);
    table_set(functions, intern_cstring("sin"), do_sin);
    table_set(functions, intern_cstring("cos"), do_cos);
    table_set(functions, intern_cstring("tan"), do_tan);
    table_set(functions, intern_cstring("mod"), do_mod);
    table_set(functions, intern_cstring("abs"), do_abs);
    table_set(functions, intern_cstring("ceil"), do_ceil);
    table_set(functions, intern_cstring("floor"), do_floor);
    table_set(functions, intern_cstring("round"), do_round);
    table_set(functions, intern_cstring("toggle"), do_toggle);
    table_set(functions, intern_cstring("range"), do_range);
}
