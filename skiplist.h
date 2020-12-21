#include <stdlib.h>

#define SKIPLIST_MAXLEVEL 32
#define SKIPLIST_P 0.25

#define GT 1
#define EQ 0
#define LT -1

typedef struct slobj {
    char *ptr;
    size_t length;
} slobj;

typedef int (*comp_mm)(void* ud, slobj* p1, slobj* p2);
typedef int (*comp_ms)(void* ud, slobj* p1, int p2);
typedef int (*comp_ss)(void* ud, int p1, int p2);

typedef struct skiplistNode {
    slobj* obj;
    struct skiplistNode *backward;
    struct skiplistLevel {
        struct skiplistNode *forward;
        unsigned int span;
    }level[];
} skiplistNode;

typedef struct skiplist {
    struct skiplistNode *header, *tail;
    unsigned long length;
    int level;
} skiplist;

typedef void (*slDeleteCb) (void *ud, slobj *obj);
slobj* slCreateObj(const char* ptr, size_t length);
void slFreeObj(slobj *obj);

skiplist *slCreate(void);
void slFree(skiplist *sl);
void slDump(skiplist *sl);

void slInsert(skiplist *sl, slobj *obj, comp_mm comp, void* ud);
int slDelete(skiplist *sl, slobj *obj, comp_mm comp, void* ud);
unsigned long slDeleteByRank(skiplist *sl, unsigned int start, unsigned int end, slDeleteCb cb, void* ud);

unsigned long slGetRank(skiplist *sl, slobj *o, comp_mm comp, void* ud);
skiplistNode* slGetNodeByRank(skiplist *sl, unsigned long rank);

skiplistNode *slFirstInRange(skiplist *sl, int min_index, int max_index, comp_ms comp, void* ud);
skiplistNode *slLastInRange(skiplist *sl, int min_index, int max_index, comp_ms comp, void* ud);

