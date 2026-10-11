-- WowPlus Journal: shows the journal the server keeps for your character.
-- The server writes the lines (Server/modules/mod-wowplus/src/WowPlusJournal.cpp); this addon
-- asks for a book, shows it newest first, and flashes a new line on screen when one is written.
-- Open it with /journal or the small book by the minimap. Scroll with the mouse wheel.

local PREFIX = "WPJ"
local BOOKS = { { "world", "The World" }, { "people", "People" } }

local lines = { world = {}, people = {} }       -- newest first: { time, text }
local incoming = {}
local current = "world"

local window = CreateFrame("Frame", "WowPlusJournalWindow", UIParent)
window:SetWidth(420)
window:SetHeight(460)
window:SetPoint("CENTER", 0, 40)
window:SetBackdrop({ bgFile = "Interface\\DialogFrame\\UI-DialogBox-Background", edgeFile = "Interface\\DialogFrame\\UI-DialogBox-Border",
                     tile = true, tileSize = 32, edgeSize = 32, insets = { left = 11, right = 12, top = 12, bottom = 11 } })
window:SetMovable(true)
window:EnableMouse(true)
window:RegisterForDrag("LeftButton")
window:SetScript("OnDragStart", window.StartMoving)
window:SetScript("OnDragStop", window.StopMovingOrSizing)
window:SetFrameStrata("HIGH")
window:Hide()
tinsert(UISpecialFrames, "WowPlusJournalWindow")     -- Escape closes it

local title = window:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
title:SetPoint("TOP", 0, -18)
title:SetText("Journal")

local close = CreateFrame("Button", nil, window, "UIPanelCloseButton")
close:SetPoint("TOPRIGHT", -6, -6)

local page = CreateFrame("ScrollingMessageFrame", nil, window)
page:SetPoint("TOPLEFT", 24, -78)
page:SetPoint("BOTTOMRIGHT", -24, 24)
page:SetFontObject(GameFontHighlight)
page:SetJustifyH("LEFT")
page:SetFading(false)
page:SetMaxLines(400)
page:SetInsertMode("BOTTOM")
page:EnableMouseWheel(true)
page:SetScript("OnMouseWheel", function(self, delta)
    if delta > 0 then self:ScrollUp() self:ScrollUp() else self:ScrollDown() self:ScrollDown() end
end)

local function Show()
    page:Clear()
    local shown = lines[current]
    if #shown == 0 then
        page:AddMessage("Nothing written here yet.", 0.6, 0.6, 0.6)
    end
    -- The frame shows its last line at the bottom, so the oldest goes in first
    for i = #shown, 1, -1 do
        page:AddMessage("|cffc0a060" .. date("%d %b %H:%M", shown[i][1]) .. "|r  " .. shown[i][2])
    end
    page:ScrollToBottom()
end

local tabs = {}
local function Ask(book)
    incoming[book] = {}
    SendAddonMessage(PREFIX, "L:" .. book, "WHISPER", UnitName("player"))
end

for index, book in ipairs(BOOKS) do
    local tab = CreateFrame("Button", "WowPlusJournalTab" .. index, window, "UIPanelButtonTemplate")
    tab:SetWidth(120)
    tab:SetHeight(22)
    tab:SetPoint("TOPLEFT", 24 + (index - 1) * 126, -46)
    tab:SetText(book[2])
    tab:SetScript("OnClick", function()
        current = book[1]
        for _, other in ipairs(tabs) do other:UnlockHighlight() end
        tab:LockHighlight()
        Show()
        Ask(current)
    end)
    tabs[index] = tab
end

local function Toggle()
    if window:IsShown() then
        window:Hide()
    else
        window:Show()
        tabs[current == "people" and 2 or 1]:Click()
    end
end

-- A small book by the minimap
local button = CreateFrame("Button", "WowPlusJournalButton", Minimap)
button:SetWidth(28)
button:SetHeight(28)
button:SetFrameStrata("MEDIUM")
button:SetPoint("TOPLEFT", Minimap, "TOPLEFT", -18, -52)
button:SetNormalTexture("Interface\\Icons\\INV_Misc_Book_09")
button:SetHighlightTexture("Interface\\Minimap\\UI-Minimap-ZoomButton-Highlight")
button:SetScript("OnClick", Toggle)
button:SetScript("OnEnter", function(self)
    GameTooltip:SetOwner(self, "ANCHOR_LEFT")
    GameTooltip:SetText("Journal", 1, 1, 1)
    GameTooltip:AddLine("What you killed and found, and who you met.", nil, nil, nil, true)
    GameTooltip:Show()
end)
button:SetScript("OnLeave", function() GameTooltip:Hide() end)

local listener = CreateFrame("Frame")
listener:RegisterEvent("CHAT_MSG_ADDON")
listener:SetScript("OnEvent", function(self, event, prefix, message, channel, sender)
    if prefix ~= PREFIX or sender ~= UnitName("player") then return end

    local kind, book, rest = string.match(message, "^(%a):(%a+):?(.*)$")
    if not kind or not lines[book] then return end

    if kind == "E" or kind == "N" then
        local at, text = string.match(rest, "^(%d+):(.*)$")
        if not at then return end
        if kind == "E" then
            if incoming[book] then tinsert(incoming[book], { tonumber(at), text }) end
        else
            -- Written just now: show it on screen for a moment, and in the book if it is open
            tinsert(lines[book], 1, { tonumber(at), text })
            UIErrorsFrame:AddMessage("Journal: " .. text, 0.95, 0.85, 0.5, 1, 5)
            if window:IsShown() and current == book then Show() end
        end
    elseif kind == "D" and incoming[book] then
        lines[book] = incoming[book]
        incoming[book] = nil
        if window:IsShown() and current == book then Show() end
    end
end)

SLASH_WOWPLUSJOURNAL1 = "/journal"
SlashCmdList["WOWPLUSJOURNAL"] = Toggle