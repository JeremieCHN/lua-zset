local zset = (require "zset").new()

local function comp(score_tbl1, score_tbl2)
    if score_tbl1.score > score_tbl2.score then
        return 1
    elseif score_tbl1.score < score_tbl2.score then
        return -1
    end

    if score_tbl1.ts < score_tbl2.ts then
        return 1
    elseif score_tbl1.ts > score_tbl2.ts then
        return -1
    end

    return 0
end

local function dump()
    local count = zset:count()
    for i = 1, count do
        local member = zset:member_by_rev_rank(i)
        local score_tbl = zset:score(member)
        print(string.format("rank:%s, member: %s, score:%s, ts:%s", i, member, score_tbl.score, score_tbl.ts))
    end
end

-- 随机数据测试
zset:set_comp(comp)
for i = 1, 100 do
    local score_tbl = {
        score = math.random(1, 50),
        ts = math.random(1, 1000)
    }
    local member = string.format("A%s", i)
    zset:add(score_tbl, member)
end
dump()
