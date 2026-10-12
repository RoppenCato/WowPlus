-- WowPlus Group: a board of people looking for a group.
--
-- The list comes from the server (mod-wowplus, WowPlusBoard.cpp): bots that are really in the world,
-- near your level and free. Invite someone and they join, and a minute or so later they stand at the
-- entrance of the place you chose under "Going to", or walk up to you when you chose none. Or tick what
-- you need and press "Post my group": people who want to come whisper you and show up at the top of the
-- list, to accept or turn down.
--
-- /board opens and closes the window; so does "Group" on the bot bar. See Settings/Docs/Group-Board.md.

local PREFIX = "WPG"
local ROWS = 18
local ROW_HEIGHT = 18
local LOOK_EVERY = 20           -- seconds between looks at the board while the window is open

local CLASS_TOKENS = { [1] = "WARRIOR", [2] = "PALADIN", [3] = "HUNTER", [4] = "ROGUE", [5] = "PRIEST", [6] = "DEATHKNIGHT",
                       [7] = "SHAMAN", [8] = "MAGE", [9] = "WARLOCK", [11] = "DRUID" }
local ROLE_LABELS = { tank = "|cff6fa8ffTank|r", healer = "|cff7fe07fHealer|r", damage = "|cffe08a6fDamage|r" }

local people, incoming = {}, nil    -- the list shown, and the one being received
local dungeons = {}                 -- { id, name, raid }
local state = { destination = 0, posted = false, limit = 5, size = 1 }

local function Ask(text)
    SendAddonMessage(PREFIX, text, "WHISPER", UnitName("player"))
end

-- ---------------------------------------------------------------------------------------------
-- The window
-- ---------------------------------------------------------------------------------------------

local frame = CreateFrame("Frame", "WowPlusGroupFrame", UIParent)
frame:SetWidth(520)
frame:SetHeight(ROWS * ROW_HEIGHT + 132)
frame:SetPoint("CENTER", 0, 40)
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
tinsert(UISpecialFrames, "WowPlusGroupFrame")       -- Escape closes it

local title = frame:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
title:SetPoint("TOP", 0, -14)
title:SetText("Looking for Group")

local close = CreateFrame("Button", nil, frame, "UIPanelCloseButton")
close:SetPoint("TOPRIGHT", -4, -4)

local goingLabel = frame:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
goingLabel:SetPoint("TOPLEFT", 18, -44)
goingLabel:SetText("Going to:")

local going = CreateFrame("Frame", "WowPlusGroupGoing", frame, "UIDropDownMenuTemplate")
going:SetPoint("TOPLEFT", 62, -34)
UIDropDownMenu_SetWidth(going, 230)

local groupSize = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
groupSize:SetPoint("TOPRIGHT", -22, -44)

local function DungeonName(id)
    for _, dungeon in ipairs(dungeons) do
        if dungeon.id == id then return dungeon.name end
    end
    return "Nowhere in particular"
end

local function ChooseDestination(self)
    state.destination = self.value
    UIDropDownMenu_SetText(going, DungeonName(self.value))
    Ask("D\t" .. self.value)
end

UIDropDownMenu_Initialize(going, function()
    local info = UIDropDownMenu_CreateInfo()
    info.text, info.value, info.func, info.checked = "Nowhere in particular", 0, ChooseDestination, state.destination == 0
    UIDropDownMenu_AddButton(info)
    for _, dungeon in ipairs(dungeons) do
        info = UIDropDownMenu_CreateInfo()
        info.text = dungeon.name .. (dungeon.raid and "  |cffffd100(raid)|r" or "")
        info.value, info.func, info.checked = dungeon.id, ChooseDestination, state.destination == dungeon.id
        UIDropDownMenu_AddButton(info)
    end
end)
UIDropDownMenu_SetText(going, "Nowhere in particular")

local rows = {}
for index = 1, ROWS do
    local row = CreateFrame("Frame", nil, frame)
    row:SetHeight(ROW_HEIGHT)
    row:SetPoint("TOPLEFT", 16, -66 - (index - 1) * ROW_HEIGHT)
    row:SetPoint("RIGHT", -16, 0)

    row.shade = row:CreateTexture(nil, "BACKGROUND")
    row.shade:SetAllPoints()
    row.shade:SetTexture(1, 0.82, 0, 0.12)

    row.role = row:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    row.role:SetPoint("LEFT", 2, 0)
    row.role:SetWidth(46)
    row.role:SetJustifyH("LEFT")

    row.name = row:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
    row.name:SetPoint("LEFT", 50, 0)
    row.name:SetWidth(118)
    row.name:SetJustifyH("LEFT")

    row.note = row:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    row.note:SetPoint("LEFT", 170, 0)
    row.note:SetWidth(232)
    row.note:SetJustifyH("LEFT")
    row.note:SetHeight(ROW_HEIGHT)

    row.invite = CreateFrame("Button", nil, row, "UIPanelButtonTemplate")
    row.invite:SetWidth(56)
    row.invite:SetHeight(ROW_HEIGHT - 1)
    row.invite:SetPoint("RIGHT", -20, 0)
    row.invite:GetFontString():SetFont(GameFontNormalSmall:GetFont())
    row.invite:SetScript("OnClick", function() if row.person then Ask("I\t" .. row.person.name) end end)

    row.decline = CreateFrame("Button", nil, row, "UIPanelButtonTemplate")
    row.decline:SetWidth(18)
    row.decline:SetHeight(ROW_HEIGHT - 1)
    row.decline:SetPoint("RIGHT", 0, 0)
    row.decline:SetText("x")
    row.decline:GetFontString():SetFont(GameFontNormalSmall:GetFont())
    row.decline:SetScript("OnClick", function() if row.person then Ask("X\t" .. row.person.name) end end)

    row:EnableMouse(true)
    row:SetScript("OnEnter", function(self)
        local person = self.person
        if not person then return end
        GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
        GameTooltip:SetText(person.name, 1, 1, 1)
        GameTooltip:AddLine("Level " .. person.level .. " " .. (LOCALIZED_CLASS_NAMES_MALE[CLASS_TOKENS[person.class] or ""] or ""), 1, 0.82, 0)
        if person.guild ~= "" then GameTooltip:AddLine("<" .. person.guild .. ">", 0.6, 0.8, 1) end
        if person.known then GameTooltip:AddLine("Someone you know", 0.5, 1, 0.5) end
        if person.applicant then GameTooltip:AddLine("Asked to join your group", 1, 0.82, 0) end
        GameTooltip:AddLine(person.note, 1, 1, 1, true)
        GameTooltip:Show()
    end)
    row:SetScript("OnLeave", function() GameTooltip:Hide() end)
    rows[index] = row
end

local message = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
message:SetPoint("BOTTOMLEFT", 18, 44)
message:SetPoint("RIGHT", -18, 0)
message:SetJustifyH("LEFT")
message:SetTextColor(1, 0.82, 0)

local needs = {}
local function MakeCheck(index, label, x)
    local check = CreateFrame("CheckButton", "WowPlusGroupNeed" .. index, frame, "UICheckButtonTemplate")
    check:SetWidth(22)
    check:SetHeight(22)
    check:SetPoint("BOTTOMLEFT", x, 14)
    _G[check:GetName() .. "Text"]:SetText(label)
    needs[index] = check
end
local needLabel = frame:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
needLabel:SetPoint("BOTTOMLEFT", 18, 20)
needLabel:SetText("I need:")
MakeCheck(1, "Tank", 62)
MakeCheck(2, "Healer", 122)
MakeCheck(3, "Damage", 192)

local post = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
post:SetWidth(120)
post:SetHeight(22)
post:SetPoint("BOTTOMRIGHT", -18, 14)
post:SetScript("OnClick", function()
    if state.posted then
        Ask("U")
    else
        Ask("P\t" .. (needs[1]:GetChecked() and 1 or 0) .. "\t" .. (needs[2]:GetChecked() and 1 or 0) .. "\t" .. (needs[3]:GetChecked() and 1 or 0))
    end
end)

local refresh = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
refresh:SetWidth(70)
refresh:SetHeight(22)
refresh:SetPoint("RIGHT", post, "LEFT", -6, 0)
refresh:SetText("Look again")
refresh:GetFontString():SetFont(GameFontNormalSmall:GetFont())
refresh:SetScript("OnClick", function() Ask("L") end)

-- ---------------------------------------------------------------------------------------------
-- Showing what the server sent
-- ---------------------------------------------------------------------------------------------

local function Show()
    -- People who asked to join come first, then people you know, then the rest as the server listed them
    table.sort(people, function(a, b)
        if a.applicant ~= b.applicant then return a.applicant end
        if a.known ~= b.known then return a.known end
        return a.order < b.order
    end)

    for index, row in ipairs(rows) do
        local person = people[index]
        row.person = person
        if person then
            local color = RAID_CLASS_COLORS[CLASS_TOKENS[person.class] or ""] or { r = 1, g = 1, b = 1 }
            row.role:SetText(ROLE_LABELS[person.role] or person.role)
            row.name:SetText(person.name .. " |cffaaaaaa" .. person.level .. "|r")
            row.name:SetTextColor(color.r, color.g, color.b)
            row.note:SetText((person.known and "|cff80ff80*|r " or "") .. person.note)
            row.invite:SetText(person.applicant and "Accept" or "Invite")
            if person.applicant then row.shade:Show() row.decline:Show() else row.shade:Hide() row.decline:Hide() end
            row:Show()
        else
            row:Hide()
        end
    end

    post:SetText(state.posted and "Take post down" or "Post my group")
    groupSize:SetText("Your group: " .. state.size .. " of " .. state.limit)
    UIDropDownMenu_SetText(going, DungeonName(state.destination))
end

local listener = CreateFrame("Frame")
listener:RegisterEvent("CHAT_MSG_ADDON")
listener:SetScript("OnEvent", function(self, event, prefix, text, channel, sender)
    if prefix ~= PREFIX or sender ~= UnitName("player") then return end

    local what, a, b, c, d, e, f, g, h = strsplit("\t", text)
    if what == "C" then
        incoming = {}
    elseif what == "E" and incoming then
        tinsert(incoming, { name = a, class = tonumber(b), role = c, level = tonumber(d) or 0, guild = e or "", known = f == "1",
                            applicant = g == "1", note = h or "", order = #incoming })
    elseif what == "Z" and incoming then
        people, incoming = incoming, nil
        Show()
    elseif what == "N" then                 -- a note has been written
        for _, person in ipairs(people) do
            if person.name == a then person.note = b or person.note end
        end
        Show()
    elseif what == "G" then
        local known = false
        for _, dungeon in ipairs(dungeons) do
            if dungeon.id == tonumber(a) then known = true end
        end
        if not known then tinsert(dungeons, { id = tonumber(a), name = b, raid = c == "1" }) end
    elseif what == "S" then
        state.destination, state.posted, state.limit, state.size = tonumber(a) or 0, b == "1", tonumber(c) or 5, tonumber(d) or 1
        Show()
    elseif what == "M" then
        message:SetText(a or "")
        if a and a ~= "" then DEFAULT_CHAT_FRAME:AddMessage("|cffffd100Group board:|r " .. a) end
    end
end)

local sinceLook = 0
frame:SetScript("OnUpdate", function(self, elapsed)
    sinceLook = sinceLook + elapsed
    if sinceLook >= LOOK_EVERY then
        sinceLook = 0
        Ask("L")
    end
end)
frame:SetScript("OnShow", function()
    sinceLook = 0
    message:SetText("")
    Ask("L")
end)

function WowPlusGroup_Toggle()
    if frame:IsShown() then frame:Hide() else frame:Show() end
end

SLASH_WOWPLUSGROUP1 = "/board"
SLASH_WOWPLUSGROUP2 = "/groupboard"
SlashCmdList["WOWPLUSGROUP"] = WowPlusGroup_Toggle
