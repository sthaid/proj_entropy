#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

typedef struct {
    char * buff;
    size_t len;
} curl_buff_t;

#if 0
size_t header_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    int i;

    printf("HC ptr %p size %ld nmemb %ld user %p\n",
       ptr, size, nmemb, userdata);

    for (i = 0; i < size*nmemb; i++) {
        putchar(ptr[i]);
    }

    return size*nmemb;
}
#endif

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
#if 0
    int i;

    printf("WC ptr %p size %ld nmemb %ld user %p\n",
       ptr, size, nmemb, userdata);

    for (i = 0; i < size*nmemb; i++) {
        putchar(ptr[i]);
    }
#endif
    curl_buff_t * b   = userdata;
    size_t        len = size * nmemb;

    b->buff = realloc(b->buff, b->len + len + 1);
    memcpy(b->buff + b->len, ptr, len);
    b->len += len;
    b->buff[b->len] = '\0';

    return len;
}


int main(int argc, char **argv)
{
    CURL        * curl;
    CURLcode      res;
    curl_buff_t   b;
    char        * s, * tmp, * s_next, * filename;

    curl = curl_easy_init();
    if (curl == NULL) {
        printf("curl_easy_init failed\n");
        exit(1);
    }

    b.buff = NULL;
    b.len = 0;

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &b);
    curl_easy_setopt(curl, CURLOPT_URL, "http://wikiscience137.sthaid.org/public/sim_gravity/");
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        printf("RES %d\n", res);
        exit(1);
    }

    // read lines from buff
    for (s = b.buff; *s; s = s_next) {
        // find newline and replace with 0, and set s_next
        tmp = strchr(s, '\n');
        if (tmp != NULL) {
            *tmp = 0;
            s_next = tmp + 1;
        } else {
            s_next = s + strlen(s);
        }

        // print the line
        printf("XXX %s\n", s);

        // <a href="file1">file1</a>

        if (strncmp(s, "<a href=\"", 9) == 0) {
            filename = s+9;
            tmp = strchr(filename, '\"');
            if (tmp == NULL) {
                printf("ERROR no closing quote\n");
                continue;
            }
            *tmp = '\0';
            printf("FILENAME %s\n", filename);
        }
    }


#if 0



        //curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        //curl_easy_setopt(curl, CURLOPT_HEADERDATA, chunk);

        printf("----------------------------------\n");
        printf("RES %d\n", res);

        printf("----------------------------------\n");
        curl_easy_setopt(curl, CURLOPT_URL, "http://wikiscience137.sthaid.org/public/sim_gravity/file1");
        res = curl_easy_perform(curl);
        printf("RES %d\n", res);
#endif

    curl_easy_cleanup(curl);

    return 0;
}
