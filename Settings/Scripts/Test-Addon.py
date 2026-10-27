"""Runs one of our addons outside the game, against a stand-in for the 3.3.5 game's API, to catch Lua
errors before Robin opens the game: a nil called, a typo in a name, a table indexed wrong.

    python Settings/Scripts/Test-Addon.py WowPlusGuide "/guide" "tab 3" "tab 4" "search sword"
    python Settings/Scripts/Test-Addon.py WowPlusDialogue "event GOSSIP_SHOW" "event QUEST_DETAIL" "click WowPlusDialogueNext"

Every game function is a stand-in that accepts anything and returns a stand-in, so this does not
show whether the window looks right or whether the game really has a function: only that the
addon's own code runs from start to end. Each step after the addon's name is one of:
    /command args      a slash command
    event NAME [arg]   an event sent to every frame that registered it
    click NAME         a named button's OnClick
    tab N              clicks WowPlusGuideFrameTab N
    search TEXT        types into WowPlusGuideSearch
    update             runs every frame's OnUpdate once (1.6 seconds passed)
    texts              prints the text of every shown text on shown frames (what the window says)
Needs the Python package lupa (pip install lupa).
"""
import importlib
import io
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))

STAND_IN = r'''
tinsert, tremove, sort = table.insert, table.remove, table.sort
local Stub
local function call(...) return Stub() end
local stubMeta = {
    __index = function(t, k)
        local v = rawget(t, "_scripts") and nil
        local f = function(self, ...) return Stub() end
        rawset(t, k, f)
        return f
    end,
    __call = function() return Stub() end,
    __concat = function(a, b) return tostring(a) .. tostring(b) end,
    __tostring = function() return "stub" end,
    __add = function() return 0 end, __sub = function() return 0 end, __mul = function() return 0 end, __div = function() return 1 end,
    __lt = function() return false end, __le = function() return true end, __unm = function() return 0 end, __len = function() return 0 end,
}
Stub = function() return setmetatable({}, stubMeta) end

-- Frames: remember scripts and events, so that steps can run them
frames = {}
local frameMeta = {}
local function NewRegion(kind, name)
    local r = setmetatable({ _kind = kind, _scripts = {}, _events = {}, _shown = true, _text = "", _points = 0 }, frameMeta)
    allRegions = allRegions or {}
    tinsert(allRegions, r)
    if name then _G[name] = r end
    return r
end
frameMeta.__index = function(t, k)
    if type(k) ~= "string" or k:sub(1, 1) == "_" or k:sub(1, 1):lower() == k:sub(1, 1) then return nil end   -- fields, not methods
    local own = {
        SetScript = function(self, what, fn) self._scripts[what] = fn end,
        GetScript = function(self, what) return self._scripts[what] end,
        HookScript = function(self, what, fn) local old = self._scripts[what]; self._scripts[what] = function(...) if old then old(...) end fn(...) end end,
        RegisterEvent = function(self, e) self._events[e] = true end,
        UnregisterEvent = function(self, e) self._events[e] = nil end,
        UnregisterAllEvents = function(self) self._events = {} end,
        Show = function(self) local was = self._shown; self._shown = true; if not was and self._scripts.OnShow then self._scripts.OnShow(self) end end,
        Hide = function(self) local was = self._shown; self._shown = false; if was and self._scripts.OnHide then self._scripts.OnHide(self) end end,
        IsShown = function(self) return self._shown end,
        IsVisible = function(self) return self._shown end,
        SetText = function(self, s) self._text = s end,
        GetText = function(self) return self._text end,
        SetID = function(self, id) self._id = id end,
        GetID = function(self) return self._id or 0 end,
        GetName = function(self) return self._name end,
        CreateTexture = function(self, name) return NewRegion("Texture", name) end,
        CreateFontString = function(self, name) return NewRegion("FontString", name) end,
        GetFontString = function(self) self._fs = self._fs or NewRegion("FontString"); return self._fs end,
        GetStringHeight = function() return 14 end, GetStringWidth = function() return 80 end,
        GetWidth = function() return 100 end, GetHeight = function() return 100 end,
        GetVerticalScrollRange = function() return 200 end, GetVerticalScroll = function() return 0 end,
        SetTexture = function() return 1 end,
        GetModel = function() return "model" end,
        Click = function(self) if self._scripts.OnClick then self._scripts.OnClick(self, "LeftButton") end end,
        GetFont = function() return "font", 12, "" end,
    }
    local f = own[k] or function(self, ...) return Stub() end
    rawset(t, k, f)
    return f
end
function CreateFrame(kind, name, parent, template)
    local f = NewRegion(kind, name)
    f._name = name
    f._shown = true
    if template and template:find("ScrollFrame") and name then
        _G[name .. "ScrollBar"] = NewRegion("Slider", name .. "ScrollBar")
    end
    tinsert(frames, f)
    return f
end
UIParent = CreateFrame("Frame", "UIParent")
GameTooltip = CreateFrame("GameTooltip", "GameTooltip")
DEFAULT_CHAT_FRAME = CreateFrame("Frame", "DEFAULT_CHAT_FRAME")
UIErrorsFrame = CreateFrame("Frame", "UIErrorsFrame")
GossipFrame = CreateFrame("Frame", "GossipFrame")
QuestFrame = CreateFrame("Frame", "QuestFrame")
GameFontNormalSmall = CreateFrame("Font", "GameFontNormalSmall")
DEFAULT_CHAT_FRAME.AddMessage = function(self, text) print("chat: " .. tostring(text)) end
UISpecialFrames, SlashCmdList = {}, {}
RAID_CLASS_COLORS = setmetatable({}, { __index = function() return { r = 1, g = 1, b = 1 } end })
LOCALIZED_CLASS_NAMES_MALE = setmetatable({}, { __index = function(t, k) return k end })
tinsert, tremove, wipe, sort = table.insert, table.remove, function(t) for k in pairs(t) do t[k] = nil end return t end, table.sort
strlower, strupper, strlen, strfind, strsub, format, strrep = string.lower, string.upper, string.len, string.find, string.sub, string.format, string.rep
function strtrim(s) return (tostring(s):gsub("^%s+", ""):gsub("%s+$", "")) end
function strsplit(sep, s) local out = {} for p in (s .. sep):gmatch("(.-)" .. sep:gsub("%p", "%%%0")) do out[#out + 1] = p end return unpack(out) end
GetTime = function() return 0 end
UnitLevel = function() return 15 end
UnitName = function() return "Someone" end
GetRealZoneText = function() return "Westfall" end
GetSubZoneText = function() return "Sentinel Hill" end
GetItemInfo = function() return nil end
GetItemIcon = function() return nil end
GetQuestDifficultyColor = function() return { r = 1, g = 1, b = 0 } end
InCombatLockdown = function() return false end
IsModifiedClick = function() return false end
GetCursorPosition = function() return 0, 0 end
GetNumGossipActiveQuests = function() return 1 end
GetGossipActiveQuests = function() return "An old quest", 10, false, true end
GetNumGossipAvailableQuests = function() return 2 end
GetGossipAvailableQuests = function() return "Quest one", 12, false, false, false, "Quest two", 14, true, false, false end
GetGossipOptions = function() return "Let me browse your goods.", "vendor", "I want to travel.", "taxi" end
GetGossipText = function() return "Well met, traveller. The roads are not safe these days.\n\nBe careful out there!" end
GetNumQuestChoices = function() return 2 end
GetNumQuestRewards = function() return 1 end
GetNumQuestItems = function() return 1 end
GetQuestItemInfo = function() return "Thing", "icon", 2, 3, true end
GetRewardMoney = function() return 1234 end
GetQuestMoneyToGet = function() return 0 end
GetRewardSpell = function() return nil end
GetQuestText = function() return string.rep("This is a long story about the land. ", 20) end
GetObjectiveText = function() return "Kill 8 things." end
GetTitleText = function() return "A Test Quest" end
GetProgressText = function() return "Have you done it?" end
GetRewardText = function() return "Thank you!" end
GetGreetingText = function() return "Hello." end
IsQuestCompletable = function() return true end
GetCoinTextureString = function(n) return tostring(n) .. "c" end
UnitOnTaxi = function() return false end
-- Spoken's player (SpokenPlayer), as far as its public interface goes: it prints what it is given to play
Spoken = { sources = {} }
function Spoken:IsCompatible(n) return n <= 1 end
function Spoken:GetSource(key) return self.sources[key] end
function Spoken:RegisterSource(key, info)
    local source = { key = key }
    function source:Enqueue(clip) print("spoken plays: " .. clip.path .. " (" .. clip.length .. " s) " .. clip.present.header .. " / " .. clip.present.label) end
    self.sources[key] = source
    return source
end
setmetatable(_G, { __index = function(t, k) if type(k) == "string" and (k:match("DB$") or k:match("^WowPlus")) then return nil end return function(...) return Stub() end end })   -- saved variables and our own globals start empty
'''

RUNNER = r'''
function fire(event, arg)
    for _, f in ipairs(frames) do
        if f._events[event] and f._scripts.OnEvent then f._scripts.OnEvent(f, event, arg) end
    end
end
function texts()
    local seen = 0
    for _, f in ipairs(allRegions) do
        if f._kind == "FontString" and f._shown and type(f._text) == "string" and f._text ~= "" and seen < 60 then
            seen = seen + 1
            print("  | " .. (f._text:sub(1, 110):gsub("%s+", " ")))
        end
    end
end
function updateAll()
    for _, f in ipairs(frames) do
        if f._scripts.OnUpdate then f._scripts.OnUpdate(f, 1.6) end
    end
end
'''


def main():
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    try:
        lupa = importlib.import_module("lupa.lua51")
    except ImportError:
        print("Needs the Python package lupa: pip install lupa")
        return 2
    addon = sys.argv[1]
    folder = os.path.join(ROOT, "Client", "AddOns", addon)
    toc = io.open(os.path.join(folder, addon + ".toc"), encoding="utf-8").read()
    files = [line.strip() for line in toc.splitlines() if line.strip() and not line.startswith("#")]
    lua = lupa.LuaRuntime(unpack_returned_tuples=True)
    lua.execute(STAND_IN)
    lua.execute(RUNNER)
    for name in files:
        code = io.open(os.path.join(folder, name), encoding="utf-8").read()
        lua.execute(code)
        print("loaded", name)
    g = lua.globals()
    g.fire("ADDON_LOADED", addon)
    g.fire("PLAYER_LOGIN")
    g.fire("ZONE_CHANGED_NEW_AREA")
    for step in sys.argv[2:]:
        print(">", step)
        word, _, rest = step.partition(" ")
        if word.startswith("/"):
            for key, fn in g.SlashCmdList.items():
                for i in range(1, 5):
                    if g["SLASH_%s%d" % (key, i)] == word:
                        fn(rest)
        elif word == "event":
            event, _, arg = rest.partition(" ")
            g.fire(event, arg or None)
        elif word == "click":
            g[rest].Click(g[rest])
        elif word == "tab":
            tab = g["WowPlusGuideFrameTab" + rest]
            tab._scripts.OnClick(tab)
        elif word == "search":
            box = g.WowPlusGuideSearch
            box._text = rest
            box._scripts.OnTextChanged(box)
        elif word == "update":
            g.updateAll()
        elif word == "texts":
            g.texts()
    print("No Lua errors.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
