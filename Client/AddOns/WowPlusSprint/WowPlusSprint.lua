-- WowPlus Sprint: tells the server when Shift is held, and shows the stamina bar.
-- The server decides everything (speed, stamina, when sprint is allowed); see
-- Server/modules/mod-wowplus/src/WowPlusSprint.cpp for the messages.
--
-- The bar sits just below your character, where the eye already is while running. It only
-- shows while stamina is not full. Hold Alt and drag it to move it; /sprintbar reset puts it back.

local PREFIX = "WPS"
local FULL = 1000
local DEFAULT_X, DEFAULT_Y = 0, -110        -- from the middle of the screen

local value, rate, stamp = FULL, 0, 0       -- last word from the server
local exhausted = false
local held = false
local serverOff = false
local sinceCheck, sinceHello, answered = 0, 0, false

local bar = CreateFrame("StatusBar", "WowPlusSprintBar", UIParent)
bar:SetWidth(150)
bar:SetHeight(7)
bar:SetStatusBarTexture("Interface\\TargetingFrame\\UI-StatusBar")
bar:SetMinMaxValues(0, FULL)
bar:SetValue(FULL)
bar:SetAlpha(0)
bar:SetMovable(true)
bar:SetClampedToScreen(true)

local back = bar:CreateTexture(nil, "BACKGROUND")
back:SetPoint("TOPLEFT", -1, 1)
back:SetPoint("BOTTOMRIGHT", 1, -1)
back:SetTexture(0, 0, 0, 0.6)

local function Place()
    bar:ClearAllPoints()
    bar:SetPoint("CENTER", UIParent, "CENTER", WowPlusSprintDB.x or DEFAULT_X, WowPlusSprintDB.y or DEFAULT_Y)
end

bar:EnableMouse(true)
bar:SetScript("OnMouseDown", function(self)
    if IsAltKeyDown() then self:StartMoving() end
end)
bar:SetScript("OnMouseUp", function(self)
    self:StopMovingOrSizing()
    local x, y = self:GetCenter()
    local px, py = UIParent:GetCenter()
    WowPlusSprintDB.x, WowPlusSprintDB.y = x - px, y - py
    Place()
end)

local function Send(text)
    SendAddonMessage(PREFIX, text, "WHISPER", UnitName("player"))
end

local function Current()
    local now = value + rate * (GetTime() - stamp)
    if now < 0 then return 0 end
    if now > FULL then return FULL end
    return now
end

local driver = CreateFrame("Frame")
driver:RegisterEvent("PLAYER_LOGIN")
driver:RegisterEvent("PLAYER_ENTERING_WORLD")
driver:RegisterEvent("CHAT_MSG_ADDON")

driver:SetScript("OnEvent", function(self, event, prefix, message, channel, sender)
    if event == "PLAYER_LOGIN" then
        WowPlusSprintDB = WowPlusSprintDB or {}
        Place()
    elseif event == "PLAYER_ENTERING_WORLD" then
        held, answered, sinceHello = false, false, 0
        Send("H")
    elseif prefix == PREFIX and sender == UnitName("player") then
        answered = true
        if message == "X" then
            serverOff = true
            bar:Hide()
            return
        end
        local v, r, e = string.match(message, "^S:(%-?%d+):(%-?%d+):(%d)$")
        if v then
            serverOff = false
            bar:Show()
            value, rate, stamp, exhausted = tonumber(v), tonumber(r), GetTime(), e == "1"
        end
    end
end)

driver:SetScript("OnUpdate", function(self, elapsed)
    if serverOff then return end

    -- Shift is polled, not taken from an event: an event is missed when the game loses focus
    sinceCheck = sinceCheck + elapsed
    if sinceCheck >= 0.05 then
        sinceCheck = 0
        local typing = GetCurrentKeyBoardFocus and GetCurrentKeyBoardFocus()
        local want = IsShiftKeyDown() and not typing and true or false
        if want ~= held then
            held = want
            Send(want and "1" or "0")
        end
    end

    -- Say hello again if the server did not answer (the first try can come too early)
    if not answered then
        sinceHello = sinceHello + elapsed
        if sinceHello >= 3 then
            sinceHello = 0
            Send("H")
        end
    end

    local now = Current()
    bar:SetValue(now)
    if exhausted then
        bar:SetStatusBarColor(0.8, 0.25, 0.2)
    else
        bar:SetStatusBarColor(0.95, 0.8, 0.25)
    end

    -- Shown while in use or refilling, fading out once full
    local alpha = bar:GetAlpha()
    if now < FULL then
        alpha = math.min(1, alpha + elapsed * 6)
    else
        alpha = math.max(0, alpha - elapsed * 2)
    end
    bar:SetAlpha(alpha)
end)

SLASH_WOWPLUSSPRINT1 = "/sprintbar"
SlashCmdList["WOWPLUSSPRINT"] = function(text)
    if text == "reset" then
        WowPlusSprintDB.x, WowPlusSprintDB.y = nil, nil
        Place()
        DEFAULT_CHAT_FRAME:AddMessage("Sprint bar: back in its usual place.")
    else
        DEFAULT_CHAT_FRAME:AddMessage("Sprint bar: hold Alt and drag the bar to move it (it shows while stamina is not full). /sprintbar reset puts it back.")
    end
end
