## to download and install a dedicated copy of luajit:

BUILD = $(CURDIR)


PLATFORM = $(shell uname)

ifeq ($(PLATFORM), Darwin)
  # revisit the address space layout post-lua
  LINK += -pagezero_size 10000 -image_base 100000000 -Wl,-no_pie
  INCLUDES += --include ../csrc/unix/darwin/darwin.h
  S = _
endif

ifeq ($(PLATFORM), Linux)
  LINK += -Wl,-E -lpthread
  INCLUDES += --include ../csrc/unix/linux/linux.h
endif

all: eve

VPATH += ..
VPATH += ../csrc/unix
VPATH += ../evesrc
VPATH += ../csrc/core
VPATH += ../csrc/http
VPATH += ../csrc/crypto
VPATH += ../csrc/dns
VPATH += ../csrc

CFLAGS = -g

opt: eve
opt: CFLAGS = -O2

CC = cc -std=gnu99 $(CFLAGS)

COMPILER = compiler.o error.o fs.o parser.o util.o set.o color.o build.o implicationResolver.o db.o
UNIX =  unix_util.o region.o select.o tcp_socket.o process.o page.o udp.o thread.o
UNIX_H = unix_internal.h unix.h
CORE = buffer.o table.o vector.o format.o rolling.o string.o pqueue.o timer.o field.o
CORE_H = bswap.h pqueue.h vector.h buffer.h heap.h timer.h continuation.h core.h string.h table.h atomic.h
EXEC = exec_expression.o exec_string.o exec_aggregate.o exec.o
EVE = runner.o envbag.o staticdb.o postgres.o runner.o filebag.o $(EXEC) udpbag.o
RUNTIME = init.o uuid.o estring.o types.o path.o serialize.o log.o edb.o station.o $(UNIX) $(CORE)
RUNTIME_H = runtime.h estring.h number.h types.h exec.h edb.h multibag.h thread.h serialize.h
HTTP = server.o websocket.o base64.o sha1.o json.o client.o http.o hmac_sha2.o sha256.o md5.o
DNS = dns.o dnsbag.o
HTTP_H = http.h

contgen: contgen.c
	$(CC) $< -o $@

continuation_templates.h: contgen
	$(BUILD)/contgen 10 10 > continuation_templates.h

path.c: Makefile
	echo 'char *pathroot = "$(BUILD)/../";' > path.c

package: package.o path.o $(RUNTIME) 
	$(CC) $^ -lm -lpthread -o $@

staticdb: $(DB_INCLUDES) package $(BUILD)/../bootstrap
	$(BUILD)/package < $(BUILD)/../bootstrap > staticdb

staticdb.o: staticdb
	$(CC) wrap.S -DSTART=$(S)db_start -DEND=$(S)db_end -DFILE='"$<"' -c -o $@

%.o: %.c continuation_templates.h $(UNIX_H) $(CORE_H) $(RUNTIME_H) $(HTTP_H)
	$(CC) -I../csrc/unix -I../csrc/core -I../csrc -I. $(INCLUDES) -c $<

eve: $(UTF) $(HTTP) $(RUNTIME) $(DNS) $(EVE) eve.o
	$(CC) $^ $(LINK) -lm -ldl  -o $@

clean:
	rm -f *.o eve continuation_templates.h db contgen

distclean:
	make clean ; rm -rf lua luajit-2.0
