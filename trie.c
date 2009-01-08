#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#include "trie.h"

/* initialise the trie structure at *trie, return 0 on success */
int trieinit__pt(trie_t * trie)
{
    trie_t ntrie = {
        MAPMAGIC,               /* magic */
        1,                      /* cnt */
        0,                      /* ldebug */
        NULL,                   /* root */
        NULL                    /* debug */
    };

    memcpy(trie, &ntrie, sizeof(trie_t));

    if ((trie->root = malloc(sizeof(subtrie_t))) == NULL)
        return -1;

    /* initialise root node to zero */
    memset(trie->root, 0x00, sizeof(subtrie_t));

    return 0;
}

/* insert the application app into the trie trie, and associate it with debugger debugger.
 * 
 * returns 0 on success.
 * 
 */
int trieadd__pt(const char *app, unsigned short percent, const char *debugger, trie_t * trie)
{
    unsigned off = 0;           /* offset from root of current node */
    const char *c;

    /* for every character in application name, traverse to a deeper level in the trie */
    for (c = app; *c != '\0'; c++) {

        /* check if the next node has already been created, ie an existing node shares the 
         * same prefix to at least this point in the string app 
         */
        if ((trie->root + off)->sublevel[index(*c)] == 0) {

            /* allocate space for new node, and check for allocation error */
            if ((trie->root = realloc(trie->root, sizeof(subtrie_t) * ++(trie->cnt))) == NULL)
                return -1;

            /* save the offset of this new node */
            (trie->root + off)->sublevel[index(*c)] = (trie->cnt - 1);

            /* initialise everything to zero */
            memset(trie->root + (trie->root + off)->sublevel[index(*c)], 0x00, sizeof(subtrie_t));
        }

        /* traverse to next node */
        off = (trie->root + off)->sublevel[index(*c)];
    }

    /* at this point we have traversed the trie down to the NUL terminator in the appname.
     * so this is the end node, create a string for debugger name, and store offset into
     * string table in node
     */

    /* make room in string table */
    trie->debug = realloc(trie->debug, trie->ldebug + strlen(debugger) + 1);

    /* copy debugger into freshly allocated space */
    strcpy(trie->debug + trie->ldebug, debugger);

    /* store offset into table into node */
    (trie->root + off)->ndebug = trie->ldebug;

    /* store percentage */
    (trie->root + off)->percent = percent;

    /* save newsize string table size into trie */
    trie->ldebug += strlen(debugger) + 1;

    return 0;
}

/* search trie trie for application app, returns the debugger associated with app if 
 * found, or NULL if not known. percentage will be placed in *percent.
 * 
 */
char *trielookup__pt(const char *app, unsigned short *percent, trie_t * trie)
{
    subtrie_t *currnode = trie->root;
    const char *c;

    /* c points to the current character of app, use it to traverse down the trie until we
     * hit NUL, ie the end of the string */
    for (c = app; *c != '\0'; c++) {
        /* does the next node exist? if not, this application has not been entered into the trie */
        if (currnode->sublevel[index(*c)] == 0) {
            /* app has obviously not been stored in this trie, you lose. */
            return NULL;
        } else {
            /* traverse down to the next level */
            currnode = trie->root + currnode->sublevel[index(*c)];
        }
    }

    /* if we get here, there is a corresponding app stored in the trie */

    /* save the percentage */
    *percent = currnode->percent;

    /* return the debugger string */
    return trie->debug + currnode->ndebug;
}

/* save the data structure at *trie to filename, returns 0 on success, < 0 on error */
int triewrite__pt(const char *filename, trie_t * trie)
{
    FILE *map;

    if ((map = fopen(filename, "w")) != NULL) {
        /* write out data about the trie */
        fwrite(trie, sizeof(trie_t), 1, map);
        fwrite(trie->root, sizeof(subtrie_t), trie->cnt, map);
        /* append string table */
        fwrite(trie->debug, 1, trie->ldebug, map);
        fclose(map);
        return 0;
    }

    return -1;
}

/* open and mmap() filename, parseing into trie.
 * returns size of filename, which can be thought of as a descriptor as it must be used when munmapping via trieclose__pt */
int trieopen__pt(const char *filename, trie_t * trie)
{
    int mapfile;
    struct stat mapstat;
    char *map;

    if ((mapfile = open(filename, O_RDONLY)) != -1) {
        if (fstat(mapfile, &mapstat) == -1)
            return -4;

        if ((map = mmap(NULL, mapstat.st_size, PROT_READ, MAP_PRIVATE, mapfile, 0)) == (void *) -1)
            return -2;

        /* read in trie structure */
        memcpy(trie, map, sizeof(trie_t));

        /* verify this is a valid map file */
        if (trie->magic != MAPMAGIC) {
            munmap(map, mapstat.st_size);
            return -3;
        }

        trie->root = (subtrie_t *) (map + sizeof(trie_t));
        trie->debug = map + sizeof(trie_t) + (sizeof(subtrie_t) * trie->cnt);

        return mapstat.st_size;
    }

    return -1;
}

/* unmap *map */
int trieclose__pt(trie_t * trie, size_t length)
{
    return munmap(((void *) trie->root) - sizeof(trie_t), length);
}

/* this is called for a trie created via trieinit__pt, it releases any allocated memory.
 * should not be used for trie's created via trieopen!
 */
void triedestroy__pt(trie_t * trie)
{
    free(trie->debug);
    free(trie->root);

    /* zero out trie so dererfences show up */
    memset(trie, 0x00, sizeof(trie_t));
}
