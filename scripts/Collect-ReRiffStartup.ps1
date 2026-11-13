param([string]$GameDir)

$ErrorActionPreference = 'Continue'
if (-not $GameDir) {
    $GameDir = $PSScriptRoot
    if ((Split-Path -Leaf $PSScriptRoot) -eq 'logs') {
        $GameDir = Split-Path -Parent $PSScriptRoot
    }
}
$GameDir = [IO.Path]::GetFullPath($GameDir)
$settings = Join-Path $GameDir 'server.ini'
$enabled = $false
if (Test-Path $settings) {
    $section = ''
    foreach ($line in (Get-Content $settings)) {
        $text = $line.Trim()
        if ($text.StartsWith('[')) { $section = $text }
        if ($section -eq '[diagnostics]' -and $text -match '^logging_enabled\s*=\s*(true|1|yes)\s*$') { $enabled = $true }
    }
}
if (-not $enabled) { throw 'Enable diagnostic logging in the launcher before collecting a startup report.' }
if (-not (Test-Path (Join-Path $GameDir 'Game.exe'))) {
    throw 'Game.exe was not found. Pass -GameDir with your Riff Racer folder.'
}
$logDir = Join-Path $GameDir 'logs'
New-Item -ItemType Directory -Force $logDir | Out-Null
$report = Join-Path $logDir ('ReRiff-startup-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.txt')
$lines = New-Object 'System.Collections.Generic.List[string]'
$lines.Add('ReRiff Windows startup report; collected ' + (Get-Date -Format o))
$lines.Add('Game folder: ' + $GameDir)
$lines.Add('')
$lines.Add('Files (missing helpers can explain an incomplete manual installation):')
foreach ($name in @('Game.exe','ParseOctane.dll','Parse.NetFx45.dll','ReRiff-Launcher.exe','ReRiff.Notify.dll','ReRiff.ClientConfig.dll','ReRiff.NewsHeaders.dll')) {
    $path = Join-Path $GameDir $name
    if (Test-Path $path) {
        $item = Get-Item $path
        $hash = (Get-FileHash $path -Algorithm SHA256).Hash
        $lines.Add(('{0}: {1} bytes; SHA256={2}' -f $name,$item.Length,$hash))
    } else { $lines.Add($name + ': MISSING') }
}
foreach ($name in @('logs\ReRiff-launcher.log','logs\ReRiff-client.log','logs\ReRiff-network.log','logs\ReRiff-engine-menu.log','localserver\install-state.json','localserver\client-channel.ini')) {
    $lines.Add(''); $lines.Add('--- ' + $name + ' ---')
    $path = Join-Path $GameDir $name
    if (Test-Path $path) {
        foreach ($line in (Get-Content $path -Tail 60)) { $lines.Add([string]$line) }
    } else { $lines.Add('MISSING') }
}
$lines.Add(''); $lines.Add('--- Running game ---')
foreach ($process in (Get-Process -Name Game -ErrorAction SilentlyContinue)) {
    $lines.Add(('PID={0}; Responding={1}; CPU={2}; Title={3}' -f $process.Id,$process.Responding,$process.CPU,$process.MainWindowTitle))
}
$lines.Add(''); $lines.Add('--- Display adapters ---')
foreach ($adapter in (Get-CimInstance Win32_VideoController -ErrorAction SilentlyContinue)) {
    $lines.Add(('{0}; driver={1}; processor={2}' -f $adapter.Name,$adapter.DriverVersion,$adapter.VideoProcessor))
}
$lines.Add(''); $lines.Add('--- Recent game/.NET application errors ---')
$events = Get-WinEvent -FilterHashtable @{LogName='Application'; StartTime=(Get-Date).AddDays(-1); Level=@(1,2)} -MaxEvents 100 -ErrorAction SilentlyContinue
foreach ($event in ($events | Where-Object { $_.Message -match 'Game\.exe|ReRiff' } | Select-Object -First 5)) {
    $lines.Add(('{0} {1} ID={2}' -f $event.TimeCreated,$event.ProviderName,$event.Id))
    $lines.Add([string]$event.Message)
}
$lines | Set-Content -LiteralPath $report -Encoding UTF8
Write-Host ('Report saved to ' + $report)
Write-Host 'No game files or Windows settings were changed.'
