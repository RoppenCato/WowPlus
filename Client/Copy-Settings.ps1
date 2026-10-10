# Copies game settings (graphics, sound, key bindings, interface options, macros, addon
# settings) between our game copies, to a new account, or to a new character.
# Started by Copy-Settings.bat. The game must be closed: it rewrites these files on exit.

$ErrorActionPreference = 'Stop'

if (Get-Process Wow -ErrorAction SilentlyContinue) {
    Write-Host 'The game is running. Close it first, then run this again.' -ForegroundColor Yellow
    exit 1
}

# Every game copy in this folder
$clients = @(Get-ChildItem $PSScriptRoot -Directory | Where-Object { Test-Path (Join-Path $_.FullName 'Wow.exe') })
if ($clients.Count -eq 0) { Write-Host 'No game copy found in this folder.'; exit 1 }

function Choose($title, $items) {
    Write-Host ''
    Write-Host $title
    for ($i = 0; $i -lt $items.Count; $i++) { Write-Host ("  {0}. {1}" -f ($i + 1), $items[$i]) }
    while ($true) {
        $answer = Read-Host 'Number'
        $n = 0
        if ([int]::TryParse($answer, [ref]$n) -and $n -ge 1 -and $n -le $items.Count) { return $n - 1 }
    }
}

# Copies a settings folder, leaving out the game's own backup files
function Copy-Folder($from, $to) {
    New-Item -ItemType Directory -Force $to | Out-Null
    robocopy $from $to /E /XF *.bak *.old /NFL /NDL /NJH /NJS /NP | Out-Null
}

function Accounts($client) {
    $dir = Join-Path $client.FullName 'WTF\Account'
    if (Test-Path $dir) { @(Get-ChildItem $dir -Directory) } else { @() }
}

$task = Choose 'What do you want to do?' @(
    'Copy ALL settings from one game copy to the other (graphics, sound, every account and character)',
    'Give a NEW ACCOUNT the settings of an existing account (key bindings, options, macros, addon settings)',
    'Give a NEW CHARACTER the settings of an existing character (chat windows, per-character options)'
)

if ($task -eq 0) {
    if ($clients.Count -lt 2) { Write-Host 'There is only one game copy.'; exit 1 }
    $from = $clients[(Choose 'Copy FROM which game copy?' $clients.Name)]
    $targets = @($clients | Where-Object { $_.FullName -ne $from.FullName })
    $to = if ($targets.Count -eq 1) { $targets[0] } else { $targets[(Choose 'Copy TO which game copy?' $targets.Name)] }
    Write-Host ''
    Write-Host "This replaces the settings in $($to.Name) with those from $($from.Name)."
    if ((Read-Host 'Continue? (y/n)') -ne 'y') { exit 0 }
    Copy-Folder (Join-Path $from.FullName 'WTF') (Join-Path $to.FullName 'WTF')
    Write-Host "Done. $($to.Name) now has the same settings." -ForegroundColor Green
    exit 0
}

# Tasks 2 and 3 work on one game copy and are then repeated in the others
$source = $clients[0]
if ($clients.Count -gt 1) { $source = $clients[(Choose 'Which game copy has the settings you want to reuse?' $clients.Name)] }
$accounts = Accounts $source
if ($accounts.Count -eq 0) { Write-Host 'No account has logged in with that game copy yet.'; exit 1 }
$account = $accounts[0]
if ($accounts.Count -gt 1) { $account = $accounts[(Choose 'Which account has the settings?' $accounts.Name)] }

if ($task -eq 1) {
    $new = (Read-Host 'Name of the NEW account (as created with "account create")').Trim().ToUpper()
    if (-not $new -or $new -eq $account.Name) { Write-Host 'No valid name given.'; exit 1 }
    foreach ($client in $clients) {
        $target = Join-Path $client.FullName "WTF\Account\$new"
        # Account-wide files only: the old account's characters do not exist on the new account
        New-Item -ItemType Directory -Force $target | Out-Null
        robocopy $account.FullName $target /XF *.bak *.old /NFL /NDL /NJH /NJS /NP | Out-Null
        $saved = Join-Path $account.FullName 'SavedVariables'
        if (Test-Path $saved) { Copy-Folder $saved (Join-Path $target 'SavedVariables') }
        Write-Host "Done in $($client.Name): account $new has the settings of $($account.Name)." -ForegroundColor Green
    }
    Write-Host 'Log in with the new account once before changing anything, so the settings are taken into use.'
    exit 0
}

# Task 3: character settings
$chars = @(Get-ChildItem $account.FullName -Directory | Where-Object { $_.Name -ne 'SavedVariables' } |
    ForEach-Object { $realm = $_; Get-ChildItem $realm.FullName -Directory | ForEach-Object { [pscustomobject]@{ Realm = $realm.Name; Name = $_.Name; Path = $_.FullName } } })
if ($chars.Count -eq 0) { Write-Host 'That account has no characters with saved settings yet.'; exit 1 }
$char = $chars[0]
if ($chars.Count -gt 1) { $char = $chars[(Choose 'Which character has the settings?' ($chars | ForEach-Object { "$($_.Name) ($($_.Realm))" }))] }
$targetAccount = (Read-Host "Account of the NEW character (Enter = $($account.Name))").Trim().ToUpper()
if (-not $targetAccount) { $targetAccount = $account.Name }
$newChar = (Read-Host 'Name of the NEW character, exactly as in the game').Trim()
if (-not $newChar) { Write-Host 'No name given.'; exit 1 }
foreach ($client in $clients) {
    $target = Join-Path $client.FullName "WTF\Account\$targetAccount\$($char.Realm)\$newChar"
    Copy-Folder $char.Path $target
    Write-Host "Done in $($client.Name): $newChar has the settings of $($char.Name)." -ForegroundColor Green
}
Write-Host 'Action bars are not included: which spell sits on which button is stored on the server, per character.'
