-- WowPlus Coach: the journey. When you take a coach from an innkeeper, the server sends a line of text;
-- the screen fades to black with that line, stays black through the loading screen, and fades back in
-- when you have arrived. (Server side: Server/modules/mod-wowplus/src/WowPlusCoach.cpp.)

local FADE_IN, FADE_OUT, GIVE_UP = 1.2, 1.5, 20

local black = CreateFrame("Frame", "WowPlusCoachFade", UIParent)
black:SetAllPoints(UIParent)
black:SetFrameStrata("FULLSCREEN_DIALOG")
black:Hide()

local dark = black:CreateTexture(nil, "BACKGROUND")
dark:SetAllPoints(black)
dark:SetTexture(0, 0, 0, 1)

local line = black:CreateFontString(nil, "OVERLAY")
line:SetFont(STANDARD_TEXT_FONT, 22, "OUTLINE")
line:SetPoint("CENTER", 0, 0)
line:SetTextColor(0.95, 0.85, 0.6)

local state, timer = "idle", 0

black:SetScript("OnUpdate", function(self, elapsed)
    timer = timer + elapsed
    if state == "leaving" then
        self:SetAlpha(math.min(1, timer / FADE_IN))
        if timer > GIVE_UP then state, timer = "arriving", 0 end       -- the journey never came: let the player see again
    elseif state == "arriving" then
        local alpha = 1 - timer / FADE_OUT
        if alpha <= 0 then
            state = "idle"
            self:Hide()
        else
            self:SetAlpha(alpha)
        end
    end
end)

local listener = CreateFrame("Frame")
listener:RegisterEvent("CHAT_MSG_ADDON")
listener:RegisterEvent("PLAYER_ENTERING_WORLD")
listener:SetScript("OnEvent", function(self, event, prefix, message, channel, sender)
    if event == "CHAT_MSG_ADDON" then
        if prefix == "WPC" and sender == UnitName("player") then
            line:SetText(message)
            state, timer = "leaving", 0
            black:SetAlpha(0)
            black:Show()
        end
    elseif state == "leaving" then
        state, timer = "arriving", 0                                   -- the loading screen is over: we are there
    end
end)