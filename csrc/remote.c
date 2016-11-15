#include <runtime.h>


CONTINUATION_2_(endpoint, station);

static void connection_input()
{
    // handle event registrations
    // bag updates
    // commit coordination
}

static void remote_new_connection(endpoint e,
                                  station peer)
{
    // xxx - should identify the peer in the heap map - fix reflection
    heap h = allocate_rolling(pages, sstring("remote connection"));
    buffer_handler h = allocate_deserialize(h, cont(h, connection_input));
    
    // dump catalog
}

void start_remote_service(station p)
{
    tcp_create_server(h,
                      p,
                      cont(h, new_connection, s),
                      ignore);
}
