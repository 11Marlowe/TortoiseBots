--[==[
TortoiseBots Eluna test toolkit: encounter watcher PROTOTYPES (TEST SERVERS ONLY).

A pattern for later encounter probes. Each watcher is bound to one map, looks
its units up BY GUID (Map:GetWorldObject(GetUnitGUID(dbGuid, entry)); for a
static spawn the guid counter is the creature DB guid), so it never scans the
grid, and prints `TTEV {"ev":"proto.<name>",...}` only when something changes
(plus a heartbeat while the boss is in combat). Timers >= 1000 ms.

No creature events are registered (they would swap the creature's AI, see
tt_telemetry.lua).

Watchers:
  hailar_ritual  Frostmane Hollow (map 822): Hailar the Frigid 63130 (DB guid
                 2596345) and his five Ritualists 36519 (2596499..2596503),
                 which channel Frostmane Ritual 41592 at Hailar
                 (core src/scripts/dungeons/frostmane_hollow/*; r19
                 KILLFIRST-HAILAR-2026-10-22.md). Logs each Ritualist's channel
                 start/stop with its target and Hailar's hp, and who holds the
                 41592 aura on Hailar (Unit:GetAura + Aura:GetCasterGUID).
  kresh          Wailing Caverns (map 43): Kresh 3653 (DB guid 27368), a
                 patroller along the river (WC-ANACONDRA-KRESH-2026-10-22.md).
                 Logs his position every 5 s when he moved > 3 yd, and combat
                 changes.
Add a watcher: copy one table into WATCHERS with its own tick function.
]==]

local TT = require("tt_lib")
local CFG = TT.CFG
if not CFG.prototypes or not CFG.telemetry then return end

local function unitByDb(map, db, entry)
    local o = map:GetWorldObject(GetUnitGUID(db, entry))
    return o and o:ToUnit() or nil
end

-- ---------------------------------------------------------------------------
-- watchers
-- ---------------------------------------------------------------------------
local WATCHERS = {}

WATCHERS[#WATCHERS + 1] = {
    name = "hailar_ritual",
    map = 822,
    every = 1000,
    boss = { db = 2596345, e = 63130 },
    adds = { 2596499, 2596500, 2596501, 2596502, 2596503 },
    addEntry = 36519,
    spell = 41592,
    st = { chan = {}, bossCb = false, beat = 0 },
    tick = function(w, map, now)
        local boss = unitByDb(map, w.boss.db, w.boss.e)
        local bossHex = boss and TT.objHex(boss) or nil
        local changed = {}
        local channelling = 0
        for _, db in ipairs(w.adds) do
            local u = unitByDb(map, db, w.addEntry)
            local key = tostring(db)
            local now_s
            if u and u:IsAlive() then
                local cid, ct = TT.spellInfo(u, TT.SPELL_CHANNELED)
                if cid == w.spell then
                    now_s = (ct == bossHex) and "on_boss" or ("on:" .. tostring(ct))
                    channelling = channelling + 1
                else
                    now_s = cid and ("other:" .. cid) or "idle"
                end
            else
                now_s = u and "dead" or "absent"
            end
            if w.st.chan[key] ~= now_s then
                changed[#changed + 1] = { db = db, g = u and TT.objHex(u) or TT.NULL, was = w.st.chan[key] or TT.NULL,
                    now = now_s, d = (u and boss) and TT.round(TT.dist3(u:GetX(), u:GetY(), u:GetZ(),
                        boss:GetX(), boss:GetY(), boss:GetZ()), 1) or TT.NULL }
                w.st.chan[key] = now_s
            end
        end
        local bossCb = boss and boss:IsInCombat() or false
        local beat = bossCb and (now - w.st.beat >= 30000)
        if #changed > 0 or bossCb ~= w.st.bossCb or beat then
            w.st.bossCb = bossCb
            if beat or #changed > 0 then w.st.beat = now end
            local holders = {}
            if boss then
                local a = boss:GetAura(w.spell)
                if a then holders[#holders + 1] = TT.guidHex(a:GetCasterGUID()) or "?" end
            end
            TT.ev("proto.hailar_ritual", {
                map = w.map, inst = map:GetInstanceId(),
                boss = bossHex or TT.NULL,
                boss_hp = boss and TT.round(boss:GetHealthPct(), 1) or TT.NULL,
                boss_cb = bossCb and true or false,
                boss_p = boss and TT.pos(boss) or TT.NULL,
                channelling = channelling,
                aura_caster = holders, -- Unit:GetAura returns ONE holder of the id
                changed = changed,
            })
        end
    end,
}

WATCHERS[#WATCHERS + 1] = {
    name = "kresh",
    map = 43,
    every = 5000,
    unit = { db = 27368, e = 3653 },
    moved = 3,
    st = { x = nil, y = nil, cb = false, alive = nil },
    tick = function(w, map, now)
        local u = unitByDb(map, w.unit.db, w.unit.e)
        if not u then
            if w.st.alive ~= "absent" then
                w.st.alive = "absent"
                TT.ev("proto.kresh", { map = w.map, inst = map:GetInstanceId(), state = "absent" })
            end
            return
        end
        local alive = u:IsAlive() and "alive" or "dead"
        local x, y = u:GetX(), u:GetY()
        local moved = w.st.x and TT.dist2(x, y, w.st.x, w.st.y) or 999
        local cb = u:IsInCombat() and true or false
        if moved > w.moved or cb ~= w.st.cb or alive ~= w.st.alive then
            w.st.x, w.st.y, w.st.cb, w.st.alive = x, y, cb, alive
            local hx, hy = u:ToCreature():GetHomePosition()
            local v = u:GetVictim()
            TT.ev("proto.kresh", {
                map = w.map, inst = map:GetInstanceId(), g = TT.objHex(u), state = alive,
                p = TT.pos(u), cb = cb, hp = TT.round(u:GetHealthPct(), 1),
                v = v and TT.objHex(v) or TT.NULL,
                from_spawn = hx and TT.round(TT.dist2(x, y, hx, hy), 1) or TT.NULL,
            })
        end
    end,
}

-- ---------------------------------------------------------------------------
-- per-state start/stop: a watcher runs only in the state of its own map,
-- while players are in it.
-- ---------------------------------------------------------------------------
local running = {}

local function startWatcher(w, mapId, inst)
    if running[w.name] then return end
    local every = w.every
    if every < 1000 then every = 1000 end
    running[w.name] = CreateLuaEvent(function(eventId)
        TT.guard("proto." .. w.name, function()
            local map = GetMapById(mapId, inst)
            if not map or map:GetPlayerCount() == 0 then
                RemoveEventById(eventId)
                running[w.name] = nil
                return
            end
            w.tick(w, map, TT.nowMs())
        end)
    end, every, 0)
end

RegisterServerEvent(TT.EV.MAP_ON_PLAYER_ENTER, function(event, map, player)
    TT.guard("proto.enter", function()
        local id = map:GetMapId()
        for _, w in ipairs(WATCHERS) do
            if w.map == id then startWatcher(w, id, map:GetInstanceId()) end
        end
    end)
end)
