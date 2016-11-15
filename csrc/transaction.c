#include <runtime.h>

typedef struct transaction {
    bag b;
    edb proposed;
}*transaction;

typedef struct peer {
    table transactions;
}*peer;

typedef struct coordinator {
    table peers;
}*coordinator;


// reason for cancel?
void prepare_transaction(coordinator c, station peer, closure(status, boolean))
{
    // if we dont think we're the coordinator for this bag, cancel
    transaction t; 
}

void commit_transaction(coordinator c, station peer, closure(status, boolean))
{
    // if we dont have this transaction then punt
    edb_foreach() {
    }

}



