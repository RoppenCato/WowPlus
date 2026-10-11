-- WowPlus Talents: a "Reset talents" button in the talent window.
-- The game itself only lets a class trainer reset talents. The button asks the server
-- (Server/modules/mod-wowplus/src/WowPlusClasses.cpp), which hands back the talents of the set
-- you are using, for free. It asks "are you sure" first.

StaticPopupDialogs["WOWPLUS_RESET_TALENTS"] = {
    text = "Reset all talents of the set you are using? It is free.",
    button1 = YES,
    button2 = NO,
    OnAccept = function() SendAddonMessage("WPT", "reset", "WHISPER", UnitName("player")) end,
    timeout = 0,
    whileDead = false,
    hideOnEscape = true,
}

local function AddButton()
    if WowPlusTalentsReset or not PlayerTalentFrame then return end
    local button = CreateFrame("Button", "WowPlusTalentsReset", PlayerTalentFrame, "UIPanelButtonTemplate")
    button:SetWidth(100)
    button:SetHeight(22)
    -- Below the window, in line with the Arcane / Fire / Frost / Glyphs tabs and to the right of them: the
    -- only place nothing else of the talent window uses
    if PlayerTalentFrameTab4 then
        button:SetPoint("LEFT", PlayerTalentFrameTab4, "RIGHT", 8, 2)
    else
        button:SetPoint("BOTTOMRIGHT", PlayerTalentFrame, "BOTTOMRIGHT", -44, 52)
    end
    button:SetText("Reset talents")
    button:SetScript("OnClick", function() StaticPopup_Show("WOWPLUS_RESET_TALENTS") end)
end

-- The talent window is loaded by the game the first time it is opened
local loader = CreateFrame("Frame")
loader:RegisterEvent("ADDON_LOADED")
loader:SetScript("OnEvent", function(self, event, name)
    if name == "Blizzard_TalentUI" then AddButton() end
end)
if IsAddOnLoaded("Blizzard_TalentUI") then AddButton() end