[CmdletBinding(DefaultParameterSetName='Local')]
param(
    [Parameter(Mandatory=$true)][string]$GameBuild,
    [ValidatePattern('^\d+\.\d+\.\d+$')][string]$Version = '0.1.10',
    [Parameter(Mandatory=$true, ParameterSetName='Signed')]
    [Parameter(Mandatory=$true, ParameterSetName='Local')]
    [ValidatePattern('^[a-fA-F0-9]{64}$')][string]$GameSha256,
    [Parameter(ParameterSetName='Local')][switch]$Direct,
    [Parameter(ParameterSetName='Local')][switch]$Integrated,
    [string]$FrameworkRoot,
    [string]$CrtDirectory,
    [string]$OutputDirectory,
    [Parameter(Mandatory=$true, ParameterSetName='Signed')]
    [ValidatePattern('^[a-fA-F0-9]{40}$')][string]$CertificateThumbprint,
    [Parameter(Mandatory=$true, ParameterSetName='Signed')]
    [ValidatePattern('^https?://')][string]$TimestampUrl
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot
$signed = $PSCmdlet.ParameterSetName -eq 'Signed'
if (!$OutputDirectory) { $OutputDirectory = Join-Path $root 'releases' }
if (!$FrameworkRoot) { $FrameworkRoot = Join-Path $root 'local/segagenesisrecomp' }
if (!$CrtDirectory) {
    $finder = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
    $vs = & $finder -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    $CrtDirectory = Join-Path $vs 'VC/Redist/MSVC/14.44.35112/x64/Microsoft.VC143.CRT'
}
$name = "Doom-Troopers-PC-Recomp-v$Version-Windows-x64"
if ($Direct -and $Integrated) { throw 'Select one package format.' }
if ($Integrated) { $name += '-Integrated' }
if ($Direct) { $name += '-Direct' }
if ($signed) { $name += '-signed' }
$output = (New-Item -ItemType Directory -Force $OutputDirectory).FullName
$zip = Join-Path $output "$name.zip"
if ((Test-Path -LiteralPath $zip) -or (Test-Path -LiteralPath "$zip.sha256")) { throw "Output already exists: $zip" }

function Sign-Binary($path) {
    & $signTool sign /sha1 $CertificateThumbprint /fd SHA256 /tr $TimestampUrl /td SHA256 $path
    if ($LASTEXITCODE) { throw "Signing failed: $path" }
    & $signTool verify /pa /all /tw $path
    $sig = Get-AuthenticodeSignature -LiteralPath $path
    if ($LASTEXITCODE -or $sig.Status -ne 'Valid' -or !$sig.TimeStamperCertificate -or
        $sig.SignerCertificate.Thumbprint -ne $CertificateThumbprint) { throw "Signature verification failed: $path" }
}
function Run-Portable($action, $first, $second) {
    & python (Join-Path $PSScriptRoot 'portable.py') $action $first $second
    if ($LASTEXITCODE) { throw "Portable package check failed: $action" }
}
$work = Join-Path $output ([Guid]::NewGuid().ToString())
$stage = (New-Item -ItemType Directory (Join-Path $work 'payload')).FullName
try {
    $game = Join-Path $stage 'DoomBootProbe.exe'
    Copy-Item -LiteralPath $GameBuild -Destination $game
    if ((Get-FileHash -LiteralPath $game).Hash -ne $GameSha256) { throw 'Game SHA-256 mismatch' }
    foreach ($dll in 'msvcp140.dll','vcruntime140.dll','vcruntime140_1.dll') {
        $path = Join-Path $stage $dll
        Copy-Item -LiteralPath (Join-Path $CrtDirectory $dll) -Destination $path
        $sig = Get-AuthenticodeSignature -LiteralPath $path
        if ($sig.Status -ne 'Valid' -or $sig.SignerCertificate.Subject -notmatch '(^|, )O=Microsoft Corporation(,|$)') {
            throw "Untrusted Microsoft runtime: $dll"
        }
    }
    Copy-Item -LiteralPath (Join-Path $FrameworkRoot 'runner/external/SDL2/SDL2-2.28.5/lib/x64/SDL2.dll') -Destination $stage
    Run-Portable 'notices' $FrameworkRoot $stage
    Copy-Item -LiteralPath (Join-Path $root 'recomplauncher.json') -Destination $stage
    if ($Integrated) {
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'integrated/LEIA-ME.txt') -Destination $stage
    } elseif ($Direct) {
        foreach ($file in 'Jogar.cmd','settings.ini','LEIA-ME.txt') {
            Copy-Item -LiteralPath (Join-Path $PSScriptRoot "direct/$file") -Destination $stage
        }
    } else {
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'LEIA-ME.txt') -Destination $stage
        if ($signed) {
            $signTool = (Get-Command signtool.exe -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
            Sign-Binary $game
        }
        $hashFile = Join-Path $work 'runtime.sha256'
        $runtime = @('DoomBootProbe.exe','SDL2.dll','msvcp140.dll','vcruntime140.dll','vcruntime140_1.dll')
        $runtime | ForEach-Object { "$((Get-FileHash -LiteralPath (Join-Path $stage $_)).Hash)  $_" } | Set-Content -LiteralPath $hashFile -Encoding ascii
        & "$env:WINDIR/Microsoft.NET/Framework64/v4.0.30319/csc.exe" /nologo /target:winexe /platform:x64 /optimize+ "/win32icon:$PSScriptRoot/trooper.ico" "/resource:$root/image/Screenshot_3.jpg,cover.jpg" "/resource:$hashFile,runtime.sha256" "/out:$stage/Launcher.exe" (Join-Path $PSScriptRoot 'Launcher.cs')
        if ($LASTEXITCODE) { throw 'Launcher build failed' }
        if ($signed) { Sign-Binary (Join-Path $stage 'Launcher.exe') }
    }
    $tmpZip = Join-Path $work 'portable.zip'
    $action = if ($Integrated) { 'archive-integrated' } elseif ($Direct) { 'archive-direct' } else { 'archive' }
    Run-Portable $action $stage $tmpZip
    $digest = (Get-FileHash -LiteralPath $tmpZip).Hash.ToLowerInvariant()
    "$digest  $name.zip" | Set-Content -LiteralPath "$work/digest" -Encoding ascii
    [IO.File]::Move($tmpZip, $zip)
    [IO.File]::Move("$work/digest", "$zip.sha256")
    Write-Output "$digest  $zip"
} finally {
    $resolved = (Resolve-Path -LiteralPath $work).Path
    if ((Split-Path $resolved) -eq $output -and (Split-Path $resolved -Leaf) -eq (Split-Path $work -Leaf)) {
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
}
