-- WowPlus Loot Book: what every dungeon and raid boss drops on this server.
--
-- The lists are in Data.lua, which the server writes at every start from the same lists it drops from
-- (Settings/Scripts/Make-Boss-Loot.py), so the book and the game agree. For each boss it shows the
-- game's own loot with its chance, and under "New on this server" the drops that boss does not have in
-- the real game, each with the class it was picked for.
--
-- Left: dungeons or raids. Middle: the bosses of the place you clicked. Right: the loot.
-- Type in the search box to find an item anywhere. Shift-click an item to link it in chat, Ctrl-click
-- to try it on. /lootbook opens and closes the window. See Settings/Docs/Boss-Loot.md.

local DATA, ITEMS = WowPlusLootBookData or {}, WowPlusLootBookItems or {}

local PLACE_ROWS, BOSS_ROWS, ITEM_ROWS = 22, 22, 20
local ROW = 18
local CLASS_TOKENS = { [1] = "WARRIOR", [2] = "PALADIN", [3] = "HUNTER", [4] = "ROGUE", [5] = "PRIEST", [6] = "DEATHKNIGHT",
                       [7] = "SHAMAN", [8] = "MAGE", [9] = "WARLOCK", [11] = "DRUID" }
local QUALITY = { [0] = "9d9d9d", [1] = "ffffff", [2] = "1eff00", [3] = "0070dd", [4] = "a335ee", [5] = "ff8000", [6] = "e6cc80" }

local view = { raids = false, place = nil, boss = nil, mode = 1, search = "" }
local places, bosses, lines = {}, {}, {}            -- what the three columns show right now
local offset = { places = 0, bosses = 0, lines = 0 }

local frame = CreateFrame("Frame", "WowPlusLootBookFrame", UIParent)
frame:SetWidth(760)
frame:SetHeight(PLACE_ROWS * ROW + 96)
frame:SetPoint("CENTER", 0, 20)
frame:SetBackdrop({ bgFile = "Interface\\DialogFrame\\UI-DialogBox-Background", edgeFile = "Interface\\DialogFrame\\UI-DialogBox-Border",
                    tile = true, tileSize = 32, edgeSize = 24, insets = { left = 6, right = 6, top = 6, bottom = 6 } })
frame:SetMovable(true)
frame:SetClampedToScreen(true)
frame:EnableMouse(true)
frame:RegisterForDrag("LeftButton")
frame:SetScript("OnDragStart", frame.StartMoving)
frame:SetScript("OnDragStop", frame.StopMovingOrSizing)
frame:SetFrameStrata("HIGH")
frame:Hide()
tinsert(UISpecialFrames, "WowPlusLootBookFrame")

local title = frame:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
title:SetPoint("TOP", 0, -14)
title:SetText("Loot Book")

local close = CreateFrame("Button", nil, frame, "UIPanelCloseButton")
close:SetPoint("TOPRIGHT", -4, -4)

local heading = frame:CreateFontString(nil, "OVERLAY", "GameFontNormal")
heading:SetPoint("TOPLEFT", 396, -44)
heading:SetWidth(340)
heading:SetJustifyH("LEFT")

local Refresh                                       -- defined below; the buttons need it

-- ---------------------------------------------------------------------------------------------
-- What the columns hold
-- ---------------------------------------------------------------------------------------------

local function ItemName(id)
    local known = ITEMS[id]
    local name, _, quality = GetItemInfo(id)
    return name or (known and known[1]) or ("Item " .. id), quality or (known and known[2]) or 1
end

local function ClassName(class)
    local token = CLASS_TOKENS[class]
    local colour = token and RAID_CLASS_COLORS[token]
    local name = token and LOCALIZED_CLASS_NAMES_MALE[token] or "anyone"
    if not colour then return name end
    return string.format("|cff%02x%02x%02x%s|r", colour.r * 255, colour.g * 255, colour.b * 255, name)
end

local function BuildPlaces()
    wipe(places)
    for _, place in ipairs(DATA) do
        if place.raid == view.raids then tinsert(places, place) end
    end
end

local function BuildBosses()
    wipe(bosses)
    if view.place then
        for _, boss in ipairs(view.place.bosses) do tinsert(bosses, boss) end
    end
end

-- The right-hand column: either one boss's loot, or what the search found
local function BuildLines()
    wipe(lines)
    if view.search ~= "" then
        local wanted = strlower(view.search)
        for _, place in ipairs(DATA) do
            for _, boss in ipairs(place.bosses) do
                for _, mode in ipairs(boss.modes) do
                    local from = boss.name .. ", " .. place.name .. (mode.label ~= "" and (" (" .. mode.label .. ")") or "")
                    for _, drop in ipairs(mode.o) do
                        local known = ITEMS[drop[1]]
                        if known and strfind(strlower(known[1]), wanted, 1, true) then
                            tinsert(lines, { item = drop[1], right = drop[2] .. "%", note = from })
                        end
                    end
                    for _, drop in ipairs(mode.n) do
                        local known = ITEMS[drop[1]]
                        if known and strfind(strlower(known[1]), wanted, 1, true) then
                            tinsert(lines, { item = drop[1], right = "new", note = from })
                        end
                    end
                    if #lines >= 300 then return end
                end
            end
        end
        return
    end

    local boss = view.boss
    local mode = boss and (boss.modes[view.mode] or boss.modes[1])
    if not mode then return end
    for _, drop in ipairs(mode.o) do
        tinsert(lines, { item = drop[1], right = drop[2] .. "%" })
    end
    if #mode.n > 0 then
        tinsert(lines, { header = "New on this server" })
        for _, drop in ipairs(mode.n) do
            tinsert(lines, { item = drop[1], right = ClassName(drop[2]) })
        end
    end
end

-- ---------------------------------------------------------------------------------------------
-- The three columns
-- ---------------------------------------------------------------------------------------------

local function MakeColumn(x, width, count, key, onClick)
    local rows = {}
    local holder = CreateFrame("Frame", nil, frame)
    holder:SetPoint("TOPLEFT", x, -66)
    holder:SetWidth(width)
    holder:SetHeight(count * ROW)
    holder:EnableMouseWheel(true)
    holder:SetScript("OnMouseWheel", function(self, delta)
        offset[key] = offset[key] - delta * 3
        Refresh()
    end)
    for index = 1, count do
        local row = CreateFrame("Button", nil, holder)
        row:SetHeight(ROW)
        row:SetWidth(width)
        row:SetPoint("TOPLEFT", 0, -(index - 1) * ROW)
        row:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight", "ADD")
        row.text = row:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
        row.text:SetPoint("LEFT", 4, 0)
        row.text:SetPoint("RIGHT", -4, 0)
        row.text:SetJustifyH("LEFT")
        row:SetScript("OnClick", function(self) if self.value then onClick(self.value) end end)
        rows[index] = row
    end
    return rows
end

local placeRows = MakeColumn(16, 176, PLACE_ROWS, "places", function(place)
    view.place, view.boss, view.mode, view.search = place, place.bosses[1], 1, ""
    WowPlusLootBookSearch:SetText("")
    offset.bosses, offset.lines = 0, 0
    Refresh()
end)
local bossRows = MakeColumn(198, 190, BOSS_ROWS, "bosses", function(boss)
    view.boss, view.mode, view.search = boss, 1, ""
    WowPlusLootBookSearch:SetText("")
    offset.lines = 0
    Refresh()
end)

-- Loot rows have an icon, a name, something on the right, and a tooltip
local itemRows = {}
local itemHolder = CreateFrame("Frame", nil, frame)
itemHolder:SetPoint("TOPLEFT", 396, -66)
itemHolder:SetWidth(348)
itemHolder:SetHeight(ITEM_ROWS * 20)
itemHolder:EnableMouseWheel(true)
itemHolder:SetScript("OnMouseWheel", function(self, delta)
    offset.lines = offset.lines - delta * 3
    Refresh()
end)
for index = 1, ITEM_ROWS do
    local row = CreateFrame("Button", nil, itemHolder)
    row:SetHeight(20)
    row:SetWidth(348)
    row:SetPoint("TOPLEFT", 0, -(index - 1) * 20)
    row:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight", "ADD")
    row.icon = row:CreateTexture(nil, "ARTWORK")
    row.icon:SetWidth(18)
    row.icon:SetHeight(18)
    row.icon:SetPoint("LEFT", 2, 0)
    row.name = row:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    row.name:SetPoint("LEFT", 24, 0)
    row.name:SetWidth(236)
    row.name:SetJustifyH("LEFT")
    row.right = row:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    row.right:SetPoint("RIGHT", -4, 0)
    row.right:SetWidth(84)
    row.right:SetJustifyH("RIGHT")
    row:SetScript("OnEnter", function(self)
        if not self.item then return end
        GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
        GameTooltip:SetHyperlink("item:" .. self.item)      -- also asks the server for an item the game has not seen yet
        if self.note then GameTooltip:AddLine(self.note, 1, 0.82, 0, true) end
        GameTooltip:Show()
    end)
    row:SetScript("OnLeave", function() GameTooltip:Hide() end)
    row:SetScript("OnClick", function(self)
        if not self.item then return end
        local _, link = GetItemInfo(self.item)
        if not link then
            DEFAULT_CHAT_FRAME:AddMessage("Loot book: hover over the item for a moment first, so that the game loads it.")
        elseif IsModifiedClick("CHATLINK") then
            ChatEdit_InsertLink(link)
        elseif IsModifiedClick("DRESSUP") then
            DressUpItemLink(link)
        end
    end)
    itemRows[index] = row
end

-- Dungeons / Raids, the ways of playing a boss, and the search box
local function MakeTab(text, x, onClick)
    local button = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
    button:SetWidth(84)
    button:SetHeight(20)
    button:SetPoint("TOPLEFT", x, -40)
    button:SetText(text)
    button:GetFontString():SetFont(GameFontNormalSmall:GetFont())
    button:SetScript("OnClick", onClick)
    return button
end
local function ShowKind(raids)
    view.raids, view.search = raids, ""
    WowPlusLootBookSearch:SetText("")
    BuildPlaces()
    view.place = places[1]
    view.boss = view.place and view.place.bosses[1]
    view.mode = 1
    offset.places, offset.bosses, offset.lines = 0, 0, 0
    Refresh()
end
local dungeonTab = MakeTab("Dungeons", 16, function() ShowKind(false) end)
local raidTab = MakeTab("Raids", 104, function() ShowKind(true) end)

local modeButtons = {}
for index = 1, 4 do
    local button = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
    button:SetWidth(84)
    button:SetHeight(20)
    button:SetPoint("BOTTOMLEFT", 396 + (index - 1) * 86, 14)
    button:GetFontString():SetFont(GameFontNormalSmall:GetFont())
    button:SetScript("OnClick", function()
        view.mode = index
        offset.lines = 0
        Refresh()
    end)
    modeButtons[index] = button
end

local searchLabel = frame:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
searchLabel:SetPoint("BOTTOMLEFT", 18, 20)
searchLabel:SetText("Find an item:")
local search = CreateFrame("EditBox", "WowPlusLootBookSearch", frame, "InputBoxTemplate")
search:SetWidth(200)
search:SetHeight(20)
search:SetPoint("BOTTOMLEFT", 100, 14)
search:SetAutoFocus(false)
search:SetScript("OnTextChanged", function(self)
    local text = self:GetText() or ""
    view.search = strlen(text) >= 3 and text or ""
    offset.lines = 0
    Refresh()
end)
search:SetScript("OnEscapePressed", function(self) self:ClearFocus() end)
search:SetScript("OnEnterPressed", function(self) self:ClearFocus() end)

-- ---------------------------------------------------------------------------------------------

local function Clamp(key, total, shown)
    offset[key] = math.max(0, math.min(offset[key], math.max(0, total - shown)))
end

function Refresh()
    BuildPlaces()
    BuildBosses()
    BuildLines()
    Clamp("places", #places, PLACE_ROWS)
    Clamp("bosses", #bosses, BOSS_ROWS)
    Clamp("lines", #lines, ITEM_ROWS)

    for index, row in ipairs(placeRows) do
        local place = places[index + offset.places]
        row.value = place
        if place then
            row.text:SetText((place == view.place and "|cffffd100" or "") .. place.name)
            row:Show()
        else
            row:Hide()
        end
    end

    for index, row in ipairs(bossRows) do
        local boss = bosses[index + offset.bosses]
        row.value = boss
        if boss then
            row.text:SetText((boss == view.boss and view.search == "" and "|cffffd100" or "") .. boss.name)
            row:Show()
        else
            row:Hide()
        end
    end

    for index, row in ipairs(itemRows) do
        local line = lines[index + offset.lines]
        if not line then
            row.item = nil
            row:Hide()
        elseif line.header then
            row.item, row.note = nil, nil
            row.icon:SetTexture(nil)
            row.name:SetText("|cffffd100" .. line.header .. "|r")
            row.right:SetText("for a")
            row:Show()
        else
            local name, quality = ItemName(line.item)
            row.item, row.note = line.item, line.note
            row.icon:SetTexture(GetItemIcon(line.item) or "Interface\\Icons\\INV_Misc_QuestionMark")
            row.name:SetText("|cff" .. (QUALITY[quality] or "ffffff") .. name .. "|r")
            row.right:SetText(line.right or "")
            row:Show()
        end
    end

    if view.search ~= "" then
        heading:SetText(#lines == 0 and "Nothing found" or (#lines >= 300 and "The first 300 found" or (#lines .. " found")))
    elseif view.boss then
        heading:SetText(view.boss.name .. (#lines > ITEM_ROWS and "  |cffaaaaaa(scroll for more)|r" or ""))
    else
        heading:SetText("")
    end

    for index, button in ipairs(modeButtons) do
        local mode = view.search == "" and view.boss and view.boss.modes[index]
        if mode and #view.boss.modes > 1 then
            button:SetText(mode.label ~= "" and mode.label or "Normal")
            if index == view.mode then button:LockHighlight() else button:UnlockHighlight() end
            button:Show()
        else
            button:Hide()
        end
    end

    if view.raids then raidTab:LockHighlight() dungeonTab:UnlockHighlight() else dungeonTab:LockHighlight() raidTab:UnlockHighlight() end
end

-- Names of items the game had not loaded come in a moment after the first look
local sinceRefresh = 0
frame:SetScript("OnUpdate", function(self, elapsed)
    sinceRefresh = sinceRefresh + elapsed
    if sinceRefresh >= 1.5 then
        sinceRefresh = 0
        Refresh()
    end
end)

frame:SetScript("OnShow", function()
    if not view.place then
        BuildPlaces()
        -- Open at the first dungeon that suits the character's level
        local level = UnitLevel("player")
        view.place = places[1]
        for _, place in ipairs(places) do
            if place.level <= level + 3 then view.place = place end
        end
        view.boss = view.place and view.place.bosses[1]
    end
    Refresh()
end)

local function Toggle()
    if #DATA == 0 then
        DEFAULT_CHAT_FRAME:AddMessage("Loot book: there is no list yet. The server writes it when it starts (it needs Python); restart the game afterwards.")
        return
    end
    if frame:IsShown() then frame:Hide() else frame:Show() end
end

SLASH_WOWPLUSLOOTBOOK1 = "/lootbook"
SLASH_WOWPLUSLOOTBOOK2 = "/lb"
SlashCmdList["WOWPLUSLOOTBOOK"] = Toggle
