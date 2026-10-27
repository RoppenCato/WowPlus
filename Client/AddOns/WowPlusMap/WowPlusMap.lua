-- WowPlus Map: more on the game's own world map. Step 4 of Settings/Docs/Addons-Plan.md, first part.
--
--   Unexplored   the parts of a zone you have not found yet are drawn too, faintly, so the whole zone can
--                be read and exploring still shows
--   Pins         dungeon and raid entrances (with their levels), champions, flight masters, inns, and the
--                givers of this server's own quests you can still take (your side, your class, not done)
--   Coordinates  yours and the mouse's, at the top of the map
--
-- The pieces of each zone map and the pins are written by the server at every start (Settings/Scripts/
-- Make-Guide.py makes Data.lua), from the client's own tables and this server's database.
-- The row of boxes at the top of the map turns each kind on and off; /wpmap prints the same. Settings/Docs/Map.md.

local OVERLAYS, PINS = WowPlusMapOverlays or {}, WowPlusMapPins or {}
local KINDS = {
    { key = "fog", label = "Unexplored" },
    { key = "dungeon", label = "Dungeons", kinds = { dungeon = true, raid = true } },
    { key = "champion", label = "Champions", kinds = { champion = true } },
    { key = "travel", label = "Travel", kinds = { flight = true, inn = true } },
    { key = "quest", label = "Our quests", kinds = { quest = true } },
}
local ICONS = {
    champion = { "Interface\\TargetingFrame\\UI-TargetingFrame-Skull", 16 },
    flight = { "Interface\\TaxiFrame\\UI-Taxi-Icon-Green", 14 },
    inn = { "Interface\\Minimap\\Tracking\\Innkeeper", 16 },
    quest = { "Interface\\GossipFrame\\AvailableQuestIcon", 18 },
}
local FAINT = { 0.55, 0.5, 0.45, 0.75 }             -- how an unexplored piece is drawn

local db
local done = {}                                     -- quests this character has handed in (QueryQuestsCompleted)
local pieces, pins = {}, {}

-- ---------------------------------------------------------------------------------------------
-- Unexplored pieces
-- ---------------------------------------------------------------------------------------------

-- Draws a zone's pieces the way the game draws the explored ones: each piece is cut into 256-pixel tiles,
-- named after the piece with a number, the last row and column in smaller files
local function DrawUnexplored(mapFile)
    local used = 0
    if db.fog and OVERLAYS[mapFile] then
        local explored = {}
        for i = 1, GetNumMapOverlays() do
            local name = GetMapOverlayInfo(i)
            if name then explored[strupper(name)] = true end
        end
        for _, piece in ipairs(OVERLAYS[mapFile]) do
            local path = "Interface\\WorldMap\\" .. mapFile .. "\\" .. piece[1]
            if not explored[strupper(path)] then
                local width, height, offsetX, offsetY = piece[2], piece[3], piece[4], piece[5]
                local wide, tall = math.ceil(width / 256), math.ceil(height / 256)
                for row = 1, tall do
                    local pixelHeight, fileHeight = 256, 256
                    if row == tall then
                        pixelHeight = height % 256
                        if pixelHeight == 0 then pixelHeight = 256 end
                        fileHeight = 16
                        while fileHeight < pixelHeight do fileHeight = fileHeight * 2 end
                    end
                    for column = 1, wide do
                        local pixelWidth, fileWidth = 256, 256
                        if column == wide then
                            pixelWidth = width % 256
                            if pixelWidth == 0 then pixelWidth = 256 end
                            fileWidth = 16
                            while fileWidth < pixelWidth do fileWidth = fileWidth * 2 end
                        end
                        used = used + 1
                        local texture = pieces[used]
                        if not texture then
                            texture = WorldMapDetailFrame:CreateTexture(nil, "ARTWORK")
                            pieces[used] = texture
                        end
                        texture:SetWidth(pixelWidth)
                        texture:SetHeight(pixelHeight)
                        texture:SetTexCoord(0, pixelWidth / fileWidth, 0, pixelHeight / fileHeight)
                        texture:ClearAllPoints()
                        texture:SetPoint("TOPLEFT", WorldMapDetailFrame, "TOPLEFT", offsetX + 256 * (column - 1), -(offsetY + 256 * (row - 1)))
                        texture:SetTexture(path .. ((row - 1) * wide + column))
                        texture:SetVertexColor(FAINT[1], FAINT[2], FAINT[3], FAINT[4])
                        texture:Show()
                    end
                end
            end
        end
    end
    for i = used + 1, #pieces do pieces[i]:Hide() end
end

-- ---------------------------------------------------------------------------------------------
-- Pins
-- ---------------------------------------------------------------------------------------------

local function Wanted(kind)
    for _, k in ipairs(KINDS) do
        if k.kinds and k.kinds[kind] then return db[k.key] end
    end
end

-- Our quests at one giver that this character could still take
local function OpenQuests(list)
    local side = UnitFactionGroup("player") == "Horde" and "H" or "A"
    local _, class = UnitClass("player")
    local open = {}
    for _, q in ipairs(list) do
        if not done[q[1]] and (q[4] == "" or q[4] == side) and (q[5] == "" or q[5] == class) then tinsert(open, q) end
    end
    return open
end

local function PinTooltip(self)
    local p = self.pin
    WorldMapTooltip:SetOwner(self, "ANCHOR_RIGHT")
    WorldMapTooltip:AddLine(p[4], 1, 0.82, 0)
    if p[3] == "quest" then
        for _, q in ipairs(self.open or {}) do
            local colour = GetQuestDifficultyColor(q[3])
            WorldMapTooltip:AddLine("[" .. q[3] .. "] " .. q[2], colour.r, colour.g, colour.b)
        end
    elseif p[5] ~= "" then
        WorldMapTooltip:AddLine(p[5], 1, 1, 1, true)
    end
    WorldMapTooltip:AddLine(string.format("%.0f, %.0f", p[1], p[2]), 0.6, 0.6, 0.6)
    WorldMapTooltip:Show()
end

local function DrawPins(mapFile)
    local used = 0
    local width, height = WorldMapButton:GetWidth(), WorldMapButton:GetHeight()
    for _, p in ipairs(PINS[mapFile] or {}) do
        local kind = p[3]
        local open = kind == "quest" and OpenQuests(p[5]) or nil
        if Wanted(kind) and (not open or #open > 0) then
            used = used + 1
            local button = pins[used]
            if not button then
                button = CreateFrame("Button", nil, WorldMapButton)
                button.icon = button:CreateTexture(nil, "ARTWORK")
                button.icon:SetAllPoints()
                button:SetScript("OnEnter", PinTooltip)
                button:SetScript("OnLeave", function() WorldMapTooltip:Hide() end)
                pins[used] = button
            end
            local icon, size = nil, 16
            if kind == "dungeon" or kind == "raid" then
                icon, size = p[6] ~= "" and ("Interface\\LFGFrame\\LFGIcon-" .. p[6]) or "Interface\\Minimap\\Dungeon", 24
            else
                icon, size = ICONS[kind][1], ICONS[kind][2]
            end
            button.icon:SetTexture(icon)
            button:SetWidth(size)
            button:SetHeight(size)
            button:SetFrameLevel(WorldMapButton:GetFrameLevel() + 3)
            button:ClearAllPoints()
            button:SetPoint("CENTER", WorldMapButton, "TOPLEFT", p[1] / 100 * width, -p[2] / 100 * height)
            button.pin, button.open = p, open
            button:Show()
        end
    end
    for i = used + 1, #pins do pins[i]:Hide() end
end

-- ---------------------------------------------------------------------------------------------
-- The bar on the map: the boxes, and the coordinates
-- ---------------------------------------------------------------------------------------------

local bar = CreateFrame("Frame", nil, WorldMapButton)
bar:SetPoint("TOPLEFT", WorldMapButton, "TOPLEFT", 4, -4)
bar:SetPoint("TOPRIGHT", WorldMapButton, "TOPRIGHT", -4, -4)
bar:SetHeight(22)
bar:SetFrameLevel(WorldMapButton:GetFrameLevel() + 5)
local barBack = bar:CreateTexture(nil, "BACKGROUND")
barBack:SetAllPoints()
barBack:SetTexture(0, 0, 0, 0.55)

local Update
local boxes = {}
local x = 4
for _, k in ipairs(KINDS) do
    local box = CreateFrame("CheckButton", nil, bar, "UICheckButtonTemplate")
    box:SetWidth(20)
    box:SetHeight(20)
    box:SetPoint("LEFT", x, 0)
    local label = box:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    label:SetPoint("LEFT", box, "RIGHT", 0, 1)
    label:SetText(k.label)
    box:SetScript("OnClick", function(self)
        db[k.key] = self:GetChecked() and true or false
        Update()
    end)
    box.key = k.key
    boxes[#boxes + 1] = box
    x = x + 24 + label:GetStringWidth() + 12
end

local coordinates = bar:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
coordinates:SetPoint("RIGHT", -8, 0)
local sinceCoordinates = 0
bar:SetScript("OnUpdate", function(self, elapsed)
    sinceCoordinates = sinceCoordinates + elapsed
    if sinceCoordinates < 0.1 then return end
    sinceCoordinates = 0
    local text = ""
    local px, py = GetPlayerMapPosition("player")
    if px and (px > 0 or py > 0) then text = string.format("You  %.1f, %.1f", px * 100, py * 100) end
    local cx, cy = GetCursorPosition()
    local scale = WorldMapButton:GetEffectiveScale()
    local left, top, width, height = WorldMapButton:GetLeft(), WorldMapButton:GetTop(), WorldMapButton:GetWidth(), WorldMapButton:GetHeight()
    if left and width > 0 then
        cx, cy = (cx / scale - left) / width, (top - cy / scale) / height
        if cx >= 0 and cx <= 1 and cy >= 0 and cy <= 1 then
            text = text .. (text ~= "" and "     " or "") .. string.format("|cffffffffMouse  %.1f, %.1f|r", cx * 100, cy * 100)
        end
    end
    coordinates:SetText(text)
end)

function Update()
    if not db or not WorldMapFrame:IsShown() then return end
    for _, box in ipairs(boxes) do box:SetChecked(db[box.key]) end
    local mapFile = GetMapInfo()
    if not mapFile then return end
    DrawUnexplored(mapFile)
    DrawPins(mapFile)
end

-- ---------------------------------------------------------------------------------------------

local events = CreateFrame("Frame")
events:RegisterEvent("ADDON_LOADED")
events:RegisterEvent("PLAYER_LOGIN")
events:RegisterEvent("WORLD_MAP_UPDATE")
events:RegisterEvent("QUEST_QUERY_COMPLETE")
events:SetScript("OnEvent", function(self, event, arg)
    if event == "ADDON_LOADED" then
        if arg ~= "WowPlusMap" then return end
        WowPlusMapDB = WowPlusMapDB or {}
        db = WowPlusMapDB
        for _, k in ipairs(KINDS) do
            if db[k.key] == nil then db[k.key] = true end
        end
    elseif event == "PLAYER_LOGIN" then
        if QueryQuestsCompleted then QueryQuestsCompleted() end
    elseif event == "QUEST_QUERY_COMPLETE" then
        if GetQuestsCompleted then GetQuestsCompleted(done) end
        Update()
    else
        Update()
    end
end)
WorldMapFrame:HookScript("OnShow", function()
    if QueryQuestsCompleted then QueryQuestsCompleted() end     -- which of our quests are done; the answer redraws the pins
    Update()
end)

SLASH_WOWPLUSMAP1 = "/wpmap"
SlashCmdList["WOWPLUSMAP"] = function(arg)
    arg = strlower(strtrim(arg or ""))
    for _, k in ipairs(KINDS) do
        if arg == k.key or arg == strlower(k.label) then
            db[k.key] = not db[k.key]
            Update()
        end
    end
    local shown = {}
    for _, k in ipairs(KINDS) do tinsert(shown, k.label .. (db[k.key] and " on" or " off")) end
    DEFAULT_CHAT_FRAME:AddMessage("Map: " .. table.concat(shown, ", ") .. ". /wpmap fog, dungeon, champion, travel or quest turns one on or off; so do the boxes at the top of the map.")
end
