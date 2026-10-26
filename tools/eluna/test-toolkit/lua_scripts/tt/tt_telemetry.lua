--[==[
TortoiseBots Eluna test toolkit: run telemetry (TEST SERVERS ONLY).

Prints `TTEV {json}` lines to mangosd's stdout (= `docker logs <container>`),
optionally also to TT.CFG.file_sink. No network.

Runs in the MAP Lua states (one per map/instance; core docs/ELUNA.md). Player
hooks fire in the state of the map the player is on (ElunaScriptBridge.cpp:
player->GetEluna()). In a state whose map is not a dungeon the hooks remove
themselves on their first call, so continents with 500 bots pay one call each.

NO creature events are registered on purpose: Eluna::GetAI (LuaEngine.cpp:954)
gives every creature whose entry has ANY Lua creature binding an
ElunaCreatureAI when it has no C++ script (ScriptMgr.cpp:1788-1791), which
would replace its EventAI and change the dungeon under test. Creature combat,
casts, channels and deaths come from a bounded 1 s poll around the players
instead (fast casts under 1 s can be missed: see README).

Events (ev):
  enter / leave        player entered/left this instance (MAP_EVENT 21/22)
  kill                 a player killed a creature (PLAYER_EVENT 7)
  pdeath               a creature killed a player (PLAYER_EVENT 8)
  pcombat / pleave     player enter/leave combat (PLAYER_EVENT 33/34)
  pcast                player spell cast (PLAYER_EVENT 5, at Spell::cast)
  c_combat             creature combat on/off (poll)
  c_cast               creature non-channel cast seen (poll)
  c_chan               creature channel start/stop with target (poll)
  c_death              creature dead, seen by the poll (no player killer hook)
  dropped              lines dropped by the per-state rate cap
Common fields: ts (unix s), ms (core ms clock), map, inst.
]==]

local TT = require("tt_lib")
local CFG = TT.CFG
if not CFG.telemetry then return end

local S = {
    dungeon = nil,   -- nil unknown, true/false once a hook saw the map
    mapId = nil, inst = nil,
    pollId = nil,
    cache = {}, cacheN = 0,
    cancels = {},
    seenCombat = {},
}

local function cancelAll()
    for _, c in ipairs(S.cancels) do pcall(c) end
    S.cancels = {}
end

-- Decide once per state; cancel later (not inside the running hook loop)
local function inDungeon(obj)
    if S.dungeon ~= nil then return S.dungeon end
    local map = obj:GetMap()
    if not map then return false end
    S.dungeon = map:IsDungeon() and true or false
    S.mapId, S.inst = map:GetMapId(), map:GetInstanceId()
    if not S.dungeon then
        CreateLuaEvent(function() cancelAll() end, 1000, 1)
    end
    return S.dungeon
end

local function base(extra)
    extra = extra or {}
    extra.map = S.mapId
    extra.inst = S.inst
    return extra
end

local function unitFields(t, prefix, u)
    if not u then return t end
    t[prefix .. "g"] = TT.objHex(u)
    t[prefix .. "e"] = u:GetEntry()
    t[prefix .. "n"] = u:GetName()
    return t
end

-- ---------------------------------------------------------------------------
-- creature poll
-- ---------------------------------------------------------------------------
local function stopPoll()
    if S.pollId then
        pcall(RemoveEventById, S.pollId)
        S.pollId = nil
    end
end

local function emitCreature(kind, c, extra)
    local t = base(extra)
    unitFields(t, "", c)
    t.p = TT.pos(c)
    t.hp = TT.round(c:GetHealthPct(), 1)
    TT.ev(kind, t)
end

local function pollTick()
    if not S.mapId then return end
    local map = GetMapById(S.mapId, S.inst)
    if not map then stopPoll(); return end
    local players = map:GetPlayers() or {}
    if #players == 0 then stopPoll(); return end

    -- anchors: in-combat players first, at most poll_max_anchors
    table.sort(players, function(a, b)
        local ac, bc = a:IsInCombat() and 1 or 0, b:IsInCombat() and 1 or 0
        return ac > bc
    end)
    local now = TT.nowMs()
    local seenNow, handled = {}, 0
    for i = 1, math.min(#players, CFG.poll_max_anchors) do
        local found = players[i]:GetCreaturesInRange(CFG.poll_radius, 0, 0, 0) or {}
        for j = 1, #found do
            if handled >= CFG.poll_max_units then break end
            local c = found[j]
            local hex = TT.objHex(c)
            if hex and not seenNow[hex] then
                seenNow[hex] = true
                handled = handled + 1
                local cur = {
                    cb = c:IsInCombat() and true or false,
                    dead = c:IsDead() and true or false,
                    seen = now,
                }
                local gid, gt = TT.spellInfo(c, TT.SPELL_GENERIC)
                local cid, ct = TT.spellInfo(c, TT.SPELL_CHANNELED)
                cur.cs, cur.cst, cur.ch, cur.cht = gid, gt, cid, ct
                local prev = S.cache[hex]
                if not prev then
                    S.cacheN = S.cacheN + 1
                    prev = { cb = false, dead = cur.dead, killed = false }
                end
                if cur.cb ~= prev.cb and not cur.dead then
                    local v = c:GetVictim()
                    emitCreature("c_combat", c, { on = cur.cb, v = v and TT.objHex(v) or TT.NULL })
                end
                if cur.dead and not prev.dead and not prev.killed then
                    emitCreature("c_death", c, { src = "poll" })
                end
                if gid and gid ~= prev.cs then
                    emitCreature("c_cast", c, { s = gid, t = gt or TT.NULL })
                end
                if cid ~= prev.ch or (cid and ct ~= prev.cht) then
                    if prev.ch then
                        emitCreature("c_chan", c, { on = false, s = prev.ch, t = prev.cht or TT.NULL })
                    end
                    if cid then
                        emitCreature("c_chan", c, { on = true, s = cid, t = ct or TT.NULL })
                    end
                end
                cur.killed = prev.killed and cur.dead
                S.cache[hex] = cur
            end
        end
    end
    -- evict entries not seen for 60 s; hard cap
    if S.cacheN > CFG.poll_cache_max or (now % 10000) < CFG.poll_ms then
        local n = 0
        for hex, e in pairs(S.cache) do
            if now - e.seen > 60000 then S.cache[hex] = nil else n = n + 1 end
        end
        S.cacheN = n
        if n > CFG.poll_cache_max then S.cache = {}; S.cacheN = 0 end
    end
end

local function startPoll()
    if S.pollId then return end
    S.pollId = CreateLuaEvent(function()
        TT.guard("telemetry.poll", pollTick)
    end, CFG.poll_ms, 0)
end

-- ---------------------------------------------------------------------------
-- hooks
-- ---------------------------------------------------------------------------
local function onMapEnter(event, map, player)
    TT.guard("telemetry.enter", function()
        if not inDungeon(player) then return end
        local t = base()
        unitFields(t, "", player)
        t.gm = (player:GetGMRank() >= CFG.min_gm_rank) and true or false
        t.p = TT.pos(player)
        TT.ev("enter", t)
        startPoll()
    end)
end

local function onMapLeave(event, map, player)
    TT.guard("telemetry.leave", function()
        if S.dungeon ~= true then return end
        local t = base()
        unitFields(t, "", player)
        TT.ev("leave", t)
    end)
end

local function onKill(event, killer, killed)
    TT.guard("telemetry.kill", function()
        if not inDungeon(killer) then return end
        local t = base()
        unitFields(t, "", killed)
        unitFields(t, "k", killer)
        t.p = TT.pos(killed)
        TT.ev("kill", t)
        local hex = t.g
        if hex then
            local e = S.cache[hex] or { seen = TT.nowMs() }
            e.dead, e.killed = true, true
            if not S.cache[hex] then S.cacheN = S.cacheN + 1 end
            S.cache[hex] = e
        end
    end)
end

local function onKilledBy(event, killer, killed)
    TT.guard("telemetry.pdeath", function()
        if not inDungeon(killed) then return end
        local t = base()
        unitFields(t, "", killed)
        unitFields(t, "k", killer)
        t.p = TT.pos(killed)
        TT.ev("pdeath", t)
    end)
end

local function onEnterCombat(event, player, enemy)
    TT.guard("telemetry.pcombat", function()
        if not inDungeon(player) then return end
        local key = tostring(player:GetGUIDLow()) .. ":" .. tostring(enemy and TT.objHex(enemy))
        local now = TT.nowMs()
        local last = S.seenCombat[key]
        if last and now - last < 10000 then return end
        S.seenCombat[key] = now
        local t = base()
        unitFields(t, "", player)
        unitFields(t, "x", enemy)
        TT.ev("pcombat", t)
    end)
end

local function onLeaveCombat(event, player)
    TT.guard("telemetry.pleave", function()
        if not inDungeon(player) then return end
        local t = base()
        unitFields(t, "", player)
        t.hp = TT.round(player:GetHealthPct(), 1)
        TT.ev("pleave", t)
        -- keep the dedupe table small
        local n = 0
        for _ in pairs(S.seenCombat) do n = n + 1 end
        if n > 500 then S.seenCombat = {} end
    end)
end

local function onSpellCast(event, player, spell, skipCheck)
    TT.guard("telemetry.pcast", function()
        if not inDungeon(player) then return end
        local t = base()
        unitFields(t, "", player)
        t.s = spell:GetEntry()
        local tgt = spell:GetTarget()
        if tgt and tgt.GetGUID then t.t = TT.objHex(tgt) end
        TT.ev("pcast", t)
    end)
end

local function reg(list, c) if c then list[#list + 1] = c end end
reg(S.cancels, RegisterServerEvent(TT.EV.MAP_ON_PLAYER_ENTER, onMapEnter))
reg(S.cancels, RegisterServerEvent(TT.EV.MAP_ON_PLAYER_LEAVE, onMapLeave))
reg(S.cancels, RegisterPlayerEvent(TT.EV.PLAYER_ON_KILL_CREATURE, onKill))
reg(S.cancels, RegisterPlayerEvent(TT.EV.PLAYER_ON_KILLED_BY_CREATURE, onKilledBy))
reg(S.cancels, RegisterPlayerEvent(TT.EV.PLAYER_ON_ENTER_COMBAT, onEnterCombat))
reg(S.cancels, RegisterPlayerEvent(TT.EV.PLAYER_ON_LEAVE_COMBAT, onLeaveCombat))
if CFG.log_player_casts then
    reg(S.cancels, RegisterPlayerEvent(TT.EV.PLAYER_ON_SPELL_CAST, onSpellCast))
end
