#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netdb.h>

/*
 * RFC 5321 + 821
 * 220
 * HELO + response (or)
 * EHLO + extensions (8BITMIME?)
 * MAIL FROM
 * RCPT TO
 * DATA->354 (if good)
 * . -> 250 (if good)
 * (reset state machine?)
 * ^ also RSET->250 (go back to helo.ehlo state, if applicable)
 * NOOP, why not
 * QUIT->221
 *
 * 450 for temp blocked
 * 452 out of space
 */

/* Add received line, from header (check maildir)
 * CRLF to LF conversion?
 * Envelope-From header?
 *
 * Return-path: <bounce-..@com>
   Envelope-to: you@dot.com
   Delivery-date: Mon, 08 Jan 2024 09:01:19 +0000
   Received: from their.hostname ([ip.addr])
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

    char *verb = strsep(&line, " \r\n");

    int i = 0;
    for(;;) {
        if(dispatch_table[i].verb == NULL) {
            // did not match anything we know about.
            printf("500 uwotm8?\n");
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

#define RES_NOOP 0
#define RES_QUIT 1

struct utsname uts;
struct sockaddr peer;
socklen_t peersz = sizeof(peer);

int main(int c, char **v) {
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
    char host[512] = {0};
    int err = getnameinfo(&peer, peersz, host, 512, NULL, 0, NI_NUMERICHOST);
    if(err != 0) {
        strcpy(host, "unknown"); // it's cool, I can trust me.
    }
    printf("250 %s hi. [%s]\n", uts.nodename, host);
    return RES_NOOP;
}

int QUIT(char *verb, char *rest) {
    printf("220 bye.\n");
    return RES_QUIT;
}

struct dispatchEntry dispatch_table[] = {
    {"HELO", HELO},
    {"QUIT", QUIT},
    {NULL, NULL},
};
