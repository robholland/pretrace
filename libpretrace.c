/*
*
* $Author: taviso $
* $Revision: 1.1 $
* $Log: libpretrace.c,v $
*
*/

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* #define DEBUG */

#include "trie.h"
#include "pretrace.h"

/* 
 * preload library to always start certain applications under a debugging 
 * environment for example, strace, ltrace, valgrind, etc.
 *
 */

static void __attribute__ ((constructor)) init(void)
{
    char exename[PATH_MAX], cmdline[ARG_MAX], pexename[PATH_MAX];
    char **argv = NULL, *f, *debugger, procexe[64];
    int cnt, argl, mapid;
    unsigned short percent;
    unsigned r;
    trie_t trie;
    FILE *args;

    /* identify the parent */
    snprintf(procexe, sizeof(procexe), "/proc/%i/exe", getppid());

    if ((cnt = readlink(procexe, pexename, sizeof(pexename) - 1)) <= 0) {
        eprintf("debug: couldn't identify parent, return.\n");
        return;
    }
    pexename[cnt] = 0x00;

    /* identify this process */
    if ((cnt = readlink("/proc/self/exe", exename, sizeof(exename) - 1)) <= 0) {
        eprintf("debug: couldn't identify executable, return...\n");
        return;
    }
    exename[cnt] = 0x00;

    eprintf("debug: reading %s, looking for %s...\n", PRETRACEMAP, exename);

    /* check if this app should be traced */
    if ((mapid = trieopen__pt(PRETRACEMAP, &trie)) >= 0) {
        if ((debugger = trielookup__pt(exename, &percent, &trie)) != NULL) {
            eprintf("debug: match, found debugger %s.\n", debugger);
            if ((debugger = strdup(debugger)) == NULL) {
                eprintf("debug: allocation errors.\n");
            }
        }

        trieclose__pt(&trie, mapid);
    } else {
        eprintf("debug: unexpected error mapping %s, return.\n", PRETRACEMAP);
        return;
    }

    if (debugger == NULL) {
        eprintf("debug: no match found, handing return over to main.\n");
        return;
    }

    /* grab some random data */
    RANDOMIZE(r);

    if (r % 100 < percent) {
        eprintf("debug: percentage check says we run with it (%i < %i)\n", r % 100, percent);
    } else {
        eprintf("debug: percentage check says not this time (%i > %i)\n", r % 100, percent);
        return;
    }

    eprintf("debug: parsing debugger commandline...\n");

    /* check if user wanted to pass arguments to the debugger.
     * if they did, tokenize the string and parse it into **argv.
     */

    cnt = 0;

    for (f = strtok(debugger, " "); f; f = strtok(NULL, " ")) {
        eprintf("debug: \tadded argv[%i] as %s...\n", cnt, f);
        /* alloc space for another pointer */
        if ((argv = realloc(argv, sizeof(char *) * (cnt + 1))) == NULL) {
            eprintf("debug: allocation error.\n");
            return;
        }

        if ((argv[cnt] = malloc(strlen(f) + 1)) == NULL) {
            eprintf("debug: allocation error.\n");
            return;
        }

        strcpy(argv[cnt++], f);
    }

    free(debugger);

    eprintf("debug: done parsing debugger.\n");
    eprintf("debug: debugger is %s, parent is %s.\n", basename(*argv), pexename);

    /* catch infinite recursion */
    if (strcmp(*argv, pexename) == 0 || getenv("__PRETRACE_CHECK") != NULL) {
        if (getenv("__PRETRACE_CHECK"))
            eprintf("debug: env var detected.\n");

        eprintf("debug: parent is %s! *stop*\n", pexename);
        eprintf("debug: infinite recursion avoided, handing control over to main()...\n");

        unsetenv("__PRETRACE_CHECK");
        return;
    } else
        setenv("__PRETRACE_CHECK", "1", 1);

    if ((args = fopen("/proc/self/cmdline", "r"))) {
        argl = fread(cmdline, 1, ARG_MAX, args);
    } else {
        eprintf("debug: couldnt read cmdline, return.\n");
        return;
    }

    fclose(args);

    eprintf("debug: cmdline starts with %s (%i bytes total (including delimeters))\n", cmdline, argl);

    /* tokenize cmdline and append to **argv */
    for (f = (char *) &cmdline; f < cmdline + argl;) {
        if ((argv = realloc(argv, sizeof(char *) * (cnt + 1))) == NULL) {
            eprintf("debug: allocation error.\n");
            return;
        }

        eprintf("debug: \targv[%i] is %s (%i bytes)\n", cnt, f, strlen(f));

        if ((argv[cnt] = malloc(strlen(f) + 1)) == NULL) {
            eprintf("debug: allocation error.\n");
            return;
        }

        strcpy(argv[cnt++], f);
        f += strlen(f) + 1;
    }
    argv[cnt] = NULL;

    eprintf("debug: parsed args into **argv\n");
    eprintf("debug: execv (%s, **argv);\n", *argv);

    execv(*argv, argv);

    /* execv should never return, if it did, tavis broke something */
    fprintf(stderr, "error: execv(%s, **argv) failed: %s\n", *argv, strerror(errno));
    eprintf("debug: handing control over to main()..\n");

    return;
}
