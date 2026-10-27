-- WowPlus Dialogue: talking to someone as a conversation, in the spirit of the Immersion addon.
--
-- Instead of the gossip and quest windows, the person you talk to is shown up close in a panel low on
-- the screen, and what they say comes a few lines at a time. Space (or a click on the panel) goes on;
-- when they are done, your answers are numbered and 1-9 picks one. Escape walks away.
--
-- Covers everything those windows do: gossip and its options, a quest giver's greeting, a quest's
-- details (accept or decline), its progress (hand in, or not yet) and its rewards (pick one).
-- /dialogue off gives back the game's own windows, /dialogue on takes them again.
--
-- Written for the 3.3.5 game, so: no keyboard capture (Space and the number keys are bound to the
-- panel only while it is open, and not in combat), and the game's windows are kept from opening by
-- taking away the events that open them. Settings/Docs/Dialogue.md.

local PAGE_LENGTH = 300                             -- characters on one page, at most; longer paragraphs are split at sentences
local OPTION_ROWS = 9
local HEADING_FONT = "Fonts\\MORPHEUS.TTF"
local TALK, ASK, EXCLAIM = 60, 65, 64               -- the talking animations

local db
local state = { kind = nil, pages = {}, page = 1, options = {}, rewards = {}, choice = nil, title = nil }
local closing = nil                                 -- a pending close: the game says goodbye between two windows too

-- ---------------------------------------------------------------------------------------------
-- The panel
-- ---------------------------------------------------------------------------------------------

local frame = CreateFrame("Button", "WowPlusDialogueFrame", UIParent)
frame:SetWidth(760)
frame:SetHeight(196)
frame:SetPoint("BOTTOM", 0, 170)
frame:SetBackdrop({ bgFile = "Interface\\Tooltips\\UI-Tooltip-Background", edgeFile = "Interface\\DialogFrame\\UI-DialogBox-Gold-Border",
                    tile = true, tileSize = 16, edgeSize = 24, insets = { left = 6, right = 6, top = 6, bottom = 6 } })
frame:SetBackdropColor(0.03, 0.02, 0.01, 0.92)
frame:SetFrameStrata("DIALOG")
frame:SetClampedToScreen(true)
frame:SetMovable(true)
frame:RegisterForDrag("LeftButton")
frame:RegisterForClicks("LeftButtonUp")
frame:Hide()
tinsert(UISpecialFrames, "WowPlusDialogueFrame")

-- The person, up close
local portraitBack = frame:CreateTexture(nil, "BACKGROUND")
portraitBack:SetPoint("TOPLEFT", 10, -10)
portraitBack:SetWidth(176)
portraitBack:SetHeight(176)
portraitBack:SetTexture(0, 0, 0, 0.6)
local portrait = CreateFrame("PlayerModel", nil, frame)
portrait:SetPoint("TOPLEFT", portraitBack)
portrait:SetPoint("BOTTOMRIGHT", portraitBack)
local portraitEdge = frame:CreateTexture(nil, "OVERLAY")
portraitEdge:SetTexture("Interface\\AchievementFrame\\UI-Achievement-IconFrame")
portraitEdge:SetTexCoord(0, 0.5625, 0, 0.5625)
portraitEdge:SetPoint("TOPLEFT", portraitBack, "TOPLEFT", -8, 8)
portraitEdge:SetPoint("BOTTOMRIGHT", portraitBack, "BOTTOMRIGHT", 8, -8)

local name = frame:CreateFontString(nil, "OVERLAY")
name:SetFont(HEADING_FONT, 22, "")
name:SetTextColor(1, 0.82, 0.25)
name:SetShadowColor(0, 0, 0, 1)
name:SetShadowOffset(1, -1)
name:SetPoint("TOPLEFT", 202, -16)
name:SetJustifyH("LEFT")
local title = frame:CreateFontString(nil, "OVERLAY", "GameFontNormal")
title:SetPoint("LEFT", name, "RIGHT", 12, -2)
title:SetJustifyH("LEFT")

local text = frame:CreateFontString(nil, "OVERLAY")
text:SetFont("Fonts\\FRIZQT__.TTF", 15, "")
text:SetTextColor(0.95, 0.92, 0.85)
text:SetShadowColor(0, 0, 0, 1)
text:SetShadowOffset(1, -1)
text:SetPoint("TOPLEFT", 202, -48)
text:SetWidth(540)
text:SetJustifyH("LEFT")
text:SetJustifyV("TOP")
text:SetSpacing(3)

local hint = frame:CreateFontString(nil, "OVERLAY", "GameFontDisableSmall")
hint:SetPoint("BOTTOMRIGHT", -14, 12)
local pageCount = frame:CreateFontString(nil, "OVERLAY", "GameFontDisableSmall")
pageCount:SetPoint("BOTTOMLEFT", 202, 12)

-- Answers, stacked above the panel's right side
local answers = CreateFrame("Frame", nil, frame)
answers:SetPoint("BOTTOMRIGHT", frame, "TOPRIGHT", 0, 4)
answers:SetWidth(470)
answers:SetBackdrop({ bgFile = "Interface\\Tooltips\\UI-Tooltip-Background", edgeFile = "Interface\\Tooltips\\UI-Tooltip-Border",
                      tile = true, tileSize = 16, edgeSize = 14, insets = { left = 4, right = 4, top = 4, bottom = 4 } })
answers:SetBackdropColor(0.03, 0.02, 0.01, 0.9)
answers:SetBackdropBorderColor(0.8, 0.65, 0.3)
answers:EnableMouseWheel(true)
answers.first = 0
answers:Hide()

local optionButtons = {}
for index = 1, OPTION_ROWS do
    local button = CreateFrame("Button", "WowPlusDialogueChoice" .. index, answers)
    button:SetHeight(24)
    button:SetPoint("TOPLEFT", 8, -8 - (index - 1) * 26)
    button:SetPoint("RIGHT", -8, 0)
    button:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight", "ADD")
    button.key = button:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    button.key:SetPoint("LEFT", 4, 0)
    button.key:SetWidth(18)
    button.icon = button:CreateTexture(nil, "ARTWORK")
    button.icon:SetWidth(16)
    button.icon:SetHeight(16)
    button.icon:SetPoint("LEFT", 24, 0)
    button.label = button:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
    button.label:SetPoint("LEFT", 46, 0)
    button.label:SetPoint("RIGHT", -4, 0)
    button.label:SetJustifyH("LEFT")
    button:SetScript("OnClick", function(self)
        local option = state.options[self.index]
        if option and option.action then
            PlaySound("igMainMenuOptionCheckBoxOn")
            option.action()
        end
    end)
    optionButtons[index] = button
end

-- Rewards, under the answers when there are any
local rewardStrip = CreateFrame("Frame", nil, answers)
rewardStrip:SetPoint("BOTTOMLEFT", 8, 8)
rewardStrip:SetPoint("BOTTOMRIGHT", -8, 8)
rewardStrip:SetHeight(64)
local rewardLabel = rewardStrip:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
rewardLabel:SetPoint("TOPLEFT", 0, 0)
local money = rewardStrip:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
money:SetPoint("TOPRIGHT", 0, 0)
local rewardButtons = {}
for index = 1, 10 do
    local button = CreateFrame("Button", nil, rewardStrip)
    button:SetWidth(40)
    button:SetHeight(40)
    button:SetPoint("TOPLEFT", (index - 1) * 44, -16)
    button.icon = button:CreateTexture(nil, "ARTWORK")
    button.icon:SetAllPoints()
    button.count = button:CreateFontString(nil, "OVERLAY", "NumberFontNormal")
    button.count:SetPoint("BOTTOMRIGHT", -2, 2)
    button.glow = button:CreateTexture(nil, "OVERLAY")
    button.glow:SetTexture("Interface\\Buttons\\CheckButtonHilight")
    button.glow:SetBlendMode("ADD")
    button.glow:SetAllPoints()
    button.glow:Hide()
    button:SetHighlightTexture("Interface\\Buttons\\ButtonHilight-Square", "ADD")
    button:SetScript("OnEnter", function(self)
        GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
        if self.kind == "spell" then GameTooltip:SetQuestRewardSpell()
        else GameTooltip:SetQuestItem(self.kind, self.index) end
        GameTooltip:Show()
    end)
    button:SetScript("OnLeave", function() GameTooltip:Hide() end)
    button:SetScript("OnClick", function(self)
        if IsModifiedClick() then
            HandleModifiedItemClick(GetQuestItemLink(self.kind, self.index))
        elseif self.kind == "choice" and state.kind == "complete" then
            state.choice = self.index
            PlaySound("igMainMenuOptionCheckBoxOn")
            for _, other in ipairs(rewardButtons) do if other.glow then other.glow:Hide() end end
            self.glow:Show()
        end
    end)
    rewardButtons[index] = button
end

-- An invisible button for Space, so that it can be bound like the answers
local nextButton = CreateFrame("Button", "WowPlusDialogueNext", frame)

-- ---------------------------------------------------------------------------------------------
-- Pages and answers
-- ---------------------------------------------------------------------------------------------

-- What the person says, cut into pages: paragraphs, and long paragraphs at the end of a sentence
local function Pages(said)
    local pages = {}
    said = (said or ""):gsub("\r", "")
    for paragraph in (said .. "\n\n"):gmatch("(.-)\n\n+") do
        paragraph = strtrim((paragraph:gsub("\n", " ")))
        while #paragraph > PAGE_LENGTH do
            local cut
            for at in paragraph:gmatch("()[%.%!%?][\"']?%s") do
                if at > PAGE_LENGTH then break end
                cut = at
            end
            if not cut then break end
            local stop = paragraph:find("%s", cut)
            tinsert(pages, strtrim(paragraph:sub(1, stop)))
            paragraph = strtrim(paragraph:sub(stop + 1))
        end
        if paragraph ~= "" then tinsert(pages, paragraph) end
    end
    return pages
end

local function Animate(said)
    if not portrait.SetSequence then return end
    local animation = TALK
    if said and said:find("%?%s*$") then animation = ASK elseif said and said:find("!%s*$") then animation = EXCLAIM end
    pcall(portrait.SetSequence, portrait, animation)
end

local function Bind()
    if InCombatLockdown() then return end
    ClearOverrideBindings(frame)
    SetOverrideBindingClick(frame, true, "SPACE", "WowPlusDialogueNext")
    for index = 1, OPTION_ROWS do
        SetOverrideBindingClick(frame, true, tostring(index), "WowPlusDialogueChoice" .. index)
    end
end

local function Unbind()
    if not InCombatLockdown() then ClearOverrideBindings(frame) end
end

local ShowAnswers

local function ShowPage()
    local said = state.pages[state.page] or ""
    text:SetText(said)
    UIFrameFadeIn(text, 0.3, 0, 1)
    Animate(said)
    local last = state.page >= #state.pages
    pageCount:SetText(#state.pages > 1 and (state.page .. " / " .. #state.pages) or "")
    if last then
        ShowAnswers()
        hint:SetText(#state.options > 0 and ("|cffffd1001-" .. math.min(#state.options, OPTION_ROWS) .. "|r to answer,  |cffffd100Esc|r to leave") or "|cffffd100Esc|r to leave")
    else
        answers:Hide()
        hint:SetText("|cffffd100Space|r or click to go on")
    end
end

local function NextPage()
    if state.page < #state.pages then
        state.page = state.page + 1
        PlaySound("igQuestListOpen")
        ShowPage()
    elseif #state.options == 1 and state.options[1].onSpace then
        state.options[1].action()
    end
end
frame:SetScript("OnClick", NextPage)
nextButton:SetScript("OnClick", NextPage)

function ShowAnswers()
    local shown = 0
    for index, button in ipairs(optionButtons) do
        local option = state.options[index + answers.first]
        button.index = index + answers.first
        if option then
            button.key:SetText(index .. ".")
            button.icon:SetTexture(option.icon)
            button.label:SetText(option.text)
            if option.dim then button.label:SetTextColor(0.6, 0.6, 0.6) else button.label:SetTextColor(1, 1, 1) end
            button:Show()
            shown = index
        else
            button:Hide()
        end
    end

    local rewards = state.rewards
    local anyReward = #rewards > 0 or (rewards.money or 0) > 0
    for index, button in ipairs(rewardButtons) do
        local reward = rewards[index]
        if reward then
            button.kind, button.index = reward.kind, reward.index
            button.icon:SetTexture(reward.texture)
            button.count:SetText(reward.count and reward.count > 1 and reward.count or "")
            if reward.usable == false then button.icon:SetVertexColor(1, 0.3, 0.3) else button.icon:SetVertexColor(1, 1, 1) end
            if state.choice == reward.index and reward.kind == "choice" then button.glow:Show() else button.glow:Hide() end
            button:Show()
        else
            button:Hide()
        end
    end
    rewardLabel:SetText(rewards.label or "")
    money:SetText((rewards.money or 0) > 0 and GetCoinTextureString(rewards.money) or "")
    if anyReward then rewardStrip:Show() else rewardStrip:Hide() end

    if shown == 0 and not anyReward then
        answers:Hide()
        return
    end
    answers:SetHeight(16 + shown * 26 + (anyReward and 66 or 0))
    answers:Show()
end
answers:SetScript("OnMouseWheel", function(self, delta)
    self.first = math.max(0, math.min(self.first - delta, #state.options - OPTION_ROWS))
    ShowAnswers()
end)

-- ---------------------------------------------------------------------------------------------
-- Who is talking, and what about
-- ---------------------------------------------------------------------------------------------

local function Open(kind, heading, said, options, rewards)
    closing = nil
    state.kind, state.pages, state.page, state.options, state.rewards, state.choice = kind, Pages(said), 1, options or {}, rewards or {}, nil
    if #state.pages == 0 then state.pages = { "" } end
    answers.first = 0
    local who = UnitName("npc") or UnitName("questnpc") or ""
    name:SetText(who)
    title:SetText(heading and ("|cffa0a0a0" .. heading .. "|r") or "")
    if not frame:IsShown() then
        frame:Show()
        portrait:ClearModel()
        portrait:SetUnit("npc")
        portrait:SetCamera(0)
    end
    Bind()
    ShowPage()
    if WowPlusDialogue_OnSay then WowPlusDialogue_OnSay(kind, who, heading, said) end   -- for WowPlus Voice
end

local function Close()
    closing = nil
    frame:Hide()
end

local function GossipIcon(kind)
    return "Interface\\GossipFrame\\" .. (kind or "gossip") .. "GossipIcon"
end

-- Gossip: what they say, then quests to hand in, quests to take, and the other options
local function ShowGossip()
    local options = {}
    local active, available = GetNumGossipActiveQuests(), GetNumGossipAvailableQuests()
    local list = { GetGossipActiveQuests() }
    local step = active > 0 and math.floor(#list / active) or 1
    for index = 1, active do
        local quest, level, _, done = list[(index - 1) * step + 1], list[(index - 1) * step + 2], list[(index - 1) * step + 3], list[(index - 1) * step + 4]
        tinsert(options, { text = quest, icon = done and "Interface\\GossipFrame\\ActiveQuestIcon" or "Interface\\GossipFrame\\IncompleteQuestIcon",
                           dim = not done, action = function() SelectGossipActiveQuest(index) end })
    end
    list = { GetGossipAvailableQuests() }
    step = available > 0 and math.floor(#list / available) or 1
    for index = 1, available do
        local quest, level, trivial = list[(index - 1) * step + 1], list[(index - 1) * step + 2], list[(index - 1) * step + 3]
        tinsert(options, { text = quest .. (level and level > 0 and ("  |cffa0a0a0(" .. level .. ")|r") or ""), icon = "Interface\\GossipFrame\\AvailableQuestIcon",
                           dim = trivial, action = function() SelectGossipAvailableQuest(index) end })
    end
    list = { GetGossipOptions() }
    for index = 1, #list / 2 do
        tinsert(options, { text = list[index * 2 - 1], icon = GossipIcon(list[index * 2]), action = function() SelectGossipOption(index) end })
    end
    tinsert(options, { text = "Goodbye.", icon = "Interface\\GossipFrame\\PetitionGossipIcon", action = function() CloseGossip() Close() end })
    Open("gossip", nil, GetGossipText(), options)
end

-- A quest giver's greeting (when they have several quests and no gossip)
local function ShowGreeting()
    local options = {}
    for index = 1, GetNumActiveQuests() do
        tinsert(options, { text = GetActiveTitle(index), icon = "Interface\\GossipFrame\\ActiveQuestIcon", action = function() SelectActiveQuest(index) end })
    end
    for index = 1, GetNumAvailableQuests() do
        tinsert(options, { text = GetAvailableTitle(index), icon = "Interface\\GossipFrame\\AvailableQuestIcon", action = function() SelectAvailableQuest(index) end })
    end
    tinsert(options, { text = "Goodbye.", icon = "Interface\\GossipFrame\\PetitionGossipIcon", action = function() CloseQuest() Close() end })
    Open("greeting", nil, GetGreetingText(), options)
end

local function QuestRewards(label)
    local rewards = { label = label, money = GetRewardMoney() }
    for index = 1, GetNumQuestChoices() do
        local _, texture, count, _, usable = GetQuestItemInfo("choice", index)
        tinsert(rewards, { kind = "choice", index = index, texture = texture, count = count, usable = usable and true or false })
    end
    for index = 1, GetNumQuestRewards() do
        local _, texture, count, _, usable = GetQuestItemInfo("reward", index)
        tinsert(rewards, { kind = "reward", index = index, texture = texture, count = count, usable = usable and true or false })
    end
    local spellTexture = GetRewardSpell()
    if spellTexture then tinsert(rewards, { kind = "spell", index = 1, texture = spellTexture }) end
    return rewards
end

-- A quest offered: the story, then what to do, then accept or decline
local function ShowDetail()
    local said = GetQuestText()
    local todo = GetObjectiveText()
    if todo and todo ~= "" then said = said .. "\n\n|cffffd100What to do:|r " .. todo end
    local options = {
        { text = "I will do it.", icon = "Interface\\GossipFrame\\AvailableQuestIcon", action = function() AcceptQuest() Close() end },
        { text = "Not now.", icon = "Interface\\GossipFrame\\PetitionGossipIcon", action = function() DeclineQuest() Close() end },
    }
    Open("detail", GetTitleText(), said, options, QuestRewards(GetNumQuestChoices() > 1 and "You will choose one of" or "You will get"))
end

-- Asked how it goes: hand in what was asked for, or not yet
local function ShowProgress()
    local rewards = { label = GetNumQuestItems() > 0 and "Asked for" or nil, money = GetQuestMoneyToGet() }
    for index = 1, GetNumQuestItems() do
        local _, texture, count = GetQuestItemInfo("required", index)
        tinsert(rewards, { kind = "required", index = index, texture = texture, count = count })
    end
    local options = {}
    if IsQuestCompletable() then
        tinsert(options, { text = "Here it is.", icon = "Interface\\GossipFrame\\ActiveQuestIcon", action = function() CompleteQuest() end, onSpace = true })
    end
    tinsert(options, { text = "Not yet.", icon = "Interface\\GossipFrame\\IncompleteQuestIcon", action = function() CloseQuest() Close() end })
    Open("progress", GetTitleText(), GetProgressText(), options, rewards)
end

-- Done: thanks, and the reward, picking one if there is a choice
local function ShowComplete()
    local choices = GetNumQuestChoices()
    local options = { { text = choices > 1 and "Take the one I picked." or "Thank you.", icon = "Interface\\GossipFrame\\ActiveQuestIcon",
        action = function()
            if choices > 1 and not state.choice then
                UIErrorsFrame:AddMessage("Pick a reward first: click one of them.", 1, 0.82, 0)
                return
            end
            GetQuestReward(choices > 0 and (state.choice or 1) or nil)
            Close()
        end } }
    Open("complete", GetTitleText(), GetRewardText(), options, QuestRewards(choices > 1 and "Choose one" or "Your reward"))
    if choices == 1 then state.choice = 1 end
end

-- ---------------------------------------------------------------------------------------------
-- Taking over from the game's windows
-- ---------------------------------------------------------------------------------------------

local TAKEN = { GossipFrame = { "GOSSIP_SHOW" }, QuestFrame = { "QUEST_GREETING", "QUEST_DETAIL", "QUEST_PROGRESS", "QUEST_COMPLETE" } }

local function TakeOver(on)
    for frameName, events in pairs(TAKEN) do
        local window = _G[frameName]
        if window then
            for _, event in ipairs(events) do
                if on then window:UnregisterEvent(event) else window:RegisterEvent(event) end
            end
        end
    end
end

frame:SetScript("OnHide", function()
    Unbind()
    answers:Hide()
    if state.kind == "gossip" then CloseGossip() elseif state.kind then CloseQuest() end
    state.kind = nil
    PlaySound("igQuestListClose")
end)
frame:SetScript("OnShow", function() PlaySound("igQuestListOpen") end)

-- The game says goodbye between two windows as well (choosing a quest from gossip closes the gossip),
-- so a goodbye only closes the panel if nothing new has opened a moment later
frame:SetScript("OnUpdate", function(self, elapsed)
    if closing then
        closing = closing - elapsed
        if closing <= 0 then
            closing = nil
            state.kind = nil                        -- the game has already closed it; nothing to tell it
            self:Hide()
        end
    end
end)

local events = CreateFrame("Frame")
for _, event in ipairs({ "ADDON_LOADED", "GOSSIP_SHOW", "GOSSIP_CLOSED", "QUEST_GREETING", "QUEST_DETAIL", "QUEST_PROGRESS", "QUEST_COMPLETE",
                         "QUEST_FINISHED", "PLAYER_REGEN_DISABLED", "PLAYER_REGEN_ENABLED" }) do
    events:RegisterEvent(event)
end
events:SetScript("OnEvent", function(self, event, arg)
    if event == "ADDON_LOADED" then
        if arg ~= "WowPlusDialogue" then return end
        WowPlusDialogueDB = WowPlusDialogueDB or { on = true }
        db = WowPlusDialogueDB
        TakeOver(db.on)
        return
    end
    if not db or not db.on then return end
    if event == "GOSSIP_SHOW" then ShowGossip()
    elseif event == "QUEST_GREETING" then ShowGreeting()
    elseif event == "QUEST_DETAIL" then ShowDetail()
    elseif event == "QUEST_PROGRESS" then ShowProgress()
    elseif event == "QUEST_COMPLETE" then ShowComplete()
    elseif event == "GOSSIP_CLOSED" then
        if frame:IsShown() and state.kind == "gossip" then closing = 0.15 end
    elseif event == "QUEST_FINISHED" then
        if frame:IsShown() and state.kind ~= "gossip" then closing = 0.15 end
    elseif event == "PLAYER_REGEN_DISABLED" then Unbind()
    elseif event == "PLAYER_REGEN_ENABLED" and frame:IsShown() then Bind()
    end
end)

SLASH_WOWPLUSDIALOGUE1 = "/dialogue"
SlashCmdList["WOWPLUSDIALOGUE"] = function(arg)
    arg = strlower(strtrim(arg or ""))
    if arg == "off" or arg == "on" then
        db.on = arg == "on"
        TakeOver(db.on)
        if not db.on then frame:Hide() end
        DEFAULT_CHAT_FRAME:AddMessage("Dialogue: " .. (db.on and "conversations are shown up close." or "the game's own quest and gossip windows are back."))
    elseif arg == "reset" then
        frame:ClearAllPoints()
        frame:SetPoint("BOTTOM", 0, 170)
    else
        DEFAULT_CHAT_FRAME:AddMessage("Dialogue: /dialogue off, /dialogue on, /dialogue reset (puts the panel back). Drag the panel to move it.")
    end
end
frame:SetScript("OnDragStart", frame.StartMoving)
frame:SetScript("OnDragStop", frame.StopMovingOrSizing)
