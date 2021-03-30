CC = gcc
CFLAGS = -Werror -Wall -g --std=c99 -pedantic -pedantic-errors
CFLAGS += `pkg-config --cflags glib-2.0` -O3 -g -D_POSIX_C_SOURCE=200809L
CFLAGS += -DPRINT_MERGED_HOPS #-DDBG_DIJKSTRA
LDFLAGS = `pkg-config --libs glib-2.0` -lm -lpthread

SRC = $(wildcard *.c)
HEADERS = $(wildcard *.h)
OUTPUT = plastiroute

all: plastiroute

plastiroute: $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(SRC) -o $(OUTPUT) ${LDFLAGS}

memtest: all
	G_DEBUG=gc_friendly G_SLICE=always-malloc valgrind --leak-check=full ./$(OUTPUT) -n tests/OuterProduct/node.csv -l tests/OuterProduct/link.csv  -i 2

time: all
	time ./$(OUTPUT) -n tests/OuterProduct/node.csv -l tests/OuterProduct/link.csv

test: all
	./$(OUTPUT) -n tests/OuterProduct/node.csv -l tests/OuterProduct/link.csv -g test.dot -s0 -x1 -o test.place -v test.csv -k OuterProduct -i 10  -p 100 -f 1
	dot -Kfdp -Tpdf test.dot > test.pdf

init-test: all
	./$(OUTPUT) -n tests/OuterProduct/node.csv -l tests/OuterProduct/link.csv -g test.dot -s0 -x1 -o test.place -i 100 -p 10
	gdb --args ./$(OUTPUT) -n tests/OuterProduct/node.csv -l tests/OuterProduct/link.csv -g test.dot -s0 -x1 -o test.place -b test.place -i 10 -p 100 -f 1


.PHONY: clean
clean:
	rm -f $(OUTPUT)
