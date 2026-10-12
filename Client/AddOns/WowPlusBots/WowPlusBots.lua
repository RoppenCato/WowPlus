-- WowPlus Bots: a small bar of buttons, so that the bots need no typed commands.
--
-- The buttons only send what you could type yourself: a word in party chat for the bots in your
-- party ("follow"), or one of the server's ".bots" commands. See Settings/Docs/Bots.md.
-- Drag the bar by its title to move it. /botbar shows or hides it, /botbar reset puts it back.

local BUTTONS = {
    -- label, what it sends, where ("PARTY": party chat, needs a party. "COMMAND": a server command), tooltip
    { "Follow",  "follow",        "PARTY",   "The bots in your party follow you." },
    { "Stay",    "stay",          "PARTY",   "The bots in your party stay where they are." },
    { "Attack",  "attack",        "PARTY",   "The bots in your party attack your target." },
    { "Flee",    "flee",          "PARTY",   "The bots in your party stop fighting and run to you." },
    { "Call",    ".bots call",    "COMMAND", "Your companions join your party and come to you." },
    { "Dismiss", ".bots dismiss", "COMMAND", "Your companions leave the party and go their own way." },
    { "Away",    ".bots away",    "COMMAND", "Every free bot near you goes somewhere else, once." },
    { "Group",   "",              "BOARD",   "The board of people looking for a group: pick who comes along, or post your own group." },
}

local WIDTH, HEIGHT, GAP = 52, 20, 2

local bar = CreateFrame("Frame", "WowPlusBotsBar", UIParent)
bar:SetWidth(#BUTTONS * (WIDTH + GAP) + 54 + GAP + 8)
bar:SetHeight(HEIGHT + 22)
bar:SetBackdrop({ bgFile = "Interface\\Tooltips\\UI-Tooltip-Background", edgeFile = "Interface\\Tooltips\\UI-Tooltip-Border",
                  edgeSize = 12, insets = { left = 3, right = 3, top = 3, bottom = 3 } })
bar:SetBackdropColor(0, 0, 0, 0.6)
bar:SetMovable(true)
bar:SetClampedToScreen(true)
bar:EnableMouse(true)
bar:RegisterForDrag("LeftButton")

local title = bar:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
title:SetPoint("TOPLEFT", 8, -5)
title:SetText("Bots")

local function Place()
    bar:ClearAllPoints()
    bar:SetPoint("CENTER", UIParent, "CENTER", WowPlusBotsDB.x or 0, WowPlusBotsDB.y or -230)
    if WowPlusBotsDB.hidden then bar:Hide() else bar:Show() end
end

bar:SetScript("OnDragStart", bar.StartMoving)
bar:SetScript("OnDragStop", function(self)
    self:StopMovingOrSizing()
    local x, y = self:GetCenter()
    local px, py = UIParent:GetCenter()
    WowPlusBotsDB.x, WowPlusBotsDB.y = x - px, y - py
    Place()
end)

local function Send(text, where)
    if where == "BOARD" then
        if WowPlusGroup_Toggle then
            WowPlusGroup_Toggle()
        else
            DEFAULT_CHAT_FRAME:AddMessage("Bots: the addon WowPlus Group is not loaded. Restart the game once to pick it up.")
        end
    elseif where == "PARTY" then
        if GetNumPartyMembers() == 0 and GetNumRaidMembers() == 0 then
            DEFAULT_CHAT_FRAME:AddMessage("Bots: you are not in a party. Invite a bot, or press Call for your companions.")
            return
        end
        SendChatMessage(text, GetNumRaidMembers() > 0 and "RAID" or "PARTY")
    else
        SendChatMessage(text, "SAY")        -- the server takes a line that starts with a dot as a command
    end
end

-- Marks, in the title row. The bots' damage dealers and tanks go for the skull first; mages, hunters and
-- druids keep the moon out of the fight (sheep, trap, sleep). Click again on the same target to take it off.
local function MakeMark(icon, offset, tooltip)
    local button = CreateFrame("Button", nil, bar)
    button:SetWidth(16)
    button:SetHeight(16)
    button:SetPoint("TOPRIGHT", offset, -3)
    button:SetNormalTexture("Interface\\TargetingFrame\\UI-RaidTargetingIcon_" .. icon)
    button:SetHighlightTexture("Interface\\Buttons\\UI-Common-MouseHilight", "ADD")
    button:SetScript("OnClick", function()
        if not UnitExists("target") then
            DEFAULT_CHAT_FRAME:AddMessage("Bots: target an enemy first.")
        else
            SetRaidTarget("target", GetRaidTargetIndex("target") == icon and 0 or icon)
        end
    end)
    button:SetScript("OnEnter", function(self)
        GameTooltip:SetOwner(self, "ANCHOR_TOP")
        GameTooltip:SetText(tooltip, 1, 1, 1, 1, true)
        GameTooltip:Show()
    end)
    button:SetScript("OnLeave", function() GameTooltip:Hide() end)
end
MakeMark(8, -8, "Skull on your target: the bots kill this one first.")
MakeMark(5, -28, "Moon on your target: a mage, hunter or druid in the group keeps it out of the fight, and the others leave it alone.")

local ready = CreateFrame("Button", nil, bar, "UIPanelButtonTemplate")
ready:SetWidth(50)
ready:SetHeight(16)
ready:SetPoint("TOPRIGHT", -50, -3)
ready:SetText("Ready?")
ready:GetFontString():SetFont(GameFontNormalSmall:GetFont())
ready:SetScript("OnClick", function()
    if GetNumPartyMembers() == 0 and GetNumRaidMembers() == 0 then
        DEFAULT_CHAT_FRAME:AddMessage("Bots: you are not in a group.")
    else
        DoReadyCheck()
    end
end)
ready:SetScript("OnEnter", function(self)
    GameTooltip:SetOwner(self, "ANCHOR_TOP")
    GameTooltip:SetText("A ready check. A bot that is low on health or mana, dead or not here says no, and sits down to eat or drink.", 1, 1, 1, 1, true)
    GameTooltip:Show()
end)
ready:SetScript("OnLeave", function() GameTooltip:Hide() end)

local function MakeButton(index, label, tooltip, onClick)
    local button = CreateFrame("Button", "WowPlusBotsButton" .. index, bar, "UIPanelButtonTemplate")
    button:SetWidth(index > #BUTTONS and 54 or WIDTH)
    button:SetHeight(HEIGHT)
    button:SetPoint("BOTTOMLEFT", 4 + (index - 1) * (WIDTH + GAP), 4)
    button:SetText(label)
    button:GetFontString():SetFont(GameFontNormalSmall:GetFont())
    button:SetScript("OnClick", onClick)
    button:SetScript("OnEnter", function(self)
        GameTooltip:SetOwner(self, "ANCHOR_TOP")
        GameTooltip:SetText(tooltip, 1, 1, 1, 1, true)
        GameTooltip:Show()
    end)
    button:SetScript("OnLeave", function() GameTooltip:Hide() end)
    return button
end

for index, info in ipairs(BUTTONS) do
    MakeButton(index, info[1], info[4], function() Send(info[2], info[3]) end)
end

-- "Alone" is a switch: the server remembers it, the button shows what was last asked for
local alone = MakeButton(#BUTTONS + 1, "Alone", "Keeps free bots away from where you are until you press it again. Your party stays.", function(self)
    WowPlusBotsDB.alone = not WowPlusBotsDB.alone
    Send(".bots alone " .. (WowPlusBotsDB.alone and "on" or "off"), "COMMAND")
    self:SetText(WowPlusBotsDB.alone and "Alone: on" or "Alone")
end)

local loader = CreateFrame("Frame")
loader:RegisterEvent("PLAYER_LOGIN")
loader:SetScript("OnEvent", function()
    WowPlusBotsDB = WowPlusBotsDB or {}
    alone:SetText(WowPlusBotsDB.alone and "Alone: on" or "Alone")
    Place()
end)

SLASH_WOWPLUSBOTS1 = "/botbar"
SlashCmdList["WOWPLUSBOTS"] = function(text)
    if text == "reset" then
        WowPlusBotsDB.x, WowPlusBotsDB.y, WowPlusBotsDB.hidden = nil, nil, nil
    else
        WowPlusBotsDB.hidden = not WowPlusBotsDB.hidden
    end
    Place()
end