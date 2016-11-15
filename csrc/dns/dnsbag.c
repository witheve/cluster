#include <runtime.h>

typedef struct dns_bag {
    struct bag b;
    heap h;
} *dns_bag;


CONTINUATION_0_5(dns_scan, int, listener, value, value, value);
void dns_scan(int sig, listener out, value e, value a, value v)
{
    if (sig & e_sig) {
    }

    if (sig & a_sig) {

    }
    if (sig & v_sig) {
    }
}

bag create_dns_bag(estring resolver)
{
    heap h = allocate_rolling(pages, sstring("dns bag"));
    dns_bag b = allocate(h, sizeof(struct dns_bag));
    b->h = h;
    return (bag)b;
}
