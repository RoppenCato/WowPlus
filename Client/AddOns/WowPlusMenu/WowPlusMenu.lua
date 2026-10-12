-- WowPlus Menu: one button with a menu of the commands you can use.
--
-- Everything in the menu only does what you could do by typing: a word in party chat that the bots in
-- your group understand ("follow"), one of this server's ".bots" commands, a slash command of another
-- WowPlus addon, or an ordinary game action (a ready check, a mark, an invitation).
-- Hover over an entry to read what it does. To add or change entries, edit MENU below.
--
-- Drag the button with the right mouse button. /wp opens the menu at the mouse, /wp reset puts the
-- button back, /wp hide and /wp show hide and show it. See Settings/Docs/Bots.md, "The menu".

-- Kinds of entry:
--   party   = "word"        said in party or raid chat; the bots in your group act on it
--   command = ".bots ..."   a server command (sent the way a typed dot command is)
--   target  = ".bots ... %s"  the same, with the name of your target filled in
--   slash   = "NAME"        the slash command of another WowPlus addon (its SlashCmdList name), with `arg`
--   func    = function      anything else
local MENU = {
    { title = "Windows", items = {
        { "Group board",            slash = "WOWPLUSGROUP",   tip = "People looking for a group: pick who comes along, or post your own group." },
        { "Journal",                slash = "WOWPLUSJOURNAL", tip = "What you killed, found and reached, and who you met." },
        { "Loot book",              slash = "WOWPLUSLOOTBOOK", tip = "What every dungeon and raid boss drops, with the drops that are new on this server." },
        { "Bot bar: show or hide",  slash = "WOWPLUSBOTS",    tip = "The small bar with Follow, Stay, Attack and the rest." },
        { "Bot bar: put it back",   slash = "WOWPLUSBOTS", arg = "reset", tip = "Moves the bot bar back to where it started." },
        { "Sprint bar: put it back", slash = "WOWPLUSSPRINT", arg = "reset", tip = "Moves the stamina bar back to where it started." },
    } },
    { title = "My group", items = {
        { "Follow me",              party = "follow",      tip = "The bots in your group follow you." },
        { "Stay here",              party = "stay",        tip = "They stay where they are until told to follow." },
        { "Attack my target",       party = "attack",      tip = "They attack what you have targeted." },
        { "Tank: take my target",   party = "tank attack", tip = "The tank picks up what you have targeted." },
        { "Run to me",              party = "flee",        tip = "They stop fighting and run to you." },
        { "Come here at once",      party = "summon",      tip = "They are brought to where you stand. For when someone is stuck or lost." },
        { "Spread out",             party = "disperse",    tip = "They move apart from each other." },
        { "Sit down and drink",     party = "drink",       tip = "Those who use mana drink." },
        { "Buffs again",            party = "rebuff",      tip = "They cast their buffs again." },
        { "Get up",                 party = "revive",      tip = "A dead bot comes back to life at its body. They also get up by themselves about twenty seconds after a fight is over." },
        { "Release spirit",         party = "release",     tip = "A dead bot releases to the graveyard." },
        { "Ready check",            func = function() DoReadyCheck() end, group = true,
          tip = "A bot that is low on health or mana, dead or not here says no, and sits down to eat or drink." },
        { "Leave the group",        func = function() LeaveParty() end, group = true, tip = "You leave your party or raid." },
    } },
    { title = "Marks on my target", items = {
        { "Skull: kill this first", mark = 8, tip = "Damage dealers and the tank go for the skull first." },
        { "Moon: keep out of the fight", mark = 5, tip = "A mage, hunter or druid sheeps, traps or sleeps it; the others leave it alone." },
        { "Cross",                  mark = 7, tip = "A mark for your own use." },
        { "Square",                 mark = 6, tip = "A mark for your own use." },
        { "Star",                   mark = 1, tip = "A mark for your own use." },
        { "Take the mark off",      mark = 0, tip = "Removes the mark from your target." },
    } },
    { title = "The person I have targeted", items = {
        { "Invite to my group",     func = function(name) InviteUnit(name) end, needsTarget = true, tip = "A bot always accepts and comes to you." },
        { "Invite to my guild",     func = function(name) GuildInvite(name) end, needsTarget = true, tip = "A bot always accepts; one in a guild of bots leaves it first." },
        { "Add to friends",         func = function(name) AddFriend(name) end, needsTarget = true, tip = "A bot on your friends list keeps its level." },
    } },
    { title = "Build my guild", items = {
        { "Meet someone",           command = ".bots meet",        tip = "Someone walks up and introduces themselves. Talk to them if you like. Saying next, keep or pass does the same as the entries below." },
        { "Meet a tank",            command = ".bots meet tank",   tip = "The same, but a tank." },
        { "Meet a healer",          command = ".bots meet healer", tip = "The same, but a healer." },
        { "Meet a damage dealer",   command = ".bots meet damage", tip = "The same, but someone who deals damage." },
        { "Someone else, please",   command = ".bots meet",        tip = "The one in front of you waves and leaves, and someone else comes by." },
        { "Invite them to my guild", command = ".bots keep",       tip = "The person in front of you joins your guild." },
        { "Thanks, not today",      command = ".bots pass",        tip = "They wave and go their own way." },
    } },
    { title = "Guild mates", guild = true, items = {
        { "Call who came last time", command = ".bots call",    tip = "The guild mates you travelled with last join your group and come to you." },
        { "Send them home",          command = ".bots dismiss", tip = "The guild mates in your group leave it and go their own way." },
    } },
    { title = "Bots around me", items = {
        { "Send them away, once",   command = ".bots away",      tip = "Every free bot near you goes somewhere else." },
        { "Keep them away",         command = ".bots alone on",  tip = "Free bots stay away from where you are until you allow them back. Your group stays." },
        { "Let them come back",     command = ".bots alone off", tip = "Bots may be around you again." },
    } },
}

local function Note(text)
    DEFAULT_CHAT_FRAME:AddMessage("|cffffd100Menu:|r " .. text)
end

local function InGroup()
    return GetNumPartyMembers() > 0 or GetNumRaidMembers() > 0
end

local function Run(item)
    if item.party then
        if not InGroup() then return Note("you are not in a group. Invite someone first, or open the Group board.") end
        SendChatMessage(item.party, GetNumRaidMembers() > 0 and "RAID" or "PARTY")
    elseif item.command then
        SendChatMessage(item.command, "SAY")        -- the server takes a line that starts with a dot as a command
    elseif item.target then
        if not UnitIsPlayer("target") then return Note("target a player first.") end
        SendChatMessage(string.format(item.target, UnitName("target")), "SAY")
    elseif item.slash then
        local handler = SlashCmdList[item.slash]
        if not handler then return Note("that addon is not loaded. Restart the game once to pick up new addons.") end
        handler(item.arg or "")
    elseif item.mark then
        if not UnitExists("target") then return Note("target something first.") end
        SetRaidTarget("target", item.mark)
    elseif item.func then
        if item.group and not InGroup() then return Note("you are not in a group.") end
        if item.needsTarget then
            if not UnitIsPlayer("target") then return Note("target a player first.") end
            item.func(UnitName("target"))
        else
            item.func()
        end
    end
end

-- The guild mates who are online, each as an entry that invites them to your group. A guild mate in your
-- group is a companion: there is nothing else to set up.
local CLASS_COLOURS = RAID_CLASS_COLORS
local function GuildMatesOnline()
    local entries = {}
    if not IsInGuild() then
        tinsert(entries, { text = "You have no guild yet", notCheckable = true, disabled = true })
        return entries
    end

    local me = UnitName("player")
    for index = 1, GetNumGuildMembers(true) do
        local name, _, _, level, _, zone, _, _, online, _, classToken = GetGuildRosterInfo(index)
        if name and online and name ~= me then
            local colour = CLASS_COLOURS[classToken or ""] or { r = 1, g = 1, b = 1 }
            tinsert(entries, {
                text = string.format("|cff%02x%02x%02x%s|r  %d", colour.r * 255, colour.g * 255, colour.b * 255, name, level or 0),
                notCheckable = true,
                func = function() InviteUnit(name) end,
                tooltipTitle = name, tooltipText = "Invite to your group. " .. (zone and zone ~= "" and ("Now in " .. zone .. ".") or ""),
            })
            if #entries >= 24 then break end
        end
    end
    if #entries == 0 then
        tinsert(entries, { text = "No guild mate is online", notCheckable = true, disabled = true })
    else
        tinsert(entries, 1, { text = "Invite to my group:", isTitle = true, notCheckable = true })
    end
    return entries
end

-- The menu in the shape the game's dropdown wants
local dropdown = CreateFrame("Frame", "WowPlusMenuDropDown", UIParent, "UIDropDownMenuTemplate")
local function Build()
    local menu = { { text = "WowPlus", isTitle = true, notCheckable = true } }
    for _, section in ipairs(MENU) do
        local entries = {}
        for _, item in ipairs(section.items) do
            tinsert(entries, { text = item[1], notCheckable = true, func = function() Run(item) end,
                               tooltipTitle = item[1], tooltipText = item.tip })
        end
        if section.guild then
            for _, entry in ipairs(GuildMatesOnline()) do tinsert(entries, entry) end
        end
        tinsert(menu, { text = section.title, notCheckable = true, hasArrow = true, menuList = entries })
    end
    tinsert(menu, { text = "Close", notCheckable = true, func = function() CloseDropDownMenus() end })
    return menu
end

local function Open(anchor)
    if IsInGuild() then GuildRoster() end       -- asks the server for a fresh guild list; used the next time the menu opens
    EasyMenu(Build(), dropdown, anchor or "cursor", 0, 0, "MENU")
end

-- The button
local button = CreateFrame("Button", "WowPlusMenuButton", UIParent, "UIPanelButtonTemplate")
button:SetWidth(54)
button:SetHeight(22)
button:SetText("Menu")
button:SetMovable(true)
button:SetClampedToScreen(true)
button:RegisterForClicks("LeftButtonUp")
button:RegisterForDrag("RightButton")
button:SetScript("OnClick", function(self) Open(self) end)
button:SetScript("OnDragStart", button.StartMoving)
button:SetScript("OnEnter", function(self)
    GameTooltip:SetOwner(self, "ANCHOR_TOP")
    GameTooltip:SetText("What you can ask of the bots and of this server.\nDrag with the right mouse button.", 1, 1, 1, 1, true)
    GameTooltip:Show()
end)
button:SetScript("OnLeave", function() GameTooltip:Hide() end)

local function Place()
    button:ClearAllPoints()
    button:SetPoint("CENTER", UIParent, "CENTER", WowPlusMenuDB.x or 300, WowPlusMenuDB.y or -230)
    if WowPlusMenuDB.hidden then button:Hide() else button:Show() end
end

button:SetScript("OnDragStop", function(self)
    self:StopMovingOrSizing()
    local x, y = self:GetCenter()
    local px, py = UIParent:GetCenter()
    WowPlusMenuDB.x, WowPlusMenuDB.y = x - px, y - py
    Place()
end)

local loader = CreateFrame("Frame")
loader:RegisterEvent("PLAYER_LOGIN")
loader:SetScript("OnEvent", function()
    WowPlusMenuDB = WowPlusMenuDB or {}
    Place()
end)

SLASH_WOWPLUSMENU1 = "/wp"
SLASH_WOWPLUSMENU2 = "/wowplus"
SlashCmdList["WOWPLUSMENU"] = function(text)
    if text == "reset" then
        WowPlusMenuDB.x, WowPlusMenuDB.y, WowPlusMenuDB.hidden = nil, nil, nil
        Place()
    elseif text == "hide" or text == "show" then
        WowPlusMenuDB.hidden = text == "hide"
        Place()
    else
        Open()
    end
end
