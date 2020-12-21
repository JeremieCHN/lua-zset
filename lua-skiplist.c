/*
 *  author: xjdrew
 *  date: 2014-06-03 20:38
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "skiplist.h"

static inline skiplist*
_to_skiplist(lua_State *L) {
    skiplist **sl = lua_touserdata(L, 1);
    if(sl==NULL) {
        luaL_error(L, "must be skiplist object");
    }
    return *sl;
}

int _L_comp_mm(void* ud, slobj* p1, slobj* p2) {
    lua_State* L = (lua_State*) ud;
    lua_pushvalue(L, -1);
    lua_pushlstring(L, p1->ptr, p1->length);
    lua_pushlstring(L, p2->ptr, p2->length);
    lua_call(L, 2, 1);
    int res = luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    return res;
}

int _L_comp_ms(void* ud, slobj* p1, int p2) {
    lua_State* L = (lua_State*) ud;
    lua_pushvalue(L, -1);
    lua_pushlstring(L, p1->ptr, p1->length);
    lua_pushvalue(L, p2 + 2);
    lua_call(L, 2, 1);
    int res = luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    return res;
}

int _L_comp_ss(void* ud, int p1, int p2) {
    lua_State* L = (lua_State*) ud;
    lua_pushvalue(L, -1);
    lua_pushvalue(L, p1 + 1);
    lua_pushvalue(L, p2 + 2);
    lua_call(L, 2, 1);
    int res = luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    return res;
}

static int
_insert(lua_State *L) {
    skiplist *sl = _to_skiplist(L);
    luaL_checktype(L, 2, LUA_TSTRING);
    size_t len;
    const char* ptr = lua_tolstring(L, 2, &len);
    slobj *obj = slCreateObj(ptr, len);

    luaL_checktype(L, 3, LUA_TFUNCTION);
    lua_pushvalue(L, 3);
    slInsert(sl, obj, _L_comp_mm, L);
    lua_pop(L, 1);
    return 0;
}

static int
_delete(lua_State *L) {
    skiplist *sl = _to_skiplist(L);
    luaL_checktype(L, 2, LUA_TSTRING);
    slobj obj;
    obj.ptr = (char *)lua_tolstring(L, 2, &obj.length);

    luaL_checktype(L, 3, LUA_TFUNCTION);
    lua_pushvalue(L, 3);
    int res = slDelete(sl, &obj, _L_comp_mm, L);
    lua_pop(L, 1);
    lua_pushboolean(L, res);
    return 1;
}

static void
_delete_rank_cb(void* ud, slobj *obj) {
    lua_State *L = (lua_State*)ud;
    lua_pushvalue(L, 4);
    lua_pushlstring(L, obj->ptr, obj->length);
    lua_call(L, 1, 0);
}

static int
_delete_by_rank(lua_State *L) {
    skiplist *sl = _to_skiplist(L);
    unsigned int start = luaL_checkinteger(L, 2);
    unsigned int end = luaL_checkinteger(L, 3);
    luaL_checktype(L, 4, LUA_TFUNCTION);
    if (start > end) {
        unsigned int tmp = start;
        start = end;
        end = tmp;
    }

    lua_pushinteger(L, slDeleteByRank(sl, start, end, _delete_rank_cb, L));
    return 1;
}

static int
_get_count(lua_State *L) {
    skiplist *sl = _to_skiplist(L);
    lua_pushinteger(L, sl->length);
    return 1;
}

static int
_get_rank(lua_State *L) {
    skiplist *sl = _to_skiplist(L);
    luaL_checktype(L, 2, LUA_TSTRING);
    slobj obj;
    obj.ptr = (char *)lua_tolstring(L, 2, &obj.length);
    luaL_checktype(L, 3, LUA_TFUNCTION);
    lua_pushvalue(L, 3);
    unsigned long rank = slGetRank(sl, &obj, _L_comp_mm, L);
    lua_pop(L, 1);
    if(rank == 0) {
        return 0;
    }

    lua_pushinteger(L, rank);

    return 1;
}

static int
_get_rank_range(lua_State *L) {
    skiplist *sl = _to_skiplist(L);
    unsigned long r1 = luaL_checkinteger(L, 2);
    unsigned long r2 = luaL_checkinteger(L, 3);
    int reverse, rangelen;
    if(r1 <= r2) {
        reverse = 0;
        rangelen = r2 - r1 + 1;
    } else {
        reverse = 1;
        rangelen = r1 - r2 + 1;
    }

    skiplistNode* node = slGetNodeByRank(sl, r1);
    lua_createtable(L, rangelen, 0);
    int n = 0;
    while(node && n < rangelen) {
        n++;

        lua_pushlstring(L, node->obj->ptr, node->obj->length);
        lua_rawseti(L, -2, n);
        node = reverse? node->backward : node->level[0].forward;
    }
    return 1;
}

static int
_get_score_range(lua_State *L) {
    skiplist *sl = _to_skiplist(L);
    int s1_index = 2;
    int s2_index = 3;
    int reverse = luaL_checkinteger(L, 4);
    luaL_checktype(L, 5, LUA_TFUNCTION);

    lua_pushvalue(L, 5);
    s1_index += 1;
    s2_index += 1;
    skiplistNode *node;
    if(reverse == 0) {
        node = slFirstInRange(sl, s1_index, s2_index, _L_comp_ms, L);
    } else {
        node = slLastInRange(sl, s2_index, s1_index, _L_comp_ms, L);
    }
    lua_pop(L, 1);
    s1_index -= 1;
    s2_index -= 1;


    lua_newtable(L);
    lua_pushvalue(L, 6);
    s1_index += 2;
    s2_index += 2;
    int n = 0;
    while(node) {
        if(reverse) {
            if(_L_comp_ms(L, node->obj, s2_index) == LT) break;
        } else {
            if(_L_comp_ms(L, node->obj, s2_index) == GT) break;
        }
        n++;

        lua_pushlstring(L, node->obj->ptr, node->obj->length);
        lua_rawseti(L, -3, n);

        node = reverse ? node->backward:node->level[0].forward;
    }
    lua_pop(L, 1);
    return 1;
}

static int
_get_member_by_rank(lua_State *L){
    skiplist *sl = _to_skiplist(L);
    unsigned long r = luaL_checkinteger(L, 2);
    skiplistNode *node = slGetNodeByRank(sl, r);
    if (node) {
        lua_pushlstring(L, node->obj->ptr, node->obj->length);
        return 1;
    }
    return 0;
}

static int
_dump(lua_State *L) {
    skiplist *sl = _to_skiplist(L);
    slDump(sl);
    return 0;
}

static int
_new(lua_State *L) {
    skiplist *psl = slCreate();

    skiplist **sl = (skiplist**) lua_newuserdata(L, sizeof(skiplist*));
    *sl = psl;
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_setmetatable(L, -2);
    return 1;
}

static int
_release(lua_State *L) {
    skiplist *sl = _to_skiplist(L);
    printf("collect sl:%p\n", sl);
    slFree(sl);
    return 0;
}

int luaopen_skiplist_c(lua_State *L) {
#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM > 501
    luaL_checkversion(L);
#endif

    luaL_Reg l[] = {
        {"insert", _insert},
        {"delete", _delete},
        {"delete_by_rank", _delete_by_rank},

        {"get_count", _get_count},
        {"get_rank", _get_rank},
        {"get_rank_range", _get_rank_range},
        {"get_score_range", _get_score_range},
        {"get_member_by_rank", _get_member_by_rank},

        {"dump", _dump},
        {NULL, NULL}
    };

    lua_createtable(L, 0, 2);

    luaL_newlib(L, l);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, _release);
    lua_setfield(L, -2, "__gc");

    lua_pushcclosure(L, _new, 1);
    return 1;
}

