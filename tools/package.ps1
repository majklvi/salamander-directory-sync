#Requires -Version 7.3
# SPDX-License-Identifier: GPL-2.0-or-later
[CmdletBinding()]
param([Parameter(Mandatory)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$output=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $output){throw 'Output directory must be new.'}
$binary=Join-Path $repo 'out\bin\Release_x64\dirsync'
$inputs=@('dirsync.spl','lang\english.slg','lang\czech.slg')
foreach($relative in $inputs){
 $path=Join-Path $binary $relative
 $stream=[IO.File]::OpenRead($path);$reader=[IO.BinaryReader]::new($stream)
 try{
  if($reader.ReadUInt16() -ne 0x5a4d){throw "Not PE: $relative"}
  $stream.Position=0x3c;$offset=$reader.ReadInt32();$stream.Position=$offset
  if($reader.ReadUInt32() -ne 0x4550 -or $reader.ReadUInt16() -ne 0x8664){throw "Expected x64: $relative"}
 }finally{$reader.Dispose()}
 $version=[Diagnostics.FileVersionInfo]::GetVersionInfo($path)
 if($version.FileMajorPart -ne 0 -or $version.FileMinorPart -ne 1 -or $version.FileBuildPart -ne 4){throw "Expected 0.1.4: $relative"}
}
$payload=Join-Path $output 'payload\dirsync'
[void][IO.Directory]::CreateDirectory((Join-Path $payload 'lang'))
[void][IO.Directory]::CreateDirectory((Join-Path $payload 'licenses'))
foreach($relative in $inputs){Copy-Item -LiteralPath (Join-Path $binary $relative) -Destination (Join-Path $payload $relative)}
Copy-Item -LiteralPath (Join-Path $repo 'docs') -Destination (Join-Path $payload 'docs') -Recurse
$guide=[IO.File]::ReadAllText((Join-Path $repo 'docs\directory-sync-plugin.md'))
$guide=[regex]::Replace($guide,'\]\((directory-sync-plugin[^)]*)\)','](docs/$1)')
$guide=$guide.Replace('](docs/directory-sync-plugin-pilot-guide.md)','](PILOT.cs.md)')
[IO.File]::WriteAllText((Join-Path $payload 'README.cs.md'),$guide,[Text.UTF8Encoding]::new($false))
$pilot=[IO.File]::ReadAllText((Join-Path $repo 'docs\directory-sync-plugin-pilot-guide.md'))
$pilot=[regex]::Replace($pilot,'\]\((directory-sync-plugin[^)]*)\)','](docs/$1)')
$pilot=$pilot.Replace('](docs/directory-sync-plugin.md)','](README.cs.md)')
[IO.File]::WriteAllText((Join-Path $payload 'PILOT.cs.md'),$pilot,[Text.UTF8Encoding]::new($false))
Copy-Item -LiteralPath (Join-Path $repo 'LICENSE') -Destination (Join-Path $payload 'LICENSE')
$dark=Join-Path $repo 'vendor\samandarin-ui\third_party\darkmodelib'
Copy-Item -LiteralPath (Join-Path $dark 'LICENSE.md') -Destination (Join-Path $payload 'licenses\darkmodelib-MPL-2.0.md')
Copy-Item -LiteralPath (Join-Path $dark 'LICENSE-MIT.md') -Destination (Join-Path $payload 'licenses\darkmodelib-MIT.md')
Get-ChildItem -LiteralPath (Join-Path $dark 'docs') -Filter 'LICENSE-*.md' -File | ForEach-Object{Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $payload ('licenses\'+$_.Name))}
$revision=& git -C $repo rev-parse HEAD
if($LASTEXITCODE -ne 0){throw 'Source revision unavailable.'}
$notice=@"
Directory Synchronization 0.1.4 local standalone pilot, x64; Samandarin 0.16 SDK.
Copyright (c) 2026 Open Salamander Authors. GPL-2.0-or-later.
Source revision: $revision
This local repository is not published. No new repository URL is claimed.
Includes darkmodelib by ozone10; licenses are in licenses/.
Extract dirsync/ into the host plugins/ directory, then register dirsync.spl
using Plugins Manager > Add. English and Czech modules are in lang/.
Read PILOT.cs.md before testing, README.cs.md for usage, docs/ for records.
Unsigned local build; no catalog release or installer registration changes.
"@
[IO.File]::WriteAllText((Join-Path $payload 'NOTICE.txt'),$notice,[Text.UTF8Encoding]::new($false))
$manifest=@(Get-ChildItem -LiteralPath $payload -File -Recurse | Sort-Object FullName | ForEach-Object{
 [ordered]@{path=$_.FullName.Substring($payload.Length+1).Replace('\','/');sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}
})
[IO.File]::WriteAllText((Join-Path $payload 'files.json'),($manifest|ConvertTo-Json),[Text.UTF8Encoding]::new($false))
$archive=Join-Path $output 'dirsync-0.1.4-x64.zip'
Compress-Archive -LiteralPath $payload -DestinationPath $archive
$extracted=Join-Path $output 'verified-extraction'
Expand-Archive -LiteralPath $archive -DestinationPath $extracted
$unpacked=Join-Path $extracted 'dirsync'
foreach($entry in $manifest){
 $hash=(Get-FileHash -LiteralPath (Join-Path $unpacked $entry.path)).Hash.ToLowerInvariant()
 if($hash -ne $entry.sha256){throw "Extracted hash mismatch: $($entry.path)"}
}
$zipHash=(Get-FileHash -LiteralPath $archive).Hash.ToLowerInvariant()
[IO.File]::WriteAllText((Join-Path $output 'package-sha256.txt'),"$zipHash  dirsync-0.1.4-x64.zip`n",[Text.UTF8Encoding]::new($false))
Write-Output "Verified $($manifest.Count) packaged files; $archive; SHA256 $zipHash"
