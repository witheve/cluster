#include <unix_internal.h>
    
typedef struct udp_bag {
    struct bag b;
    heap h;
    table channels;
} *udp_bag;


static CONTINUATION_1_5(udp_scan, udp_bag, int, listener, value, value, value);
static void udp_scan(udp_bag ub, int sig, listener out, value e, value a, value v)
{
    prf("udp scan: %v %v %v\n", e, a, v);
    if ((sig == s_eAV) && (a == sym(tag)) && (v == sym(udp))){
        table_foreach(ub->channels, e, u)
            apply(out, e, a, v, 0);
    }
    
    // ech
    if ((sig == s_eAv) && (a == sym(address)))  
        table_foreach(ub->channels, e, u)
            apply(out, e, a, udp_station(u), 0);

    
    if ((sig == s_EAv) && (a == sym(address))) {
        udp u = table_find(ub->channels, e);
        apply(out, e, a, udp_station(u), 0);
    }
}

static CONTINUATION_1_2(udp_input, udp_bag, station, buffer);
static void udp_input(udp_bag ub, station s, buffer b)
{
    prf ("packet input\n");
}

static CONTINUATION_1_4(udp_prepare, udp_bag, edb, edb, ticks, commit_handler);
// oh the shame
static void udp_prepare(udp_bag ub, edb add, edb remove, ticks t, commit_handler h)
{
    station d;
    prf("udp commit: %b\n", edb_dump(init, add));

    edb_foreach_e(add, e, sym(tag), sym(udp)) {
        unsigned int host = 0;
        int port = 0;
        edb_foreach_v(add, e, sym(port), port) {
            // fill in port if defined
        }
        edb_foreach_v(add, e, sym(host), port) {
            // fill in port if defined
        }
        prf("udp commit %d\n", table_elements(ub->b.listeners));
        udp u = create_udp(ub->h, ip_wildcard, cont(ub->h, udp_input, ub));
        table_set(ub->channels, e, u);
    }
    edb_foreach_e(add, e, sym(tag), sym(packet)) {
        edb_foreach_v(add, e, sym(destination), destination) {
            edb_foreach_v(add, e, sym(body), b) {
            }
        }
    }
}

static CONTINUATION_1_2(udp_reception, udp_bag, station, buffer);
static void udp_reception(udp_bag u, station s, buffer b)
{
    uuid p = generate_uuid();
    edb in = create_edb(u->h, 0);
    apply(deserialize_into_bag(u->h, in), b, ignore);
    // trigger an update
}

// we shouldn't need this evaluation, but first class bags and
// hackerism
bag udp_bag_init()
{
    // this should be some kind of parameterized listener.
    // we can do the same trick that we tried to do
    // with time, by creating an open read, but it
    // has strange consequences. sidestep by just
    // having an 'eve port'
    heap h = allocate_rolling(pages, sstring("udp bag"));
    udp_bag ub = allocate(h, sizeof(struct udp_bag));
    ub->h = h;
    ub->b.prepare = cont(h, udp_prepare, ub);
    ub->b.scan = cont(h, udp_scan, ub);
    ub->b.listeners = allocate_table(h, key_from_pointer, compare_pointer);
    ub->channels = create_value_table(h);
    // doctopus
    //     ub->ev = ev;
    return (bag)ub;
}
