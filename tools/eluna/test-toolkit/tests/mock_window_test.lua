-- Mock test (plain Lua, no server): quiet/clearto dz window, skip filter and the
-- "friendly NPCs are never cleared" rule. Run from the toolkit dir:  lua tests/mock_window_test.lua
package.path = "lua_scripts/tt/?.lua;lua_scripts/tt/?.ext;" .. package.path
local handler
local replies = {}
_G.RegisterPlayerEvent = function(_, fn) handler = fn end
_G.CreateLuaEvent = function(fn) fn() end   -- the second quiet read runs at once
_G.RegisterServerEvent = function() end
_G.GetPlayerByName = function(n) return _G.__players[n] end
io.write = function(s) replies[#replies + 1] = s end -- writeStdout echoes here (and to the player)

local function creature(entry, counter, x, y, z, hostileToBots, state)
    local c = { entry = entry, x = x, y = y, z = z, hostile = hostileToBots, removed = false }
    c.GetX = function() return c.x end
    c.GetY = function() return c.y end
    c.GetZ = function() return c.z end
    c.GetEntry = function() return entry end
    c.GetName = function() return "n" .. entry end
    c.GetOwner = function() return nil end
    c.GetRank = function() return 0 end
    c.IsWorldBoss = function() return false end
    c.GetGUID = function() return string.format("Creature (Entry: %d Guid: %d)", entry, counter) end
    c.DespawnOrUnsummon = function() c.removed = true end
    c.GetHealth = function() return 1 end c.GetMaxHealth = function() return 1 end
    c.GetHealthPct = function() return 100 end c.IsInCombat = function() return false end
    c.IsDead = function() return false end c.GetVictim = function() return nil end
    c.ToCreature = function() return nil end
    c.GetCurrentSpell = function() return nil end
    return c
end

local world = {}
local function player(name, gm, x, y, z)
    local p = { sent = {}, gm = gm }
    p.GetGMRank = function() return gm and 3 or 0 end
    p.IsGM = function() return gm end
    p.GetName = function() return name end
    p.GetX = function() return x end p.GetY = function() return y end p.GetZ = function() return z end
    p.GetO = function() return 0 end
    p.GetMapId = function() return 33 end p.GetInstanceId = function() return 7 end
    p.IsAlive = function() return true end p.IsInWorld = function() return true end
    p.GetGUID = function() return "Player " .. name .. " (Guid: 12)" end
    p.SendBroadcastMessage = function(_, s) p.sent[#p.sent + 1] = s end
    p.GetMap = function() return { GetMapId = function() return 33 end, GetInstanceId = function() return 7 end,
        IsDungeon = function() return true end } end
    p.GetCreaturesInRange = function(_, reach, entry, hostile, dead)
        local out = {}
        for _, c in ipairs(world) do
            if not c.removed and (entry == 0 or entry == c.entry) then
                -- GM caller: hostile=1 gives nothing, 0 gives everything; a bot: hostile=1 = hostile ones
                if hostile == 0 or (hostile == 1 and not gm and c.hostile) then out[#out + 1] = c end
            end
        end
        return out
    end
    return p
end

local gm = player("Cmd", true, 0, 0, 100)
local bot = player("Bot", false, 0, 0, 100)
local group = { GetMembers = function() return { bot } end }
gm.GetGroup = function() return nil end
bot.GetGroup = function() return group end
_G.__players = { Bot = bot, Cmd = gm }

require("tt_lib")
dofile("lua_scripts/tt/tt_commands.lua")

local fails = 0
local function check(name, cond) if not cond then fails = fails + 1; print("FAIL " .. name) else print("ok   " .. name) end end
local function last() return gm.sent[#gm.sent] or "" end
if os.getenv("V") then local l=last; last=function() local s=l(); print(s); return s end end
local function reset()
    world = {
        creature(1000, 1, 5, 0, 100, true),    -- same floor, hostile
        creature(1001, 2, 5, 5, 130, true),    -- 30 above, hostile
        creature(3849, 3, 3, 3, 100, false),   -- friendly NPC (Adamant)
        creature(1002, 4, 6, 6, 95, true),     -- 5 below, hostile
        creature(1003, 5, 7, 7, 50, true),     -- 50 below, hostile
    }
    gm.sent = {}
end

-- quiet, default dz 8: far-floor units and the friendly NPC? (the NPC counts: quiet counts all)
reset(); handler(42, gm, "tt quiet 50 0 0 100 w=0")
check("quiet default dz 8 counts 1000, 3849, 1002 only", last():find('"n_near":3', 1, true))
reset(); handler(42, gm, "tt quiet 50 0 0 100 w=0 dz=0")
check("quiet dz=0 is the old 2D rule (5 units)", last():find('"n_near":5', 1, true))
reset(); handler(42, gm, "tt quiet 50 0 0 100 w=0 skip=3849,1000")
check("quiet skip drops entries", last():find('"n_near":1', 1, true))
reset(); handler(42, gm, "tt quiet 50 0 0 100 w=0 dz=2 skip=3849")
check("quiet dz=2 skip=3849 leaves 1000 only", last():find('"n_near":1', 1, true))

-- clearto
reset(); handler(42, gm, "tt clearto 33 0 0 100 50 of=Bot")
check("clearto default: friendly NPC kept", not world[3].removed and last():find('"why":"friendly"', 1, true))
check("clearto default: same-floor and 5 below hostile removed", world[1].removed and world[4].removed)
check("clearto default: 30 above and 50 below kept (dz 8)", not world[2].removed and not world[5].removed)
reset(); handler(42, gm, "tt clearto 33 0 0 100 50 dz=0 of=Bot")
check("clearto dz=0 removes every hostile floor", world[2].removed and world[5].removed and not world[3].removed)
reset(); handler(42, gm, "tt clearto 33 0 0 100 50 skip=1000 of=Bot")
check("clearto skip keeps 1000", not world[1].removed and world[4].removed)
reset(); handler(42, gm, "tt clearto 33 0 0 100 50 all=1")
check("clearto all=1 removes friendly too", world[3].removed and world[1].removed)
reset(); handler(42, gm, "tt clearto 33 0 0 100 50")
check("clearto without a reference refuses", last():find('"err":"no_reference"', 1, true) and not world[1].removed)

if fails > 0 then os.exit(1) end
print("all mock tests passed")
