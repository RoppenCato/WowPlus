-- WowPlus Clock: the clock on the minimap shows the world's time, not your PC's.
-- On this server a full day and night pass in an hour (WowPlus.World.DayMinutes), and the sky
-- follows. The game's own minimap clock can show either the game's time or the PC's; this
-- makes sure it is there and set to the game's time. Hover over it to see both.

local frame = CreateFrame("Frame")
frame:RegisterEvent("PLAYER_LOGIN")
frame:SetScript("OnEvent", function()
    if not IsAddOnLoaded("Blizzard_TimeManager") then LoadAddOn("Blizzard_TimeManager") end
    SetCVar("timeMgrUseLocalTime", "0")
    if TimeManagerClockButton then
        TimeManagerClockButton:Show()
        if TimeManagerClockButton_Update then TimeManagerClockButton_Update() end
    end
end)