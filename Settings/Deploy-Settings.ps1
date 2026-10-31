# Builds the server's real config files from the defaults (.conf.dist) plus our
# overrides in this folder, and writes them into the built server.
# Run by Start-Server.bat on every start; can also be run on its own.

$ErrorActionPreference = 'Stop'

$root    = Split-Path $PSScriptRoot -Parent
$configs = Join-Path $root 'Build\bin\RelWithDebInfo\configs'

# Our override file -> the default file it is applied to (relative to the configs folder)
$files = [ordered]@{
    'worldserver.conf' = 'worldserver.conf.dist'
    'authserver.conf'  = 'authserver.conf.dist'
    'playerbots.conf'  = 'modules\playerbots.conf.dist'
}

if (-not (Test-Path $configs)) {
    throw "The server is not built yet ($configs is missing). Run Build-Server.bat first."
}

# Every other module: its default file is used as it is, unless a file with the module's
# config name exists in this folder (for example Settings\SoloLfg.conf) with our changes.
foreach ($dist in Get-ChildItem (Join-Path $configs 'modules') -Filter *.conf.dist -ErrorAction SilentlyContinue) {
    $name = $dist.Name -replace '\.dist$', ''
    if (-not $files.Contains($name)) { $files[$name] = "modules\$($dist.Name)" }
}

# Secrets from Settings/.env
$envFile = Join-Path $PSScriptRoot '.env'
if (-not (Test-Path $envFile)) {
    throw "Settings\.env is missing. Copy Settings\.env.example to Settings\.env and set the password."
}
$secrets = @{}
foreach ($line in [IO.File]::ReadAllLines($envFile)) {
    if ($line -match '^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.*?)\s*$') { $secrets[$Matches[1]] = $Matches[2] }
}

$utf8 = New-Object System.Text.UTF8Encoding($false)

foreach ($name in $files.Keys) {
    $distPath = Join-Path $configs $files[$name]
    if (-not (Test-Path $distPath)) { throw "Default config not found: $distPath" }

    # Read our overrides: "Key = Value" lines, everything else is a comment
    $overrides = [ordered]@{}
    $ours = Join-Path $PSScriptRoot $name
    $ourLines = if (Test-Path $ours) { [IO.File]::ReadAllLines($ours) } else { @() }
    foreach ($line in $ourLines) {
        if ($line -match '^\s*([^#\[\s=][^=]*?)\s*=\s*(.*?)\s*$') {
            $value = $Matches[2]
            foreach ($key in $secrets.Keys) { $value = $value.Replace("{$key}", $secrets[$key]) }
            $overrides[$Matches[1]] = $value
        }
    }

    # Replace matching lines in the default file
    $used  = @{}
    $lines = foreach ($line in [IO.File]::ReadAllLines($distPath)) {
        if ($line -match '^\s*([^#\[\s=][^=]*?)\s*=' -and $overrides.Contains($Matches[1])) {
            $used[$Matches[1]] = $true
            "$($Matches[1]) = $($overrides[$Matches[1]])"
        } else {
            $line
        }
    }

    # Settings the default file does not have are added at the end
    $extra = @($overrides.Keys | Where-Object { -not $used.ContainsKey($_) })
    if ($extra.Count -gt 0) {
        $lines = @($lines) + '' + '# Added from Settings (not in the default file)'
        foreach ($key in $extra) {
            $lines += "$key = $($overrides[$key])"
            Write-Warning "$name`: '$key' is not in $($files[$name]). Check the spelling."
        }
    }

    $target = $distPath -replace '\.dist$', ''
    [IO.File]::WriteAllLines($target, [string[]]$lines, $utf8)
    Write-Host "Deployed $name ($($overrides.Count) settings changed)"
}

New-Item -ItemType Directory -Force (Join-Path $root 'Build\bin\RelWithDebInfo\logs') | Out-Null

# If the world server was closed without a clean shutdown, the realm stays marked as
# "offline + wrong version" and the login server then refuses to start. Clear that mark.
# The world server sets the right state itself while it starts. Fails harmlessly when the
# database is still empty (very first start).
$ErrorActionPreference = 'Continue'
$mysql = Join-Path $root 'Tools\mysql\bin\mysql.exe'
$myini = "--defaults-file=$(Join-Path $PSScriptRoot 'mysql.ini')"

# Our own SQL changes: every file in Settings\SQL\world, \characters and \auth is applied on
# every start, in name order. The files are written so that running them again is harmless.
# The world seed (WowPlus.World.Seed) is handed to every SQL file as @wowplus_seed, for the files
# that build something from it, such as the champions.
$seed = 1
foreach ($file in (Join-Path $PSScriptRoot 'mod_wowplus.conf'), (Join-Path $root 'Server\modules\mod-wowplus\conf\mod_wowplus.conf.dist')) {
    $line = Get-Content $file -ErrorAction SilentlyContinue | Where-Object { $_ -match '^\s*WowPlus\.World\.Seed\s*=\s*(\d+)' } | Select-Object -First 1
    if ($line -and $line -match '=\s*(\d+)') { $seed = [uint64]$Matches[1]; break }
}
foreach ($db in 'world', 'characters', 'auth') {
    foreach ($sql in Get-ChildItem (Join-Path $PSScriptRoot "SQL\$db") -Filter *.sql -ErrorAction SilentlyContinue | Sort-Object Name) {
        ("SET @wowplus_seed = $seed;`n" + (Get-Content -LiteralPath $sql.FullName -Raw)) | & $mysql $myini "acore_$db" 2>&1 | Out-Null
        if ($LASTEXITCODE -eq 0) { Write-Host "Applied SQL\$db\$($sql.Name)" }
        else { Write-Warning "SQL\$db\$($sql.Name) failed. Run it by hand to see the error." }
    }
}

# Enough bots of either faction. The bots in the world are all of the side you play
# (WowPlus.Bots.FollowFaction), and the bot module fills the world from a fixed set of accounts.
# It counts on half the characters in them being of each faction; when there are too few of one, it
# fills the last places with the other. Extra accounts (Playerbots.RandomBotAccountCount in
# Settings\playerbots.conf) are made by the module but left unused, so here they are handed to the
# pool for bots in the world whenever either faction is short.
$maxBots = 500
foreach ($line in Get-Content (Join-Path $PSScriptRoot 'playerbots.conf') -ErrorAction SilentlyContinue) {
    if ($line -match '^\s*Playerbots\.MaxRandomBots\s*=\s*(\d+)') { $maxBots = [int]$Matches[1] }
}
$short = $false
foreach ($races in '2,5,6,8,10', '1,3,4,7,11') {
    $have = & $mysql $myini -N -e "SELECT COUNT(*) FROM acore_characters.characters c JOIN acore_playerbots.playerbots_account_type t ON t.account_id = c.account WHERE t.account_type = 1 AND c.race IN ($races)" 2>$null
    if ($LASTEXITCODE -eq 0 -and [int]$have -lt $maxBots) { $short = $true }
}
if ($short) {
    & $mysql $myini -e "UPDATE acore_playerbots.playerbots_account_type SET account_type = 1 WHERE account_type = 0" 2>&1 | Out-Null
}

# What bosses drop: the list for the game and the same list for the guide addon, made from the
# game's loot tables and the world seed (Settings\Docs\Boss-Loot.md). Then the rest of what the guide
# shows: levels, entrances, quests, boss stories, champions (Settings\Docs\Guide.md). Needs Python;
# without it the lists from last time stay.
if (Get-Command python -ErrorAction SilentlyContinue) {
    & python (Join-Path $PSScriptRoot 'Scripts\Make-Boss-Loot.py') 2>&1 | ForEach-Object { Write-Host $_ }
    & python (Join-Path $PSScriptRoot 'Scripts\Make-Guide.py') 2>&1 | ForEach-Object { Write-Host $_ }
    & python (Join-Path $PSScriptRoot 'Scripts\Make-Dungeon-Maps.py') 2>&1 | ForEach-Object { Write-Host $_ }
}

# Our own quests: one file each in Settings\Quests, put into the game here (Settings\Quests\README.md).
if (Get-Command python -ErrorAction SilentlyContinue) {
    & python (Join-Path $PSScriptRoot 'Scripts\Make-Quests.py') 2>&1 | ForEach-Object { Write-Host $_ }
    # Their voices, for quests that are new or changed (Settings\Docs\Voice.md). Only where the voice
    # model is installed; anywhere else the voices from last time stay. Voices are off for now.
    $voicesOn = $false
    if ($voicesOn) { & python -c "import kokoro_onnx" 2>$null }
    if ($voicesOn -and $LASTEXITCODE -eq 0) {
        & python (Join-Path $PSScriptRoot 'Scripts\Make-Voices.py') 2>&1 | ForEach-Object { Write-Host $_ }
    }
}

# Our own addons (Client\AddOns) are copied into every game copy, so the game always has the
# version that matches the server. A running game picks them up after /reload or a restart.
foreach ($client in Get-ChildItem (Join-Path $root 'Client') -Directory -ErrorAction SilentlyContinue | Where-Object { Test-Path (Join-Path $_.FullName 'Wow.exe') }) {
    # Addons of ours that have been replaced: the Loot Book became the Guide (2026-10-27)
    foreach ($retired in @('WowPlusLootBook')) {
        $old = Join-Path $client.FullName "Interface\AddOns\$retired"
        if (Test-Path $old) { Remove-Item $old -Recurse -Force }
    }
    foreach ($addon in Get-ChildItem (Join-Path $root 'Client\AddOns') -Directory -ErrorAction SilentlyContinue) {
        if ($addon.Name -eq 'SpokenQuestsAudioWowPlus' -and -not $voicesOn) { continue }
        if ($addon.Name -eq 'WowPlusDialogue') { continue }     # turned off for now: the game's own quest windows (Settings\Docs\Dialogue.md)
        $to = Join-Path $client.FullName "Interface\AddOns\$($addon.Name)"
        New-Item -ItemType Directory -Force $to | Out-Null
        Copy-Item (Join-Path $addon.FullName '*') $to -Recurse -Force
    }
}

& (Join-Path $root 'Tools\mysql\bin\mysql.exe') "--defaults-file=$(Join-Path $PSScriptRoot 'mysql.ini')" -e "UPDATE acore_auth.realmlist SET flag = flag & ~3" 2>&1 | Out-Null
exit 0
