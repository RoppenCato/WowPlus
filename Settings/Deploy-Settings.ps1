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
    foreach ($line in [IO.File]::ReadAllLines((Join-Path $PSScriptRoot $name))) {
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
& (Join-Path $root 'Tools\mysql\bin\mysql.exe') "--defaults-file=$(Join-Path $PSScriptRoot 'mysql.ini')" -e "UPDATE acore_auth.realmlist SET flag = flag & ~3" 2>&1 | Out-Null
exit 0
