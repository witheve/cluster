#include <runtime.h>
#include <http/http.h>
// rfc 2616

struct http_server {
    heap h;
    bag b;
    table sessions;
};

typedef struct session {
    // the evaluator is throwing away our headers,
    // so we stash them here and cant execute piplined or
    // out of order -- xx fix
    edb last_headers;
    uuid last_headers_root;

    heap h;
    uuid self;
    http_server parent;
    endpoint e;
} *session;

static CONTINUATION_1_3(dispatch_request, session, edb, uuid, register_read);

void http_send_response(http_server s, edb b, uuid root)
{
    edb shadow = (edb)b;
    estring body;
    session hs = table_find(s->sessions, root);
    // xxx - root should be per-request, not per-connection

    // type checking or coersion
    value response = lookupv((edb)b, root, sym(response));
    if (hs && response) {
        value header = lookupv((edb)b, response, sym(header));

        if ((body = lookupv((edb)b, response, sym(content))) && (type_of(body) == estring_space)) {
            // dont shadow because http header can't handle it because edb_foreach
            //                shadow = (bag)create_edb(hs->h, 0, build_vector(hs->h, s));
            edb_insert(shadow, header, sym(Content-Length), box_float(body->length), 0); 
        }

        http_send_header(hs->e->w, shadow, header,
                         sym(HTTP/1.1),
                         lookupv((edb)shadow, response, sym(status)),
                         lookupv((edb)shadow, response, sym(reason)));
        if (body) {
            // xxx - leak the wrapper
            buffer b = wrap_buffer(hs->h, body->body, body->length);
            apply(hs->e->w, b, ignore);
        }

        // xxx - if this doesn't correlate, we wont continue to read from
        // this connection
        apply(hs->e->r, request_header_parser(s->h, cont(s->h, dispatch_request, hs)));
    }
}


static void dispatch_request(session s, edb b, uuid i, register_read reg)
{
    buffer *c;

    if (b == 0){
        // tell evie?
        prf ("http connection shutdown\n");
        destroy(s->h);
        return;
    }

    edb event = create_edb(s->h, build_vector(s->h, b));
    uuid x = generate_uuid();

    // multi- sadness
    s->last_headers = b;
    s->last_headers_root = i;
    table_set(s->parent->sessions, x, s);

    edb_insert(event, x, sym(tag), sym(http-request), 0);
    edb_insert(event, x, sym(request), i, 0);
    edb_insert(event, x, sym(connection), s->self, 0);

    // should be a commit i think
    //    inject_event(s->parent->ev, event);
    s->e->r = reg;
}

CONTINUATION_1_2(new_connection, http_server, endpoint, station);
void new_connection(http_server s,
                    endpoint e,
                    station peer)
{
    heap h = allocate_rolling(tcontext()->page_heap, sstring("connection"));
    session hs = allocate(h, sizeof(struct session));
    hs->parent = s;
    hs->h = h;
    hs->e = e;
    hs->self = generate_uuid();
    table_set(s->sessions, hs->self, hs);

    // as it stands, no one really cares about new connects arriving,
    // but it seems at minumum we might want to log and keep track
    apply(e->r, request_header_parser(h, cont(h, dispatch_request, hs)));
}

static CONTINUATION_2_3(http_eval_result, http_server, process_bag, uuid,
                        multibag, boolean);
static void http_eval_result(http_server s,
                             process_bag pb,
                             uuid where,
                             multibag t,
                             boolean status)
{
    // dig out of t?
    edb b;

    edb_foreach_ev((edb)b, e, sym(response), response){
        http_send_response(s, b, e);
        return;
    }
    // used to be a websocket upgrade here
}




http_server create_http_server(station p, bag over)
{
    heap h = allocate_rolling(pages, sstring("server"));
    http_server s = allocate(h, sizeof(struct http_server));

    s->h = h;

    s->sessions = create_value_table(h);

    bag sib = (bag)create_edb(h, 0);
    uuid sid = generate_uuid();

    tcp_create_server(h,
                      p,
                      cont(h, new_connection, s),
                      ignore);
    return(s);
}
