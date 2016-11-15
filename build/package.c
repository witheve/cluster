#include <unix_internal.h>
#include <bswap.h>

table object_map; 
heap working;
buffer out;

static char hex_digit[]={'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

static void owrite(byte *x, bytes length)
{
    // add buffering
    write(1, x, length);
}


static inline void bwrite(byte x)
{
    owrite(&x, 1);
}


#define pad(x, y) (((((x)-1)/(y)) +1) *(y))

static inline void encode_integer(int offset, byte base, u64 value)
{
    int len = first_bit_set(value) + 1;
    int space = 7 - offset;
    byte out = base ;
    int total = pad(len + offset, 7);
    total -= offset;

    while (total > 0) {
        bwrite(out | ((total > space)?(1<<space):0) | bitstring_extract(value, total, space));
        total -= space;
        space = 7;
        out = 0;
    }
}


static void write_string(byte *body, int length)
{
    encode_integer(2, 0x40, length);
    owrite(body, length);
}

extern char *pathroot;

void *memcpy(void * dst, const void *src, size_t n);

static void include_file(byte *name, int length)
{
    struct stat st;
    char err[]= "file not found ";
    char t[PATH_MAX];
    int plen = 0;
    for (char *i = pathroot; *i; i++, plen++);
    memcpy(t, pathroot, plen);
    memcpy(t+plen, name, length);
    t[plen+length]=0;

    int fd = open(t, O_RDONLY);
    if (fd <= 0) {
        write(2, err, sizeof(err) - 1);
        write(2, t, plen + length);
        write(2, "\n", 1);
        exit(-1);
    }

    fstat(fd, &st);
    int flen = st.st_size;
    // encode the length
    void *buf = alloca(flen);
    read(fd, buf, flen);
    write_string(buf, flen);
    close(fd);
}

void *memset(void *b, int c, size_t len);

static u64 uuid_count;

// we can start trying to use the proper serializer
static void write_term(byte *x, int length)
{
    char start = x[0];

    if (start == '%') {
        string s = allocate_buffer(working, length-1);
        buffer_append(s, x+1, length -1);
        u8 *key;
        if (!(key = table_find(object_map, s))) {
            key = allocate(working, UUID_LENGTH);
            memset(key, 0, UUID_LENGTH);
            u64 id = uuid_count++;
            id = hton_64(id);
            memcpy(key+UUID_LENGTH-sizeof(u64), &id, sizeof(u64));
            key[0] |= 0x80;
            table_set(object_map, s, key);
        }
        owrite(key, UUID_LENGTH);
        return;
    }

    if (((start >= '0') && (start <= '9')) || (start == '-')) {
        double d = parse_float(alloca_wrap_buffer(x, length));
        bwrite(float64_prefix);
        u64 k = *(u64*)&d;
        k = hton_64(k);
        owrite((u8 *)&k, sizeof(double));
        return;
        // xxx - little endian
    }


    if (start == '{') {
        include_file(x+1, length-2);
        return;
    }

    write_string(x, length);
}

int main()
{
    // xxx - move to core
    heap smallpages = init_memory(4096);
    pthread_key_create(&pkey, 0);
    primary = init_context(smallpages);
    pthread_setspecific(pkey, primary);

    int fill = 0;
    int comment = 0;
    byte term[1024];
    char x;

    working = allocate_rolling(smallpages, sstring("package"));
    object_map = allocate_table(working, string_hash, string_equal);
    buffer out = allocate_buffer(working, 1024);

    while (read(0, &x, 1) > 0 ) {
        if (x == '#') comment = 1;

        if (comment && (x != '\n'))  continue;

        comment = 0;

        if ((x == ' ') || (x == '\n')) {
            if (fill) {
                write_term(term, fill);
                fill = 0;
            }
        } else term[fill++] = x;
    }
}
