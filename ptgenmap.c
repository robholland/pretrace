#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include "pretrace.h"
#include "trie.h"

/* this program will read the libpretrace configuration file, and parse it into a format that can be machine read
 * and searched very quickly.
 * 
 * This will reduce the impact on initialisation time of using pretrace.
 */

int main(int argc, char **argv)
{
    FILE *config;
    trie_t trie;
    char line[LINE_MAX], app[PATH_MAX], debugger[ARG_MAX], *c;
    unsigned short perc;
    unsigned short fields;

    if (trieinit__pt(&trie) != 0) {
        fprintf(stderr, "error: there was a problem with memory allocation.\n");
        return 1;
    }

    if ((config = fopen(PRETRACECONF, "r"))) {
        while (fgets(line, sizeof(line), config) != NULL) {
            if (*line == '#' || *line == '\n')
                continue;       /* skip comments */

            if ((c = strchr(line, ':')))
                *c = ' ';

            if ((fields = sscanf(line, "%" xstr(PATH_MAX) "s %[^\n]" xstr(ARG_MAX) "s", app, debugger)) < 1)
                continue;

            /* check for percentage */
            if ((c = strchr(app, '%')))
                *(c++) = '\0', perc = atoi(c);
            else
                perc = 100;

            eprintf("debug: adding %s, debugger: %s, percentage: %u\n", app, debugger, perc);

            if (trieadd__pt(app, perc, fields == 2 ? debugger : DEFAULT_DEBUGGER, &trie) != 0) {
                fprintf(stderr, "error: there was a problem constructing the map structure.\n");
                return 1;
            }
        }

        fclose(config);
    } else {
        fprintf(stderr, "error: could not open configuration file %s.\n", PRETRACECONF);
        return 1;
    }

    if (triewrite__pt(PRETRACEMAP, &trie) != 0) {
        fprintf(stderr, "error: failed to create map file %s.\n", PRETRACEMAP);
        return 1;
    }

    fprintf(stdout, "info: %s created successfully.\n", PRETRACEMAP);

    triedestroy__pt(&trie);

    return 0;
}
