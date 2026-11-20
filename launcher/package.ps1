param([Parameter(Mandatory=$true)][string]$GameBuild)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot
$folder = Join-Path $root 'releases/Doom-Troopers-PC-Recomp-v0.1.4-win64'
New-Item -ItemType Directory -Force $folder | Out-Null
$previous = Join-Path $root 'releases/Doom-Troopers-PC-Recomp-v0.1.1-win64.zip'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [IO.Compression.ZipFile]::OpenRead($previous)
$files = @('SDL2.dll','msvcp140.dll','vcruntime140.dll','vcruntime140_1.dll',
           'FRAMEWORK-LICENSE.md','SDL2-LICENSE.txt','THIRD-PARTY-LICENSES.md')
try {
    foreach ($name in $files) {
        $entry = $archive.GetEntry("Doom-Troopers-PC-Recomp-v0.1.1-win64/$name")
        [IO.Compression.ZipFileExtensions]::ExtractToFile($entry, (Join-Path $folder $name), $true)
    }
} finally { $archive.Dispose() }
Copy-Item -LiteralPath $GameBuild -Destination (Join-Path $folder 'DoomBootProbe.exe')
& "$env:WINDIR/Microsoft.NET/Framework64/v4.0.30319/csc.exe" /nologo /target:winexe /platform:x64 /optimize+ "/win32icon:$PSScriptRoot/trooper.ico" "/resource:$root/image/Screenshot_3.jpg,cover.jpg" "/out:$folder/Launcher.exe" (Join-Path $PSScriptRoot 'Launcher.cs')
if ($LASTEXITCODE) { throw 'Falha ao compilar o launcher.' }
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Jogar.bat'),(Join-Path $PSScriptRoot 'LEIA-ME.txt') -Destination $folder
$zipPath = "$folder.zip"
$tempZip = "$zipPath.$([Guid]::NewGuid()).tmp"
$package = [IO.Compression.ZipFile]::Open($tempZip, 'Create')
try {
    foreach ($name in ($files + @('Launcher.exe','DoomBootProbe.exe','Jogar.bat','LEIA-ME.txt'))) {
        $null = [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($package, (Join-Path $folder $name), "$(Split-Path $folder -Leaf)/$name")
    }
} finally { $package.Dispose() }
Move-Item -LiteralPath $tempZip -Destination $zipPath -Force
