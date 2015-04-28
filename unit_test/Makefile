
all: barrier libcurl_test

barrier: barrier.c
	gcc -Wall -o barrier -pthread barrier.c

libcurl_test: libcurl_test.c
	gcc -Wall -o libcurl_test -lcurl libcurl_test.c

clean:
	rm libcurl_test barrier
