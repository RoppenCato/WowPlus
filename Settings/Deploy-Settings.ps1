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
foreach ($db in 'world', 'characters', 'auth') {
    foreach ($sql in Get-ChildItem (Join-Path $PSScriptRoot "SQL\$db") -Filter *.sql -ErrorAction SilentlyContinue | Sort-Object Name) {
        Get-Content -LiteralPath $sql.FullName -Raw | & $mysql $myini "acore_$db" 2>&1 | Out-Null
        if ($LASTEXITCODE -eq 0) { Write-Host "Applied SQL\$db\$($sql.Name)" }
        else { Write-Warning "SQL\$db\$($sql.Name) failed. Run it by hand to see the error." }
    }
}

# Our own addons (Client\AddOns) are copied into every game copy, so the game always has the
# version that matches the server. A running game picks them up after /reload or a restart.
foreach ($client in Get-ChildItem (Join-Path $root 'Client') -Directory -ErrorAction SilentlyContinue | Where-Object { Test-Path (Join-Path $_.FullName 'Wow.exe') }) {
    foreach ($addon in Get-ChildItem (Join-Path $root 'Client\AddOns') -Directory -ErrorAction SilentlyContinue) {
        $to = Join-Path $client.FullName "Interface\AddOns\$($addon.Name)"
        New-Item -ItemType Directory -Force $to | Out-Null
        Copy-Item (Join-Path $addon.FullName '*') $to -Recurse -Force
    }
}

& (Join-Path $root 'Tools\mysql\bin\mysql.exe') "--defaults-file=$(Join-Path $PSScriptRoot 'mysql.ini')" -e "UPDATE acore_auth.realmlist SET flag = flag & ~3" 2>&1 | Out-Null
exit 0
