-- WowPlus Guide: a book of the world for this server. It grew out of the Loot Book.
--
--   Dungeons, Raids  where each place is and for what levels, its bosses (a model you can turn, and
--                    their story), what each boss drops (the game's own loot and what is new here),
--                    the quests that belong to it with who gives them, and a map with the bosses on it
--                    (drawn by Settings/Scripts/Make-Dungeon-Maps.py).
--   Champions        the named champions of every zone: where they live, what they drop, the bounty.
--   Lore             the story of the zone you are in and the places in it, and of every other zone.
--
-- The data files are written by the server at every start (Settings/Scripts/Make-Boss-Loot.py makes
-- Loot.lua, Settings/Scripts/Make-Guide.py makes Places.lua and Lore.lua), so the guide always matches
-- this server. The boss stories and the lore are from Spoken (github.com/rusty-key/spoken-wow), written
-- from warcraft.wiki.gg: CC BY-SA 4.0. See Settings/Docs/Guide.md.
--
-- /guide opens and closes it (/guide lore opens the lore of where you are, /guide map a dungeon's map).
-- /lootbook still works.

local LOOT, ITEMS = WowPlusGuideLoot or {}, WowPlusGuideItems or {}
local PLACES, CHAMPIONS, LORE = WowPlusGuidePlaces or {}, WowPlusGuideChampions or {}, WowPlusGuideLore or {}
local MAPS = WowPlusGuideMaps or {}
for id, item in pairs(WowPlusGuideMoreItems or {}) do
    if not ITEMS[id] then ITEMS[id] = item end
end

local WIDTH, HEIGHT = 960, 620
local LIST_ROWS, LIST_ROW = 17, 30
local BOSS_ROWS, LOOT_ROWS, QUEST_ROWS, RESULT_ROWS = 14, 16, 12, 18
local HEADING_FONT = "Fonts\\MORPHEUS.TTF"
local CLASS_TOKENS = { [1] = "WARRIOR", [2] = "PALADIN", [3] = "HUNTER", [4] = "ROGUE", [5] = "PRIEST", [6] = "DEATHKNIGHT",
                       [7] = "SHAMAN", [8] = "MAGE", [9] = "WARLOCK", [11] = "DRUID" }
local QUALITY = { [0] = "9d9d9d", [1] = "ffffff", [2] = "1eff00", [3] = "0070dd", [4] = "a335ee", [5] = "ff8000", [6] = "e6cc80" }
local GOLD, GREY, INK = "|cffffd100", "|cffa0a0a0", { 0.20, 0.12, 0.05 }

local db                                            -- WowPlusGuideDB, per character
local view = { tab = 1, place = nil, boss = nil, mode = 1, section = "bosses", champion = nil, zone = nil, search = "" }
local offset = { list = 0, bosses = 0, loot = 0, quests = 0, results = 0 }
local list, bosses, loot, quests, results = {}, {}, {}, {}, {}
local Refresh

-- ---------------------------------------------------------------------------------------------
-- The places: the loot book's dungeons and raids, with what Places.lua knows about each
-- ---------------------------------------------------------------------------------------------

local dungeons, raids = {}, {}
do
    local byMap = {}
    for _, place in ipairs(LOOT) do byMap[place.map] = place end
    for map, info in pairs(PLACES) do
        local place = { map = map, name = info.name, raid = info.raid, info = info, bosses = byMap[map] and byMap[map].bosses or {} }
        place.from = info.levels and info.levels[1] or (byMap[map] and byMap[map].level) or 0
        place.to = info.levels and info.levels[2] or place.from
        if #place.bosses > 0 or info.quests then
            tinsert(place.raid and raids or dungeons, place)
        end
    end
    local function byLevel(a, b)
        if a.from ~= b.from then return a.from < b.from end
        return a.name < b.name
    end
    sort(dungeons, byLevel)
    sort(raids, byLevel)
end

local zones = {}
for name in pairs(LORE) do
    if name ~= "Azeroth" and name ~= "Kalimdor" and name ~= "Eastern Kingdoms" then tinsert(zones, name) end
end
sort(zones)

-- ---------------------------------------------------------------------------------------------
-- Small helpers
-- ---------------------------------------------------------------------------------------------

local function Shown(region, on)                    -- the 3.3.5 game has no :SetShown
    if on then region:Show() else region:Hide() end
end

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

local function Levels(place)
    if not place.from or place.from == 0 then return "" end
    if place.to and place.to ~= place.from then return place.from .. "-" .. place.to end
    return tostring(place.from)
end

local function Icon(place)
    local picture = place.info and place.info.picture
    return picture and ("Interface\\LFGFrame\\LFGIcon-" .. picture) or "Interface\\Icons\\INV_Misc_Book_09"
end

local function Heading(parent, size, layer)
    local text = parent:CreateFontString(nil, layer or "OVERLAY")
    text:SetFont(HEADING_FONT, size, "")
    text:SetTextColor(1, 0.82, 0.25)
    text:SetShadowColor(0, 0, 0, 1)
    text:SetShadowOffset(1, -1)
    return text
end

-- A scrolling column of text. Returns the scroll frame; :SetPieces({{heading, text}, ...}) fills it, and
-- :ScrollToPiece(i) shows piece i at the top.
local scrollNumber = 0
local function TextScroll(parent, width, height, dark)
    scrollNumber = scrollNumber + 1
    local scroll = CreateFrame("ScrollFrame", "WowPlusGuideScroll" .. scrollNumber, parent, "UIPanelScrollFrameTemplate")
    scroll:SetWidth(width)
    scroll:SetHeight(height)
    local child = CreateFrame("Frame", nil, scroll)
    child:SetWidth(width)
    child:SetHeight(height)
    scroll:SetScrollChild(child)
    scroll.child, scroll.strings = child, {}

    function scroll:SetPieces(pieces)
        for _, text in ipairs(self.strings) do text:Hide() end
        local y, used = 4, 0
        self.tops = {}
        for index, piece in ipairs(pieces) do
            if piece[1] and piece[1] ~= "" then
                used = used + 1
                local head = self.strings[used] or Heading(child, 15)
                self.strings[used] = head
                head:ClearAllPoints()
                head:SetPoint("TOPLEFT", 4, -y)
                head:SetWidth(width - 12)
                head:SetJustifyH("LEFT")
                head:SetText(piece[1])
                if piece.here then head:SetTextColor(0.4, 1, 0.4) elseif dark then head:SetTextColor(0.45, 0.18, 0.02) else head:SetTextColor(1, 0.82, 0.25) end
                head:Show()
                self.tops[index] = y
                y = y + head:GetStringHeight() + 4
            else
                self.tops[index] = y
            end
            used = used + 1
            local body = self.strings[used] or child:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
            self.strings[used] = body
            body:ClearAllPoints()
            body:SetPoint("TOPLEFT", 4, -y)
            body:SetWidth(width - 12)
            body:SetJustifyH("LEFT")
            body:SetSpacing(2)
            body:SetText(piece[2] or "")
            if dark then body:SetTextColor(INK[1], INK[2], INK[3]) body:SetShadowOffset(0, 0) else body:SetTextColor(0.92, 0.9, 0.85) end
            body:Show()
            y = y + body:GetStringHeight() + 14
        end
        child:SetHeight(math.max(y, height))
        self:UpdateScrollChildRect()
        self:SetVerticalScroll(0)
    end

    function scroll:ScrollToPiece(index)
        local top = self.tops and self.tops[index]
        if top then
            self:UpdateScrollChildRect()
            self:SetVerticalScroll(math.min(top, self:GetVerticalScrollRange()))
        end
    end
    return scroll
end

-- ---------------------------------------------------------------------------------------------
-- The window
-- ---------------------------------------------------------------------------------------------

local frame = CreateFrame("Frame", "WowPlusGuideFrame", UIParent)
frame:SetWidth(WIDTH)
frame:SetHeight(HEIGHT)
frame:SetPoint("CENTER", 0, 30)
frame:SetBackdrop({ bgFile = "Interface\\DialogFrame\\UI-DialogBox-Background-Dark", edgeFile = "Interface\\DialogFrame\\UI-DialogBox-Gold-Border",
                    tile = true, tileSize = 32, edgeSize = 32, insets = { left = 11, right = 11, top = 11, bottom = 11 } })
frame:SetMovable(true)
frame:SetClampedToScreen(true)
frame:EnableMouse(true)
frame:RegisterForDrag("LeftButton")
frame:SetScript("OnDragStart", frame.StartMoving)
frame:SetScript("OnDragStop", frame.StopMovingOrSizing)
frame:SetFrameStrata("HIGH")
frame:SetToplevel(true)
frame:Hide()
tinsert(UISpecialFrames, "WowPlusGuideFrame")

local header = frame:CreateTexture(nil, "ARTWORK")
header:SetTexture("Interface\\DialogFrame\\UI-DialogBox-Header")
header:SetWidth(320)
header:SetHeight(64)
header:SetPoint("TOP", 0, 12)
local title = Heading(frame, 18)
title:SetPoint("TOP", header, "TOP", 0, -14)
title:SetText("The Guide")

local close = CreateFrame("Button", nil, frame, "UIPanelCloseButton")
close:SetPoint("TOPRIGHT", -6, -6)

-- The list on the left, on a dark strip
local listBack = frame:CreateTexture(nil, "BACKGROUND")
listBack:SetTexture(0, 0, 0, 0.45)
listBack:SetPoint("TOPLEFT", 14, -32)
listBack:SetWidth(232)
listBack:SetHeight(HEIGHT - 82)

local listRows = {}
local listHolder = CreateFrame("Frame", nil, frame)
listHolder:SetPoint("TOPLEFT", 18, -38)
listHolder:SetWidth(226)
listHolder:SetHeight(LIST_ROWS * LIST_ROW)
listHolder:EnableMouseWheel(true)
listHolder:SetScript("OnMouseWheel", function(self, delta)
    offset.list = offset.list - delta * 3
    Refresh()
end)
for index = 1, LIST_ROWS do
    local row = CreateFrame("Button", nil, listHolder)
    row:SetWidth(226)
    row:SetHeight(LIST_ROW)
    row:SetPoint("TOPLEFT", 0, -(index - 1) * LIST_ROW)
    row:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight", "ADD")
    row.selected = row:CreateTexture(nil, "BACKGROUND")
    row.selected:SetTexture("Interface\\QuestFrame\\UI-QuestLogTitleHighlight")
    row.selected:SetBlendMode("ADD")
    row.selected:SetVertexColor(1, 0.8, 0.2, 0.7)
    row.selected:SetAllPoints()
    row.icon = row:CreateTexture(nil, "ARTWORK")
    row.icon:SetWidth(26)
    row.icon:SetHeight(26)
    row.icon:SetPoint("LEFT", 2, 0)
    row.name = row:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    row.name:SetPoint("LEFT", 32, 0)
    row.name:SetWidth(150)
    row.name:SetJustifyH("LEFT")
    row.right = row:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    row.right:SetPoint("RIGHT", -4, 0)
    row.right:SetJustifyH("RIGHT")
    row:SetScript("OnClick", function(self)
        if not self.value then return end
        if view.tab == 3 then
            view.champion = self.value
        elseif view.tab == 4 then
            view.zone = self.value
        else
            view.place, view.boss, view.mode = self.value, self.value.bosses[1], 1
            if not (view.section == "map" and MAPS[self.value.map]) then
                view.section = #self.value.bosses > 0 and "bosses" or "quests"
            end
            offset.bosses, offset.loot, offset.quests = 0, 0, 0
        end
        view.search = ""
        WowPlusGuideSearch:SetText("")
        Refresh()
    end)
    listRows[index] = row
end

-- The banner: the place's own picture from the dungeon finder, with its name over it
local content = CreateFrame("Frame", nil, frame)
content:SetPoint("TOPLEFT", 256, -32)
content:SetPoint("BOTTOMRIGHT", -16, 46)

local banner = content:CreateTexture(nil, "BACKGROUND")
banner:SetPoint("TOPLEFT")
banner:SetPoint("TOPRIGHT")
banner:SetHeight(150)
local bannerShade = content:CreateTexture(nil, "BORDER")
bannerShade:SetTexture(0, 0, 0, 1)
bannerShade:SetPoint("BOTTOMLEFT", banner, "BOTTOMLEFT")
bannerShade:SetPoint("BOTTOMRIGHT", banner, "BOTTOMRIGHT")
bannerShade:SetHeight(90)
bannerShade:SetGradientAlpha("VERTICAL", 0, 0, 0, 0.95, 0, 0, 0, 0)
local bannerLine = content:CreateTexture(nil, "ARTWORK")
bannerLine:SetTexture("Interface\\AchievementFrame\\UI-Achievement-Divider")
bannerLine:SetPoint("TOPLEFT", banner, "BOTTOMLEFT", -8, 8)
bannerLine:SetPoint("TOPRIGHT", banner, "BOTTOMRIGHT", 8, 8)
bannerLine:SetHeight(16)

local placeName = Heading(content, 30)
placeName:SetPoint("BOTTOMLEFT", banner, "BOTTOMLEFT", 14, 30)
placeName:SetJustifyH("LEFT")
local placeLine = content:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
placeLine:SetPoint("BOTTOMLEFT", banner, "BOTTOMLEFT", 16, 10)
placeLine:SetJustifyH("LEFT")

-- Section buttons under the banner: Bosses and Quests
local sectionButtons = {}
local function SectionButton(text, key, x)
    local button = CreateFrame("Button", nil, content, "UIPanelButtonTemplate")
    button:SetWidth(110)
    button:SetHeight(22)
    button:SetPoint("TOPLEFT", banner, "BOTTOMLEFT", x, -10)
    button:SetText(text)
    button:SetScript("OnClick", function()
        view.section, view.search = key, ""
        WowPlusGuideSearch:SetText("")
        Refresh()
    end)
    sectionButtons[key] = button
end
SectionButton("Bosses", "bosses", 0)
SectionButton("Quests", "quests", 116)
SectionButton("Map", "map", 232)

local below = 186                                   -- where the sections start, from the top of the content

-- Bosses: the list, the model and story, the loot
local bossPane = CreateFrame("Frame", nil, content)
bossPane:SetPoint("TOPLEFT", 0, -below)
bossPane:SetPoint("BOTTOMRIGHT")

local bossRows = {}
for index = 1, BOSS_ROWS do
    local row = CreateFrame("Button", nil, bossPane)
    row:SetWidth(170)
    row:SetHeight(24)
    row:SetPoint("TOPLEFT", 0, -(index - 1) * 26)
    row:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight", "ADD")
    row.selected = row:CreateTexture(nil, "BACKGROUND")
    row.selected:SetTexture("Interface\\QuestFrame\\UI-QuestLogTitleHighlight")
    row.selected:SetBlendMode("ADD")
    row.selected:SetVertexColor(1, 0.8, 0.2, 0.7)
    row.selected:SetAllPoints()
    row.skull = row:CreateTexture(nil, "ARTWORK")
    row.skull:SetTexture("Interface\\TargetingFrame\\UI-TargetingFrame-Skull")
    row.skull:SetWidth(16)
    row.skull:SetHeight(16)
    row.skull:SetPoint("LEFT", 2, 0)
    row.name = row:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
    row.name:SetPoint("LEFT", 22, 0)
    row.name:SetWidth(146)
    row.name:SetJustifyH("LEFT")
    row:SetScript("OnClick", function(self)
        if self.value then
            view.boss, view.mode, offset.loot = self.value, 1, 0
            Refresh()
        end
    end)
    bossRows[index] = row
end
bossPane:EnableMouseWheel(true)
bossPane:SetScript("OnMouseWheel", function(self, delta)
    offset.bosses = offset.bosses - delta * 3
    Refresh()
end)

-- The boss himself: a model you can turn with the mouse
local modelBack = bossPane:CreateTexture(nil, "BACKGROUND")
modelBack:SetPoint("TOPLEFT", 180, 0)
modelBack:SetWidth(240)
modelBack:SetHeight(200)
modelBack:SetTexture(0, 0, 0, 0.55)
local model = CreateFrame("PlayerModel", nil, bossPane)
model:SetPoint("TOPLEFT", modelBack)
model:SetPoint("BOTTOMRIGHT", modelBack)
model.facing = 0
model:EnableMouse(true)
model:SetScript("OnMouseDown", function(self) self.turning = GetCursorPosition() end)
model:SetScript("OnMouseUp", function(self) self.turning = nil end)
model:SetScript("OnUpdate", function(self)
    if self.turning then
        local x = GetCursorPosition()
        self.facing = self.facing + (x - self.turning) * 0.02
        self.turning = x
        self:SetFacing(self.facing)
    end
end)
local modelName = Heading(bossPane, 16)
modelName:SetPoint("BOTTOM", modelBack, "BOTTOM", 0, 6)
modelName:SetWidth(232)

local function ShowCreature(entry)
    model.entry = entry
    model:ClearModel()
    if entry and model.SetCreature then
        model:SetCreature(entry)
        model.facing = 0
        model:SetFacing(0)
    end
end

-- The story, on parchment
local storyBack = bossPane:CreateTexture(nil, "BACKGROUND")
storyBack:SetTexture("Interface\\AchievementFrame\\UI-Achievement-Parchment-Horizontal")
storyBack:SetPoint("TOPLEFT", modelBack, "BOTTOMLEFT", 0, -6)
storyBack:SetPoint("BOTTOMRIGHT", bossPane, "BOTTOMLEFT", 420, 0)
local story = TextScroll(bossPane, 214, 1, true)
story:SetPoint("TOPLEFT", storyBack, "TOPLEFT", 4, -4)
story:SetPoint("BOTTOMRIGHT", storyBack, "BOTTOMRIGHT", -24, 4)
story.child:SetWidth(214)

-- Loot: icon, name, chance or class
local lootHolder = CreateFrame("Frame", nil, bossPane)
lootHolder:SetPoint("TOPLEFT", 432, 0)
lootHolder:SetPoint("BOTTOMRIGHT", 0, 26)
lootHolder:EnableMouseWheel(true)
lootHolder:SetScript("OnMouseWheel", function(self, delta)
    offset.loot = offset.loot - delta * 3
    Refresh()
end)

local function ItemRow(parent, width, height)
    local row = CreateFrame("Button", nil, parent)
    row:SetHeight(height)
    row:SetWidth(width)
    row:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight", "ADD")
    row.icon = row:CreateTexture(nil, "ARTWORK")
    row.icon:SetWidth(height - 2)
    row.icon:SetHeight(height - 2)
    row.icon:SetPoint("LEFT", 2, 0)
    row.name = row:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    row.name:SetPoint("LEFT", height + 4, 0)
    row.name:SetWidth(width - height - 90)
    row.name:SetJustifyH("LEFT")
    row.right = row:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    row.right:SetPoint("RIGHT", -4, 0)
    row.right:SetWidth(84)
    row.right:SetJustifyH("RIGHT")
    row:SetScript("OnEnter", function(self)
        if not self.item then return end
        GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
        GameTooltip:SetHyperlink("item:" .. self.item)  -- also asks the server for an item the game has not seen yet
        if self.note then GameTooltip:AddLine(self.note, 1, 0.82, 0, true) end
        GameTooltip:Show()
    end)
    row:SetScript("OnLeave", function() GameTooltip:Hide() end)
    row:SetScript("OnClick", function(self)
        if not self.item then return end
        local _, link = GetItemInfo(self.item)
        if not link then
            DEFAULT_CHAT_FRAME:AddMessage("Guide: hover over the item for a moment first, so that the game loads it.")
        elseif IsModifiedClick("CHATLINK") then
            ChatEdit_InsertLink(link)
        elseif IsModifiedClick("DRESSUP") then
            DressUpItemLink(link)
        end
    end)
    return row
end

local function FillItemRow(row, line)
    if not line then
        row.item = nil
        row:Hide()
    elseif line.header then
        row.item, row.note = nil, nil
        row.icon:SetTexture(nil)
        row.name:SetText(GOLD .. line.header .. "|r")
        row.right:SetText(line.right or "")
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

local lootRows = {}
for index = 1, LOOT_ROWS do
    local row = ItemRow(lootHolder, 248, 22)
    row:SetPoint("TOPLEFT", 0, -(index - 1) * 23)
    lootRows[index] = row
end

local modeButtons = {}
for index = 1, 4 do
    local button = CreateFrame("Button", nil, bossPane, "UIPanelButtonTemplate")
    button:SetWidth(62)
    button:SetHeight(20)
    button:SetPoint("BOTTOMLEFT", 432 + (index - 1) * 63, 0)
    button:GetFontString():SetFont(GameFontNormalSmall:GetFont())
    button:SetScript("OnClick", function()
        view.mode, offset.loot = index, 0
        Refresh()
    end)
    modeButtons[index] = button
end

-- Quests: a row each, the details on hover
local questPane = CreateFrame("Frame", nil, content)
questPane:SetPoint("TOPLEFT", 0, -below)
questPane:SetPoint("BOTTOMRIGHT")
questPane:EnableMouseWheel(true)
questPane:SetScript("OnMouseWheel", function(self, delta)
    offset.quests = offset.quests - delta * 3
    Refresh()
end)
local questRows = {}
for index = 1, QUEST_ROWS do
    local row = CreateFrame("Button", nil, questPane)
    row:SetHeight(30)
    row:SetPoint("TOPLEFT", 0, -(index - 1) * 31)
    row:SetPoint("RIGHT")
    row:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight", "ADD")
    row.back = row:CreateTexture(nil, "BACKGROUND")
    row.back:SetAllPoints()
    row.back:SetTexture(1, 1, 1, index % 2 == 0 and 0.04 or 0)
    row.side = row:CreateTexture(nil, "ARTWORK")
    row.side:SetWidth(26)
    row.side:SetHeight(26)
    row.side:SetPoint("LEFT", 0, -2)
    row.level = row:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
    row.level:SetPoint("LEFT", 28, 0)
    row.level:SetWidth(30)
    row.title = row:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    row.title:SetPoint("LEFT", 62, 0)
    row.title:SetWidth(320)
    row.title:SetJustifyH("LEFT")
    row.giver = row:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    row.giver:SetPoint("RIGHT", -6, 0)
    row.giver:SetWidth(300)
    row.giver:SetJustifyH("RIGHT")
    row:SetScript("OnEnter", function(self)
        local q = self.value
        if not q then return end
        GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
        GameTooltip:AddLine(q[2], 1, 0.82, 0)
        GameTooltip:AddLine((q[1] > 0 and ("Level " .. q[1]) or "Your level") .. (q[3] == "A" and ", Alliance" or q[3] == "H" and ", Horde" or ""), 0.7, 0.7, 0.7)
        GameTooltip:AddLine("From " .. q[4] .. (q[5] ~= "" and (", " .. q[5]) or ""), 1, 1, 1, true)
        if q[6] ~= "" then
            GameTooltip:AddLine(" ")
            GameTooltip:AddLine(q[6], 1, 1, 1, true)
        end
        GameTooltip:Show()
    end)
    row:SetScript("OnLeave", function() GameTooltip:Hide() end)
    questRows[index] = row
end
local questNone = questPane:CreateFontString(nil, "OVERLAY", "GameFontDisable")
questNone:SetPoint("TOP", 0, -30)
questNone:SetText("No quests are known to lead here.")

-- The map: the picture on the left, the bosses by number on the right. It covers the banner, for the room.
local mapPane = CreateFrame("Frame", nil, content)
mapPane:SetAllPoints()
mapPane:SetFrameLevel(content:GetFrameLevel() + 6)
mapPane:EnableMouse(true)
local mapBack = mapPane:CreateTexture(nil, "BACKGROUND")
mapBack:SetAllPoints()
mapBack:SetTexture(0.04, 0.03, 0.02, 1)
local MAP_SIZE = 512
local mapPicture = mapPane:CreateTexture(nil, "ARTWORK")
mapPicture:SetWidth(MAP_SIZE)
mapPicture:SetHeight(MAP_SIZE)
mapPicture:SetPoint("TOPLEFT", 0, -8)
local mapName = Heading(mapPane, 20)
mapName:SetPoint("TOPLEFT", MAP_SIZE + 10, -10)
mapName:SetWidth(162)
mapName:SetJustifyH("LEFT")
local mapLine = mapPane:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
mapLine:SetPoint("TOPLEFT", mapName, "BOTTOMLEFT", 0, -4)
mapLine:SetWidth(162)
mapLine:SetJustifyH("LEFT")

local function GoToBoss(boss)
    view.section, view.boss, view.mode, offset.loot = "bosses", boss, 1, 0
    Refresh()
end

-- A pin: a dark round mark with a number on it
local mapPins, mapRows = {}, {}
local function MapPin(index)
    local pin = mapPins[index]
    if pin then return pin end
    pin = CreateFrame("Button", nil, mapPane)
    pin:SetWidth(22)
    pin:SetHeight(22)
    pin:SetFrameLevel(mapPane:GetFrameLevel() + 2)
    pin.disc = pin:CreateTexture(nil, "ARTWORK")
    pin.disc:SetTexture("Interface\\Minimap\\UI-Minimap-Background")
    pin.disc:SetAllPoints()
    pin.number = pin:CreateFontString(nil, "OVERLAY")
    pin.number:SetFont("Fonts\\FRIZQT__.TTF", 12, "OUTLINE")
    pin.number:SetPoint("CENTER", 0, 0)
    pin:SetHighlightTexture("Interface\\Minimap\\UI-Minimap-ZoomButton-Highlight", "ADD")
    pin:SetScript("OnEnter", function(self)
        GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
        GameTooltip:AddLine(self.label, 1, 0.82, 0)
        if self.boss then GameTooltip:AddLine("Click for this boss", 0.7, 0.7, 0.7) end
        GameTooltip:Show()
        if self.row then self.row:LockHighlight() end
    end)
    pin:SetScript("OnLeave", function(self)
        GameTooltip:Hide()
        if self.row then self.row:UnlockHighlight() end
    end)
    pin:SetScript("OnClick", function(self) if self.boss then GoToBoss(self.boss) end end)
    mapPins[index] = pin
    return pin
end

for index = 1, 18 do
    local row = CreateFrame("Button", nil, mapPane)
    row:SetWidth(166)
    row:SetHeight(22)
    row:SetPoint("TOPLEFT", MAP_SIZE + 6, -70 - (index - 1) * 22)
    row:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight", "ADD")
    row.number = row:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
    row.number:SetPoint("LEFT", 2, 0)
    row.number:SetWidth(18)
    row.name = row:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    row.name:SetPoint("LEFT", 22, 0)
    row.name:SetWidth(142)
    row.name:SetJustifyH("LEFT")
    row:SetScript("OnEnter", function(self) if self.pin then self.pin:LockHighlight() end end)
    row:SetScript("OnLeave", function(self) if self.pin then self.pin:UnlockHighlight() end end)
    row:SetScript("OnClick", function(self) if self.boss then GoToBoss(self.boss) end end)
    mapRows[index] = row
end

local function ShowMap(place)
    local data = MAPS[place.map]
    if not data then return end
    if not mapPicture:SetTexture("Interface\\AddOns\\WowPlusGuide\\Maps\\" .. place.map) then
        mapPicture:SetTexture(0.2, 0.15, 0.1, 1)                -- drawn after the game started: it needs a restart to see new pictures
    end
    mapName:SetText(place.name)
    mapLine:SetText(Levels(place) ~= "" and ("Levels " .. Levels(place)) or "")
    local used = 0
    local function Pin(at, label, text, r, g, b)
        used = used + 1
        local pin = MapPin(used)
        pin:ClearAllPoints()
        pin:SetPoint("CENTER", mapPicture, "TOPLEFT", at[1] / 100 * MAP_SIZE, -at[2] / 100 * MAP_SIZE)
        pin.label, pin.boss, pin.row = label, nil, nil
        pin.number:SetText(text)
        pin.disc:SetVertexColor(r, g, b)
        pin:Show()
        return pin
    end
    for _, door in ipairs(data.doors or {}) do Pin(door, "The way in", "E", 0.1, 0.6, 0.15) end
    for index, row in ipairs(mapRows) do
        local boss = place.bosses[index]
        row.boss, row.pin = boss, nil
        if boss then
            local at = data.bosses and data.bosses[boss.entry]
            row.number:SetText(at and index or "-")
            row.name:SetText(boss.name)
            if at then
                local pin = Pin(at, boss.name, index, 0.65, 0.08, 0.05)
                pin.boss, pin.row = boss, row
                row.pin = pin
            end
            row:Show()
        else
            row:Hide()
        end
    end
    for i = used + 1, #mapPins do mapPins[i]:Hide() end
end

-- Champions: one champion, its model, where, its drops
local championPane = CreateFrame("Frame", nil, content)
championPane:SetPoint("TOPLEFT", 0, -below + 30)
championPane:SetPoint("BOTTOMRIGHT")
local championModelBack = championPane:CreateTexture(nil, "BACKGROUND")
championModelBack:SetPoint("TOPLEFT", 0, 0)
championModelBack:SetWidth(300)
championModelBack:SetHeight(320)
championModelBack:SetTexture(0, 0, 0, 0.55)
local championModel = CreateFrame("PlayerModel", nil, championPane)
championModel:SetPoint("TOPLEFT", championModelBack)
championModel:SetPoint("BOTTOMRIGHT", championModelBack)
championModel.facing = 0
championModel:EnableMouse(true)
championModel:SetScript("OnMouseDown", model:GetScript("OnMouseDown"))
championModel:SetScript("OnMouseUp", model:GetScript("OnMouseUp"))
championModel:SetScript("OnUpdate", model:GetScript("OnUpdate"))
local championText = championPane:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
championText:SetPoint("TOPLEFT", championModelBack, "BOTTOMLEFT", 4, -8)
championText:SetWidth(296)
championText:SetJustifyH("LEFT")
championText:SetSpacing(3)
local championLoot = CreateFrame("Frame", nil, championPane)
championLoot:SetPoint("TOPLEFT", 316, 0)
championLoot:SetPoint("BOTTOMRIGHT")
local championRows = {}
for index = 1, 16 do
    local row = ItemRow(championLoot, 360, 22)
    row:SetPoint("TOPLEFT", 0, -(index - 1) * 23)
    championRows[index] = row
end

-- Lore: the zone, then each place in it
local lorePane = CreateFrame("Frame", nil, content)
lorePane:SetPoint("TOPLEFT", 0, -below + 30)
lorePane:SetPoint("BOTTOMRIGHT")
local loreBack = lorePane:CreateTexture(nil, "BACKGROUND")
loreBack:SetTexture("Interface\\AchievementFrame\\UI-Achievement-Parchment-Horizontal")
loreBack:SetAllPoints()
local loreText = TextScroll(lorePane, 640, 1, true)
loreText:SetPoint("TOPLEFT", 10, -10)
loreText:SetPoint("BOTTOMRIGHT", -30, 24)
loreText.child:SetWidth(640)
local loreCredit = lorePane:CreateFontString(nil, "OVERLAY", "GameFontDisableSmall")
loreCredit:SetPoint("BOTTOMRIGHT", -10, 6)
loreCredit:SetText("Text from warcraft.wiki.gg (CC BY-SA 4.0), by way of Spoken")

-- Narration: Spoken's recordings of the same lore, played through Spoken's own player when it is installed
-- (Interface\AddOns\SpokenPlayer and SpokenZonesAudio; Settings/Docs/Voice.md)
local narrator
local function Narrator()
    if not narrator and Spoken and Spoken.IsCompatible and Spoken:IsCompatible(1) then
        narrator = Spoken:GetSource("zones") or Spoken:RegisterSource("zones", { title = "Zone lore", addon = "WowPlusGuide", order = 20, queueLimit = 2 })
    end
    return narrator
end

local function FindPlace(lore, name)
    if not lore or not name or name == "" then return nil end
    local wanted = strlower(name):gsub("^the ", "")
    for _, place in ipairs(lore.places) do
        if strlower(place[1]):gsub("^the ", "") == wanted then return place end
    end
end

-- Reads a zone's story, or one place's in it, aloud. Returns whether there was anything to read.
local function Narrate(zone, place)
    local lore, source = LORE[zone], Narrator()
    if not lore or not source then return false end
    local file, length, label, said = "zone", lore.secs, "", lore.full
    if place then file, length, label, said = place[4], place[5], place[1], place[2] end
    if not length or length <= 0 then return false end
    source:Enqueue({ key = "z:" .. lore.map .. ":" .. file,
                     path = "Interface\\AddOns\\SpokenZonesAudio\\Sounds\\" .. lore.map .. "\\" .. file .. ".mp3",
                     length = length,
                     present = { header = zone, label = label, transcript = said, portrait = { kind = "none" } } })
    return true
end

local listenZone = CreateFrame("Button", nil, lorePane, "UIPanelButtonTemplate")
listenZone:SetWidth(150)
listenZone:SetHeight(22)
listenZone:SetPoint("BOTTOMLEFT", 10, 4)
listenZone:SetScript("OnClick", function() Narrate(view.zone) end)
local listenPlace = CreateFrame("Button", nil, lorePane, "UIPanelButtonTemplate")
listenPlace:SetWidth(220)
listenPlace:SetHeight(22)
listenPlace:SetPoint("LEFT", listenZone, "RIGHT", 6, 0)
listenPlace:SetScript("OnClick", function(self) if self.place then Narrate(view.zone, self.place) end end)

-- Search results: every boss and champion that drops an item by that name
local resultPane = CreateFrame("Frame", nil, content)
resultPane:SetPoint("TOPLEFT", 0, -below + 30)
resultPane:SetPoint("BOTTOMRIGHT")
resultPane:EnableMouseWheel(true)
resultPane:SetScript("OnMouseWheel", function(self, delta)
    offset.results = offset.results - delta * 3
    Refresh()
end)
local resultHeading = Heading(resultPane, 18)
resultHeading:SetPoint("TOPLEFT", 0, 0)
local resultRows = {}
for index = 1, RESULT_ROWS do
    local row = ItemRow(resultPane, 680, 20)
    row:SetPoint("TOPLEFT", 0, -26 - (index - 1) * 21)
    row.name:SetWidth(300)
    row.right:SetWidth(330)
    resultRows[index] = row
end

-- Tabs along the bottom edge
local TAB_NAMES = { "Dungeons", "Raids", "Champions", "Lore" }
for index, name in ipairs(TAB_NAMES) do
    local tab = CreateFrame("Button", "WowPlusGuideFrameTab" .. index, frame, "CharacterFrameTabButtonTemplate")
    tab:SetID(index)
    tab:SetText(name)
    if index == 1 then tab:SetPoint("TOPLEFT", frame, "BOTTOMLEFT", 16, 8)
    else tab:SetPoint("LEFT", _G["WowPlusGuideFrameTab" .. (index - 1)], "RIGHT", -14, 0) end
    tab:SetScript("OnClick", function(self)
        view.tab, view.search, offset.list = self:GetID(), "", 0
        WowPlusGuideSearch:SetText("")
        PlaySound("igCharacterInfoTab")
        Refresh()
    end)
end
PanelTemplates_SetNumTabs(frame, #TAB_NAMES)
PanelTemplates_SetTab(frame, 1)

-- Search
local searchLabel = frame:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
searchLabel:SetPoint("BOTTOMLEFT", 262, 22)
searchLabel:SetText("Find an item:")
local search = CreateFrame("EditBox", "WowPlusGuideSearch", frame, "InputBoxTemplate")
search:SetWidth(220)
search:SetHeight(20)
search:SetPoint("LEFT", searchLabel, "RIGHT", 12, 0)
search:SetAutoFocus(false)
search:SetScript("OnTextChanged", function(self)
    local text = self:GetText() or ""
    view.search = strlen(text) >= 3 and text or ""
    offset.results = 0
    Refresh()
end)
search:SetScript("OnEscapePressed", function(self) self:ClearFocus() end)
search:SetScript("OnEnterPressed", function(self) self:ClearFocus() end)

-- ---------------------------------------------------------------------------------------------
-- What everything shows
-- ---------------------------------------------------------------------------------------------

local function BuildResults()
    wipe(results)
    local wanted = strlower(view.search)
    local function Look(id, right, from)
        local known = ITEMS[id]
        if #results < 300 and known and strfind(strlower(known[1]), wanted, 1, true) then
            tinsert(results, { item = id, right = right .. "  " .. GREY .. from .. "|r", note = from })
        end
    end
    for _, place in ipairs(LOOT) do
        for _, boss in ipairs(place.bosses) do
            for _, mode in ipairs(boss.modes) do
                local from = boss.name .. ", " .. place.name .. (mode.label ~= "" and (" (" .. mode.label .. ")") or "")
                for _, drop in ipairs(mode.o) do Look(drop[1], drop[2] .. "%", from) end
                for _, drop in ipairs(mode.n) do Look(drop[1], "new", from) end
            end
        end
    end
    for _, champion in ipairs(CHAMPIONS) do
        for _, drop in ipairs(champion[8]) do Look(drop[1], drop[2] .. "%", champion[2] .. ", " .. champion[4]) end
    end
end

local function BuildLoot()
    wipe(loot)
    local boss = view.boss
    local mode = boss and (boss.modes[view.mode] or boss.modes[1])
    if not mode then return end
    for _, drop in ipairs(mode.o) do tinsert(loot, { item = drop[1], right = drop[2] .. "%" }) end
    if #mode.n > 0 then
        tinsert(loot, { header = "New on this server", right = GREY .. "for a|r" })
        for _, drop in ipairs(mode.n) do tinsert(loot, { item = drop[1], right = ClassName(drop[2]) }) end
    end
end

local function Clamp(key, total, shown)
    offset[key] = math.max(0, math.min(offset[key], math.max(0, total - shown)))
end

local function ShowList()
    wipe(list)
    if view.tab == 1 then for _, p in ipairs(dungeons) do tinsert(list, p) end
    elseif view.tab == 2 then for _, p in ipairs(raids) do tinsert(list, p) end
    elseif view.tab == 3 then for _, c in ipairs(CHAMPIONS) do tinsert(list, c) end
    else
        local here = GetRealZoneText()
        if LORE[here] then tinsert(list, here) end
        for _, zone in ipairs(zones) do if zone ~= here then tinsert(list, zone) end end
    end
    Clamp("list", #list, LIST_ROWS)
    local here = GetRealZoneText()
    for index, row in ipairs(listRows) do
        local value = list[index + offset.list]
        row.value = value
        if not value then
            row:Hide()
        else
            if view.tab <= 2 then
                row.icon:SetTexture(Icon(value))
                row.icon:SetTexCoord(0, 1, 0, 1)
                row.name:SetText(value.name)
                row.right:SetText(GREY .. Levels(value) .. "|r")
                Shown(row.selected, value == view.place)
            elseif view.tab == 3 then
                row.icon:SetTexture("Interface\\TargetingFrame\\UI-TargetingFrame-Skull")
                row.icon:SetTexCoord(0, 1, 0, 1)
                row.name:SetText(value[2])
                row.right:SetText(GREY .. value[1] .. "|r")
                Shown(row.selected, value == view.champion)
            else
                row.icon:SetTexture(value == here and "Interface\\Icons\\INV_Misc_Map_01" or "Interface\\Icons\\INV_Misc_Book_11")
                row.icon:SetTexCoord(0.07, 0.93, 0.07, 0.93)
                row.name:SetText(value)
                row.right:SetText(value == here and "|cff66ff66here|r" or "")
                Shown(row.selected, value == view.zone)
            end
            row:Show()
        end
    end
end

local function ShowBanner(picture, name, line)
    if not (picture and banner:SetTexture("Interface\\LFGFrame\\UI-LFG-BACKGROUND-" .. picture)) then
        banner:SetTexture("Interface\\AchievementFrame\\UI-Achievement-Parchment-Horizontal")
    end
    banner:SetTexCoord(0, 1, 0, 0.6)
    placeName:SetText(name or "")
    placeLine:SetText(line or "")
end

local function ShowPlace()
    local place = view.place
    local info = place.info or {}
    local where = info.where and string.format("Entrance in %s, %.0f, %.0f", info.where[1], info.where[2], info.where[3]) or ""
    local levels = Levels(place) ~= "" and ("Levels " .. Levels(place) .. (info.levels and info.levels[3] < place.from and (", open from " .. info.levels[3]) or "")) or ""
    ShowBanner(info.picture, place.name, levels .. (levels ~= "" and where ~= "" and ("     " .. GREY .. "/|r     ") or "") .. where)

    sectionButtons.bosses:SetText("Bosses (" .. #place.bosses .. ")")
    sectionButtons.quests:SetText("Quests (" .. (info.quests and #info.quests or 0) .. ")")
    local onMap = view.section == "map"
    for index, key in ipairs({ "bosses", "quests", "map" }) do
        local button = sectionButtons[key]
        button:ClearAllPoints()
        if onMap then                                   -- the map covers the banner: down the right-hand side instead
            button:SetPoint("BOTTOMRIGHT", content, "BOTTOMRIGHT", -30, (3 - index) * 24 + 2)
            button:SetFrameLevel(mapPane:GetFrameLevel() + 4)
        else
            button:SetPoint("TOPLEFT", banner, "BOTTOMLEFT", (index - 1) * 116, -10)
            button:SetFrameLevel(content:GetFrameLevel() + 1)
        end
        Shown(button, key ~= "map" or MAPS[place.map])
        if key == view.section then button:LockHighlight() else button:UnlockHighlight() end
    end
    Shown(mapPane, onMap)
    if onMap then
        ShowMap(place)
        return
    end

    Shown(bossPane, view.section == "bosses")
    Shown(questPane, view.section == "quests")
    if view.section == "bosses" then
        Clamp("bosses", #place.bosses, BOSS_ROWS)
        for index, row in ipairs(bossRows) do
            local boss = place.bosses[index + offset.bosses]
            row.value = boss
            if boss then
                row.name:SetText(boss.name)
                Shown(row.selected, boss == view.boss)
                row:Show()
            else
                row:Hide()
            end
        end
        local boss = view.boss
        if boss and model.entry ~= boss.entry then ShowCreature(boss.entry) end
        modelName:SetText(boss and boss.name or "")
        local key = boss and strlower(boss.name):gsub("%-", " "):gsub("[^%w ]", ""):gsub("%s+", " ") or ""
        local told = info.stories and info.stories[key]
        story:SetPieces({ { nil, told or (boss and GREY .. "No story of " .. boss.name .. " has been written down yet.|r" or "") } })

        BuildLoot()
        Clamp("loot", #loot, LOOT_ROWS)
        for index, row in ipairs(lootRows) do FillItemRow(row, loot[index + offset.loot]) end
        for index, button in ipairs(modeButtons) do
            local mode = boss and boss.modes[index]
            if mode and #boss.modes > 1 then
                button:SetText(mode.label ~= "" and mode.label or "Normal")
                if index == view.mode then button:LockHighlight() else button:UnlockHighlight() end
                button:Show()
            else
                button:Hide()
            end
        end
    else
        local all = info.quests or {}
        Clamp("quests", #all, QUEST_ROWS)
        for index, row in ipairs(questRows) do
            local q = all[index + offset.quests]
            row.value = q
            if q then
                row.side:SetTexture(q[3] == "A" and "Interface\\TargetingFrame\\UI-PVP-Alliance" or q[3] == "H" and "Interface\\TargetingFrame\\UI-PVP-Horde" or nil)
                row.side:SetTexCoord(0, 0.62, 0, 0.62)
                local colour = GetQuestDifficultyColor(q[1] > 0 and q[1] or UnitLevel("player"))
                row.level:SetText(q[1] > 0 and q[1] or "-")
                row.level:SetTextColor(colour.r, colour.g, colour.b)
                row.title:SetText(q[2])
                row.giver:SetText(GREY .. q[4] .. (q[5] ~= "" and (", " .. q[5]) or "") .. "|r")
                row:Show()
            else
                row:Hide()
            end
        end
        Shown(questNone, #all == 0)
    end
end

local function ShowChampion()
    local c = view.champion
    ShowBanner(nil, c[2], string.format("A champion among the %s, level %d", c[3], c[1]))
    banner:SetTexture("Interface\\AchievementFrame\\UI-Achievement-Parchment-Horizontal")
    banner:SetVertexColor(0.45, 0.35, 0.3)
    if championModel.entry ~= c[7] then
        championModel.entry = c[7]
        championModel:ClearModel()
        if championModel.SetCreature then championModel:SetCreature(c[7]) end
    end
    championText:SetText(string.format("%sLives in|r %s, around %.0f, %.0f.\n%s|r",
        GOLD, c[4], c[5], c[6],
        c[9] > 0 and (GOLD .. "Bounty:|r the innkeepers of " .. c[4] .. " pay for its head.") or ""))
    local drops = {}
    tinsert(drops, { header = "Drops", right = GREY .. "chance|r" })
    for _, drop in ipairs(c[8]) do tinsert(drops, { item = drop[1], right = drop[2] .. "%" }) end
    tinsert(drops, { header = "And always one item more, as rares do" })
    for index, row in ipairs(championRows) do FillItemRow(row, drops[index]) end
end

local function ShowLore()
    local zone = view.zone
    local lore = LORE[zone]
    ShowBanner(nil, zone, zone == GetRealZoneText() and "You are here" or "")
    banner:SetVertexColor(0.55, 0.45, 0.35)
    if not lore then return end
    local here, pieces, at = GetSubZoneText(), { { nil, lore.full } }, nil
    for _, place in ipairs(lore.places) do
        tinsert(pieces, { place[1], place[2], here = place[1] == here })
        if place[1] == here then at = #pieces end
    end
    loreText:SetPieces(pieces)
    if at then loreText:ScrollToPiece(at) end
    if db then db.read[zone] = true end

    local voiced = Narrator() and (lore.secs or 0) > 0
    listenZone:SetText("Listen: " .. zone)
    Shown(listenZone, voiced)
    local place = zone == GetRealZoneText() and FindPlace(lore, here)
    listenPlace.place = place
    if voiced and place and (place[5] or 0) > 0 then
        listenPlace:SetText("Listen: " .. place[1])
        listenPlace:Show()
    else
        listenPlace:Hide()
    end
end

function Refresh()
    ShowList()
    local searching = view.search ~= ""
    for _, button in pairs(sectionButtons) do button:Hide() end
    bossPane:Hide() questPane:Hide() championPane:Hide() lorePane:Hide() resultPane:Hide() mapPane:Hide()
    banner:SetVertexColor(1, 1, 1)
    PanelTemplates_SetTab(frame, view.tab)

    if searching then
        BuildResults()
        Clamp("results", #results, RESULT_ROWS)
        ShowBanner(nil, "Find an item", "")
        banner:SetVertexColor(0.45, 0.35, 0.3)
        resultHeading:SetText(#results == 0 and "Nothing found" or (#results >= 300 and "The first 300 found" or (#results .. " found")))
        for index, row in ipairs(resultRows) do FillItemRow(row, results[index + offset.results]) end
        resultPane:Show()
    elseif view.tab <= 2 then
        if not view.place or (view.place.raid and true or false) ~= (view.tab == 2) then
            local places = view.tab == 2 and raids or dungeons
            view.place = places[1]
            for _, place in ipairs(places) do if place.from <= UnitLevel("player") + 2 then view.place = place end end
            view.boss, view.mode = view.place and view.place.bosses[1], 1
            view.section = view.place and #view.place.bosses > 0 and "bosses" or "quests"
        end
        if view.place then ShowPlace() end
    elseif view.tab == 3 then
        if not view.champion then
            view.champion = CHAMPIONS[1]
            for _, c in ipairs(CHAMPIONS) do if c[1] <= UnitLevel("player") + 2 then view.champion = c end end
        end
        if view.champion then championPane:Show() ShowChampion() end
    else
        view.zone = view.zone or (LORE[GetRealZoneText()] and GetRealZoneText()) or zones[1]
        lorePane:Show()
        ShowLore()
    end
end

-- Names of items the game had not loaded come in a moment after the first look, and so do models
local sinceRefresh = 0
frame:SetScript("OnUpdate", function(self, elapsed)
    sinceRefresh = sinceRefresh + elapsed
    if sinceRefresh >= 1.5 then
        sinceRefresh = 0
        if view.tab <= 2 and view.section == "bosses" and view.search == "" then
            for index, row in ipairs(lootRows) do FillItemRow(row, loot[index + offset.loot]) end
        end
        if model:IsVisible() and model.entry and not model:GetModel() then ShowCreature(model.entry) end
    end
end)
frame:SetScript("OnShow", function()
    PlaySound("igSpellBookOpen")
    model.entry, championModel.entry = nil, nil        -- a model set while hidden does not show
    Refresh()
end)
frame:SetScript("OnHide", function() PlaySound("igSpellBookClose") end)

-- ---------------------------------------------------------------------------------------------
-- Opening it, and telling the player when a zone has lore to read
-- ---------------------------------------------------------------------------------------------

local function Toggle(arg)
    arg = strlower(strtrim(arg or ""))
    if #LOOT == 0 and next(PLACES) == nil then
        DEFAULT_CHAT_FRAME:AddMessage("Guide: there is nothing in it yet. The server writes it when it starts (it needs Python); restart the game afterwards.")
        return
    end
    if arg == "narrate on" or arg == "narrate off" then
        db.narrate = arg == "narrate on"
        DEFAULT_CHAT_FRAME:AddMessage("Guide: " .. (db.narrate and "the story of a zone or place is read aloud the first time you come there"
            .. (Narrator() and "." or ", once Spoken's player is installed (Settings/Docs/Voice.md).") or "no reading aloud by itself; the Listen buttons in Lore still work."))
        return
    end
    if arg == "map" then
        if view.tab > 2 then view.tab = 1 end
        view.search = ""
        if not frame:IsShown() then frame:Show() end
        if view.place and MAPS[view.place.map] then
            view.section = "map"
            Refresh()
        end
        return
    end
    if arg == "lore" then
        view.tab, view.zone, view.search = 4, LORE[GetRealZoneText()] and GetRealZoneText() or view.zone, ""
        WowPlusGuideSearch:SetText("")
        if frame:IsShown() then Refresh() else frame:Show() end
        return
    end
    if frame:IsShown() then frame:Hide() else frame:Show() end
end

SLASH_WOWPLUSGUIDE1 = "/guide"
SLASH_WOWPLUSGUIDE2 = "/lootbook"
SLASH_WOWPLUSGUIDE3 = "/lb"
SlashCmdList["WOWPLUSGUIDE"] = Toggle

local events = CreateFrame("Frame")
events:RegisterEvent("ADDON_LOADED")
events:RegisterEvent("ZONE_CHANGED_NEW_AREA")
events:RegisterEvent("ZONE_CHANGED")
events:SetScript("OnEvent", function(self, event, name)
    if event == "ADDON_LOADED" then
        if name ~= "WowPlusGuide" then return end
        WowPlusGuideDB = WowPlusGuideDB or {}
        db = WowPlusGuideDB
        db.read, db.heard = db.read or {}, db.heard or {}
        if db.narrate == nil then db.narrate = true end
        return
    end
    if not db then return end
    local zone = GetRealZoneText()
    local lore = LORE[zone]
    if lore and not db.read[zone] and not db["told" .. zone] then
        db["told" .. zone] = true
        DEFAULT_CHAT_FRAME:AddMessage("|cffffd100The Guide|r has the story of " .. zone .. ": |cff71d5ff/guide lore|r")
    end
    -- The first time you come to a zone, and to each place in it, its story is read aloud (once)
    if lore and db.narrate and not UnitOnTaxi("player") then
        if not db.heard[zone] and Narrate(zone) then db.heard[zone] = true end
        local place = FindPlace(lore, GetSubZoneText())
        local key = place and (zone .. "/" .. place[1])
        if key and not db.heard[key] and Narrate(zone, place) then db.heard[key] = true end
    end
    if frame:IsShown() and view.tab == 4 then Refresh() end
end)
