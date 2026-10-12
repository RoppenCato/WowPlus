-- WowPlus Flair: the game tells you when something good happens.
--
--   Loot      When you loot something blue or better, its name and icon appear in the middle of
--             the screen in the item's colour, a beam of that colour rises behind it, and the edges of
--             the screen flash. The better the item, the longer it pulses.
--   Record    Your hardest hit is remembered per character. Beat it and the number pops up once in gold,
--             with a sound and a line in chat. No flash on the screen.
--   Big hits  Off unless you switch it on (/flair hits): critical hits and hits close to your best shown in
--             large numbers of their own. The game shows its own number as well, so every such hit is
--             shown twice; that is why it is off.
--
-- Everything is done by this addon alone; the server is not involved.
-- /flair shows your record and the switches: /flair loot, /flair hits, /flair record, /flair reset

local LOOT_FROM_QUALITY = 3             -- 2 green, 3 blue, 4 epic: the lowest quality that is announced (Robin: not for greens)
local BIG_HIT_SHARE = 0.75              -- a hit this close to the record counts as big even if it is no crit
local RECORD_PAUSE = 20                 -- seconds between two record fanfares, so levelling up does not spam them
local RECORD_QUIET_BELOW = 50           -- no fanfare for records smaller than this (the first hits of a new character)

local QUALITY_COLOURS = {
    [2] = { 0.12, 1.00, 0.00 },
    [3] = { 0.00, 0.44, 0.87 },
    [4] = { 0.64, 0.21, 0.93 },
    [5] = { 1.00, 0.50, 0.00 },
}
local PULSES = { [2] = 1, [3] = 2, [4] = 4, [5] = 6 }
local SOUNDS = {
    [2] = "Sound\\Interface\\LootCoinLarge.wav",
    [3] = "Sound\\Interface\\iQuestComplete.wav",
    [4] = "Sound\\Interface\\LevelUp.wav",
    [5] = "Sound\\Interface\\LevelUp.wav",
}

local function Options()
    WowPlusFlairDB = WowPlusFlairDB or {}
    local db = WowPlusFlairDB
    if db.loot == nil then db.loot = true end
    if not db.calmer then db.hits, db.calmer = false, true end     -- once: the second number on big hits is off from now on
    if db.hits == nil then db.hits = false end
    if db.record == nil then db.record = true end
    return db
end

-- ---------------------------------------------------------------------------------------------
-- The flash at the edges of the screen, and the beam
-- ---------------------------------------------------------------------------------------------

local flash = CreateFrame("Frame", nil, UIParent)
flash:SetAllPoints(UIParent)
flash:SetFrameStrata("BACKGROUND")
flash:Hide()

local function Edge(point, width, height, ...)
    local texture = flash:CreateTexture(nil, "BACKGROUND")
    texture:SetTexture(1, 1, 1, 1)
    texture:SetPoint(point)
    if width then texture:SetWidth(width) else texture:SetPoint(point == "TOP" and "TOPLEFT" or "BOTTOMLEFT") texture:SetPoint(point == "TOP" and "TOPRIGHT" or "BOTTOMRIGHT") end
    if height then texture:SetHeight(height) else texture:SetPoint(point == "LEFT" and "TOPLEFT" or "TOPRIGHT") texture:SetPoint(point == "LEFT" and "BOTTOMLEFT" or "BOTTOMRIGHT") end
    texture:SetGradientAlpha(...)
    return texture
end

local EDGE = 140
local edges = {
    Edge("TOP",    nil, EDGE, "VERTICAL",   1, 1, 1, 0, 1, 1, 1, 1),
    Edge("BOTTOM", nil, EDGE, "VERTICAL",   1, 1, 1, 1, 1, 1, 1, 0),
    Edge("LEFT",   EDGE, nil, "HORIZONTAL", 1, 1, 1, 1, 1, 1, 1, 0),
    Edge("RIGHT",  EDGE, nil, "HORIZONTAL", 1, 1, 1, 0, 1, 1, 1, 1),
}

local beam = flash:CreateTexture(nil, "ARTWORK")
beam:SetTexture(1, 1, 1, 1)
beam:SetWidth(90)
beam:SetPoint("BOTTOM", UIParent, "CENTER", 0, 20)
beam:SetHeight(420)
beam:SetBlendMode("ADD")

local banner = CreateFrame("Frame", nil, UIParent)
banner:SetWidth(600)
banner:SetHeight(60)
banner:SetPoint("CENTER", 0, 150)
banner:SetFrameStrata("HIGH")
banner:Hide()

local bannerIcon = banner:CreateTexture(nil, "ARTWORK")
bannerIcon:SetWidth(44)
bannerIcon:SetHeight(44)
bannerIcon:SetPoint("RIGHT", banner, "CENTER", -8, 0)

local bannerText = banner:CreateFontString(nil, "OVERLAY")
bannerText:SetFont(STANDARD_TEXT_FONT, 26, "OUTLINE")
bannerText:SetPoint("LEFT", banner, "CENTER", 0, 0)

local effect = { time = 0, length = 0, pulses = 0 }

local function StartEffect(r, g, b, pulses, text, icon)
    effect.time, effect.pulses, effect.length = 0, pulses, 0.9 + pulses * 0.55
    for _, edge in ipairs(edges) do edge:SetVertexColor(r, g, b) end
    beam:SetGradientAlpha("VERTICAL", r, g, b, 0.9, r, g, b, 0)
    bannerText:SetText(text)
    bannerText:SetTextColor(r, g, b)
    if icon then
        bannerIcon:SetTexture(icon)
        bannerIcon:Show()
        bannerText:SetPoint("LEFT", banner, "CENTER", 0, 0)
        -- Centre the icon and the name together
        local half = (44 + 8 + bannerText:GetStringWidth()) / 2
        bannerIcon:ClearAllPoints()
        bannerIcon:SetPoint("LEFT", banner, "CENTER", -half, 0)
        bannerText:ClearAllPoints()
        bannerText:SetPoint("LEFT", bannerIcon, "RIGHT", 8, 0)
    else
        bannerIcon:Hide()
        bannerText:ClearAllPoints()
        bannerText:SetPoint("CENTER", banner, "CENTER", 0, 0)
    end
    flash:Show()
    banner:Show()
end

flash:SetScript("OnUpdate", function(self, elapsed)
    effect.time = effect.time + elapsed
    local t = effect.time
    if t >= effect.length then
        self:Hide()
        banner:Hide()
        return
    end

    -- The colour pulses: bright at the start of each pulse, fading to the next
    local pulseLength = 0.55
    local pulse = math.floor(t / pulseLength)
    local glow = 0
    if pulse < effect.pulses then
        glow = 1 - (t - pulse * pulseLength) / pulseLength
    end
    local fadeOut = math.min(1, (effect.length - t) / 0.6)
    self:SetAlpha(1)
    for _, edge in ipairs(edges) do edge:SetAlpha(0.55 * glow) end
    beam:SetAlpha((0.35 + 0.65 * glow) * fadeOut)
    beam:SetHeight(200 + 260 * math.min(1, t / 0.25))
    banner:SetAlpha(fadeOut)
    banner:SetScale(1 + 0.25 * math.max(0, 1 - t / 0.2))
end)

-- ---------------------------------------------------------------------------------------------
-- Loot
-- ---------------------------------------------------------------------------------------------

-- "You receive loot: %s." and "You receive loot: %sx%d.", in whatever language the game is in
local lootPatterns = {}
for _, text in ipairs({ LOOT_ITEM_SELF_MULTIPLE, LOOT_ITEM_SELF, LOOT_ITEM_PUSHED_SELF_MULTIPLE, LOOT_ITEM_PUSHED_SELF }) do
    local pattern = string.gsub(text, "[%(%)%.%+%-%*%?%[%]%^%$]", "%%%0")
    pattern = string.gsub(pattern, "%%s", "(.+)")
    pattern = string.gsub(pattern, "%%d", "%%d+")
    tinsert(lootPatterns, "^" .. pattern .. "$")
end

local function OnLoot(message)
    if not Options().loot then return end
    for _, pattern in ipairs(lootPatterns) do
        local link = string.match(message, pattern)
        if link then
            local name, _, quality, _, _, _, _, _, _, icon = GetItemInfo(link)
            if name and quality and quality >= LOOT_FROM_QUALITY and QUALITY_COLOURS[quality] then
                local colour = QUALITY_COLOURS[quality]
                StartEffect(colour[1], colour[2], colour[3], PULSES[quality], name, icon)
                PlaySoundFile(SOUNDS[quality])
            end
            return
        end
    end
end

-- ---------------------------------------------------------------------------------------------
-- Big hits and the record
-- ---------------------------------------------------------------------------------------------

local hitText = UIParent:CreateFontString(nil, "OVERLAY")
hitText:SetFont(DAMAGE_TEXT_FONT or STANDARD_TEXT_FONT, 44, "THICKOUTLINE")
hitText:SetPoint("CENTER", 0, 60)
hitText:SetAlpha(0)

local hit = CreateFrame("Frame")
hit.time = 1
hit:SetScript("OnUpdate", function(self, elapsed)
    if self.time >= 1 then return end
    self.time = self.time + elapsed / 1.2
    local t = math.min(1, self.time)
    hitText:SetAlpha(t < 0.6 and 1 or (1 - t) / 0.4)
    hitText:SetPoint("CENTER", 0, 60 + 70 * t)
    -- Pops large, then settles
    hitText:SetTextHeight(self.size * (1 + 0.6 * math.max(0, 1 - t / 0.12)))
end)

local function ShowHit(amount, size, r, g, b)
    hit.time, hit.size = 0, size
    hitText:SetText(amount)
    hitText:SetTextColor(r, g, b)
    hitText:SetTextHeight(size)
end

local lastFanfare = 0

local function OnDamage(amount, critical, spell, target)
    local db = Options()
    local record = db.best or 0

    if amount > record then
        db.best, db.bestSpell, db.bestTarget, db.bestDate = amount, spell, target, date("%d %b %Y")
        if db.record and amount >= RECORD_QUIET_BELOW and GetTime() - lastFanfare >= RECORD_PAUSE then
            lastFanfare = GetTime()
            PlaySoundFile("Sound\\Interface\\RaidWarning.wav")
            DEFAULT_CHAT_FRAME:AddMessage("|cffffd200Your hardest hit yet: " .. amount .. " with " .. spell .. " on " .. target .. ".|r" ..
                (record > 0 and " (Before: " .. record .. ")" or ""))
            ShowHit(amount, 56, 1, 0.82, 0.1)
            return
        end
    end

    if db.hits and (critical or (record > 0 and amount >= record * BIG_HIT_SHARE)) then
        -- The closer to the record, the larger
        local share = record > 0 and math.min(1, amount / record) or 1
        ShowHit(amount, 30 + 20 * share, 1, critical and 0.3 or 0.85, 0.1)
    end
end

local driver = CreateFrame("Frame")
driver:RegisterEvent("CHAT_MSG_LOOT")
driver:RegisterEvent("COMBAT_LOG_EVENT_UNFILTERED")
driver:SetScript("OnEvent", function(self, event, ...)
    if event == "CHAT_MSG_LOOT" then
        OnLoot((...))
        return
    end

    local _, kind, sourceGUID, _, _, _, destName = ...
    if sourceGUID ~= UnitGUID("player") then return end

    if kind == "SWING_DAMAGE" then
        local amount, _, _, _, _, _, critical = select(9, ...)
        OnDamage(amount, critical, "a melee hit", destName or "?")
    elseif kind == "SPELL_DAMAGE" or kind == "RANGE_DAMAGE" then
        local _, spellName, _, amount, _, _, _, _, _, critical = select(9, ...)
        OnDamage(amount, critical, spellName or "?", destName or "?")
    end
end)

SLASH_WOWPLUSFLAIR1 = "/flair"
SlashCmdList["WOWPLUSFLAIR"] = function(text)
    local db = Options()
    local say = function(line) DEFAULT_CHAT_FRAME:AddMessage("|cffffd200Flair:|r " .. line) end
    if text == "loot" then db.loot = not db.loot
    elseif text == "hits" then db.hits = not db.hits
    elseif text == "record" then db.record = not db.record
    elseif text == "reset" then db.best, db.bestSpell, db.bestTarget, db.bestDate = nil, nil, nil, nil
    elseif text == "test" then
        StartEffect(0.64, 0.21, 0.93, 4, "Staff of Jordan", "Interface\\Icons\\INV_Staff_13")
        PlaySoundFile(SOUNDS[4])
        return
    end
    if db.best then
        say("your hardest hit is " .. db.best .. " with " .. db.bestSpell .. " on " .. db.bestTarget .. " (" .. db.bestDate .. ").")
    else
        say("no hit recorded yet.")
    end
    say("loot effect " .. (db.loot and "on" or "off") .. ", big hits " .. (db.hits and "on" or "off") .. ", record fanfare " .. (db.record and "on" or "off") ..
        ".  Switch with /flair loot, /flair hits, /flair record. /flair reset forgets the record, /flair test shows the loot effect.")
end