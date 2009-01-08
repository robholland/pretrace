#define KEY(x) (((unsigned char) x) + 1)
#define MAPMAGIC 0xB00B1E5
#define index(x) (int)(x)

typedef struct subtrie {
    unsigned short ndebug;
    unsigned short percent;
    unsigned short sublevel[KEY(~0)];
} subtrie_t;

typedef struct trie {
    unsigned int magic;
    unsigned short cnt;
    unsigned short ldebug;
    subtrie_t *root;
    char *debug;
} trie_t;

int trieadd__pt(const char *app, unsigned short percentage, const char *debugger, trie_t * trie);
char *trielookup__pt(const char *app, unsigned short *percent, trie_t * trie);
int triewrite__pt(const char *filename, trie_t * trie);
int trieopen__pt(const char *filename, trie_t * trie);
int trieclose__pt(trie_t * trie, size_t length);
void triedestroy__pt(trie_t * trie);
int trieinit__pt(trie_t * trie);
