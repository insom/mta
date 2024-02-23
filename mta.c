/* (c) 2024 Aaron Brady, GPLv3 (see LICENSE) */

#define HOMEDIR "/home/insom"
#define EMAIL_ADDRESS "xeu@ve3x.eu"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <unistd.h>

#define BUFSZ 512

#define RES_NOOP 0
#define RES_QUIT 1

/* 450 for temp blocked
 * 452 out of space
 */

struct dispatchEntry {
    char *verb;
    int (*f)(char *verb, char *rest);
};

struct dispatchEntry dispatch_table[];

int handle_line(void) {
    int res = 0;
    char *line = NULL;
    size_t line_bufsz = 0;
    size_t line_length = getline(&line, &line_bufsz, stdin);
    char *original_line = line; // for freeing later

    if(line_length == -1) return RES_QUIT;

    char *verb = strsep(&line, " \r\n");

    int i = 0;
    for(;;) {
        if(dispatch_table[i].verb == NULL) {
            // did not match anything we know about.
            printf("500 uwotm8?\r\n");
            break;
        }
        if(strcasecmp(dispatch_table[i].verb, verb) == 0) {
            res = dispatch_table[i].f(verb, line);
            break;
        }
        i++;
    }
    free(original_line);
    fflush(stdout);
    return res;
}

struct utsname uts;
struct sockaddr peer;
socklen_t peersz = sizeof(peer);
char from_email[BUFSZ] = {0};
char to_email[BUFSZ] = {0};
char host[BUFSZ] = {0};

int main(int c, char **v) {
    chdir(HOMEDIR);
    uname(&uts);
    printf("220 %s ESMTP Metropolitan Transportation Authority\r\n", uts.nodename);
    fflush(stdout);
    int err = getpeername(0, &peer, &peersz);

    for(;;) {
        int res = handle_line();
        if(res == RES_QUIT) {
            break;
        }
    }
    return 0;
}

int HELO(char *verb, char *rest) {
    int err = getnameinfo(&peer, peersz, host, BUFSZ, NULL, 0, NI_NUMERICHOST);
    if(err != 0) {
        strcpy(host, "unknown"); // it's cool, I can trust me.
    }
    printf("250 %s hi. [%s]\r\n", uts.nodename, host);
    return RES_NOOP;
}

int MAIL(char *verb, char *rest) {
    // rest ~= "FROM:<email@address>\s.*"
    if(strlen(rest) > 5) {
        rest += 5;
    } else {
        printf("553 no.\r\n");
        return RES_NOOP;
    }
    if(*rest == '<') rest++;
    char *email = strsep(&rest, "> \r\n");
    // copy, because the buffer we're using is about to get freed.
    strncpy(from_email, email, strlen(email));
    printf("250 cq de %s.\n", from_email);
    return RES_NOOP;
}

int RCPT(char *verb, char *rest) {
    // rest ~= "TO:<email@address>\s.*"
    if(strlen(rest) > 3) {
        rest += 3;
    } else {
        printf("553 no.\r\n");
        return RES_NOOP;
    }
    if(*rest == '<') rest++;
    char *email = strsep(&rest, "> \r\n");
    // copy for above reason.
    // todo: you're better than this.
    if(strcasecmp(EMAIL_ADDRESS, email) == 0) {
        strncpy(to_email, email, strlen(email));
        printf("250 %s de %s.\r\n", to_email, from_email);
    } else {
        printf("550 not interested.\r\n");
    }
    return RES_NOOP;
}

int DATA(char *verb, char *rest) {
    if(strlen(to_email) == 0 || strlen(from_email) == 0) {
        printf("503 upps.\r\n");
        return RES_NOOP;
    }
    printf("354 go on.\r\n");
    fflush(stdout);

    char filename[BUFSZ] = {0};
    char final_filename[BUFSZ] = {0};
    time_t t = time(NULL);
    sprintf(filename, "Maildir/tmp/%d.%d.tombstone", t, getpid());
    sprintf(final_filename, "Maildir/new/%d.%d.tombstone", t, getpid());

    FILE *mb = fopen(filename, "w");
    fprintf(mb, "Return-path: <%s>\n", from_email);
    fprintf(mb, "Envelope-to: %s\n", to_email);
    fprintf(mb, "Received: from %s\n", host);

    char *line = NULL;
    size_t line_bufsz = 0;
    for(;;) {
        size_t line_length = getline(&line, &line_bufsz, stdin);
        if(line_length == -1) return RES_QUIT;
        if(strcasecmp(line, ".\r\n") == 0) {
            printf("200 yuhuh.\r\n");
            fclose(mb);
            rename(filename, final_filename);
            goto end;
        } else {
            char *orig_line = strsep(&line, "\r\n");
            line = orig_line;
            if(*orig_line == '.') orig_line++;
            fprintf(mb, "%s\n", orig_line);
        }
    }

end:
    if(line != NULL) free(line);
    bzero(from_email, BUFSZ);
    bzero(to_email, BUFSZ);
    return RES_NOOP;
}

int QUIT(char *verb, char *rest) {
    printf("220 bye.\r\n");
    return RES_QUIT;
}

struct dispatchEntry dispatch_table[] = {
    {"HELO", HELO}, {"EHLO", HELO},
    {"MAIL", MAIL},
    {"RCPT", RCPT},
    {"DATA", DATA},
    {"QUIT", QUIT},
    {NULL, NULL},
};
