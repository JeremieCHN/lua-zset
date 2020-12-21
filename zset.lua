local skiplist = require "skiplist.c"
local mt = {}
mt.__index = mt

------------------------------------------------------------------------
local RES = { GT = 1, EQ = 0, LT = -1 }
local PARAMS = { MM = 1, MS = 2, SS = 3 }

function mt.__default_comp(s1, s2)
    if not (type(s1) == "number" and type(s2) == "number") then
        s1 = tostring(s1)
        s2 = tostring(s2)
    end
    if s1 > s2 then
        return RES.GT
    elseif s1 == s2 then
        return RES.GT
    else
        return RES.LT
    end
end

function mt:_get_comp(param)
    if param == PARAMS.MM then
        return function (m1, m2)
            assert(self.tbl[m1] and self.tbl[m2], "compare member but member dont exist")
            local s1 = self.tbl[m1]
            local s2 = self.tbl[m2]
            return self:_get_comp(PARAMS.SS)(s1, s2)
        end
    elseif param == PARAMS.MS then
        return function (m1, s2)
            assert(self.tbl[m1], "compare member and score but member dont exist")
            local s1 = self.tbl[m1]
            return self:_get_comp(PARAMS.SS)(s1, s2)
        end
    elseif param == PARAMS.SS then
        return function (s1, s2)
            local comp = self.comp or self.__default_comp
            return comp(s1, s2)
        end
    end
end

function  mt:set_comp(comp)
    self.comp = comp
end
------------------------------------------------------------------------
function mt:add(score, member)
    local old = self.tbl[member]
    if old then
        local comp = self:_get_comp(PARAMS.SS)
        if comp(old, score) == RES.EQ then
            return
        end
        self.sl:delete(member, self:_get_comp(PARAMS.MM))
    end

    self.tbl[member] = score
    local comp = self:_get_comp(PARAMS.MM)
    self.sl:insert(member, comp)
end

function mt:rem(member)
    local score = self.tbl[member]
    if score then
        self.sl:delete(member, self:_get_comp(PARAMS.MM))
        self.tbl[member] = nil
    end
end

function mt:count()
    return self.sl:get_count()
end

function mt:_reverse_rank(r)
    return self.sl:get_count() - r + 1
end

function mt:limit(count, delete_handler)
    local total = self.sl:get_count()
    if total <= count then
        return 0
    end

    local delete_function = function(member)
        self.tbl[member] = nil
        if delete_handler then
            delete_handler(member)
        end
    end

    return self.sl:delete_by_rank(count+1, total, delete_function)
end

function mt:rev_limit(count, delete_handler)
    local total = self.sl:get_count()
    if total <= count then
        return 0
    end
    local from = self:_reverse_rank(count+1)
    local to   = self:_reverse_rank(total)

    local delete_function = function(member)
        self.tbl[member] = nil
        if delete_handler then
            delete_handler(member)
        end
    end

    return self.sl:delete_by_rank(from, to, delete_function)
end

function mt:rev_range(r1, r2)
    r1 = self:_reverse_rank(r1)
    r2 = self:_reverse_rank(r2)
    return self:range(r1, r2)
end

function mt:range(r1, r2)
    if r1 < 1 then
        r1 = 1
    end

    if r2 < 1 then
        r2 = 1
    end
    return self.sl:get_rank_range(r1, r2)
end

function mt:rev_rank(member)
    local r = self:rank(member)
    if r then
        return self:_reverse_rank(r)
    end
    return r
end

function mt:rank(member)
    local score = self.tbl[member]
    if not score then
        return nil
    end
    return self.sl:get_rank(member, self:_get_comp(PARAMS.MM))
end

function mt:range_by_score(s1, s2)
    local comp_ss = self:_get_comp(PARAMS.SS)
    local reverse
    if comp_ss(s1, s2) == RES.GT then
        reverse = 1
    else
        reverse = 0
    end

    return self.sl:get_score_range(s1, s2, reverse, self:_get_comp(PARAMS.MS))
end

function mt:score(member)
    return self.tbl[member]
end

function mt:member_by_rank(r)
    return self.sl:get_member_by_rank(r)
end

function mt:member_by_rev_rank(r)
    r = self:_reverse_rank(r)
    if r > 0 then
        return self.sl:get_member_by_rank(r)
    end
end

function mt:dump()
    self.sl:dump()
end

local M = {}
function M.new()
    local obj = {}
    obj.sl = skiplist()
    obj.tbl = {}
    obj.comp = false
    return setmetatable(obj, mt)
end
return M

