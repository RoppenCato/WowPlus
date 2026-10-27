-- The voices of this server's own quests, as a pack for Spoken Quests (the same shape as Spoken's own packs).
-- The lookups and the sounds are made by Settings/Scripts/Make-Voices.py; see Settings/Docs/Voice.md.
if not VoiceOver or not VoiceOver.DataModules then return end

SpokenQuestsAudioWowPlus = {}

function SpokenQuestsAudioWowPlus:GetSoundPath(fileName, event)
    return format([[generated\sounds\quests\%s.ogg]], fileName)
end

VoiceOver.DataModules:Register("SpokenQuestsAudioWowPlus", SpokenQuestsAudioWowPlus)
