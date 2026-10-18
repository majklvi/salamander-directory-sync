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
 if($version.FileMajorPart -ne 0 -or $version.FileMinorPart -ne 2 -or $version.FileBuildPart -ne 0){throw "Expected 0.2.0: $relative"}
}
$payload=Join-Path $output 'payload\dirsync'
[void][IO.Directory]::CreateDirectory((Join-Path $payload 'lang'))
[void][IO.Directory]::CreateDirectory((Join-Path $payload 'licenses'))
foreach($relative in $inputs){Copy-Item -LiteralPath (Join-Path $binary $relative) -Destination (Join-Path $payload $relative)}
$revision=& git -C $repo rev-parse HEAD
if($LASTEXITCODE -ne 0){throw 'Source revision unavailable.'}
$guides=@(
 @{source='docs/directory-sync-plugin.en.md';target='README.md'},
 @{source='docs/directory-sync-plugin-pilot-guide.en.md';target='PILOT.md'},
 @{source='docs/directory-sync-plugin.md';target='README.cs.md'},
 @{source='docs/directory-sync-plugin-pilot-guide.md';target='PILOT.cs.md'}
)
$guideTargets=@{}
foreach($guide in $guides){$guideTargets[[IO.Path]::GetFullPath((Join-Path $repo $guide.source))]=$guide.target}
function Read-PackageGuide([string]$source){
 $sourcePath=[IO.Path]::GetFullPath((Join-Path $repo $source))
 $document=[IO.File]::ReadAllText($sourcePath)
 if($document -notmatch '\[!CAUTION\]' -or $document -notmatch 'ALPHA|ALFA'){throw "Missing alpha warning: $source"}
 # Keep links between the four user guides local. Other repository documents
 # remain online, pinned to this package's source commit instead of bundled.
 return [regex]::Replace($document,'\]\(([^)\r\n]+)\)',[Text.RegularExpressions.MatchEvaluator]{
  param($match)
  $link=$match.Groups[1].Value
  if($link -match '^(?:[a-z][a-z0-9+.-]*:|#|/)'){return $match.Value}
  $fragment='';$file=$link;$hashIndex=$link.IndexOf('#')
  if($hashIndex -ge 0){$file=$link.Substring(0,$hashIndex);$fragment=$link.Substring($hashIndex)}
  $resolved=[IO.Path]::GetFullPath((Join-Path ([IO.Path]::GetDirectoryName($sourcePath)) $file))
  if(!(Test-Path -LiteralPath $resolved -PathType Leaf)){throw "Missing documentation link: $source -> $link"}
  if($guideTargets.ContainsKey($resolved)){$target=$guideTargets[$resolved]}
  else{
   $relative=[IO.Path]::GetRelativePath($repo,$resolved).Replace('\','/')
   if($relative -eq '..' -or $relative.StartsWith('../')){throw "Documentation link outside repository: $link"}
   $target="https://github.com/majklvi/salamander-directory-sync/blob/$revision/$relative"
  }
  return "]($target$fragment)"
 })
}
foreach($guide in $guides){
 [IO.File]::WriteAllText((Join-Path $payload $guide.target),(Read-PackageGuide $guide.source),[Text.UTF8Encoding]::new($false))
}
Copy-Item -LiteralPath (Join-Path $repo 'LICENSE') -Destination (Join-Path $payload 'LICENSE')
$dark=Join-Path $repo 'vendor\samandarin-ui\third_party\darkmodelib'
Copy-Item -LiteralPath (Join-Path $dark 'LICENSE.md') -Destination (Join-Path $payload 'licenses\darkmodelib-MPL-2.0.md')
Copy-Item -LiteralPath (Join-Path $dark 'LICENSE-MIT.md') -Destination (Join-Path $payload 'licenses\darkmodelib-MIT.md')
Get-ChildItem -LiteralPath (Join-Path $dark 'docs') -Filter 'LICENSE-*.md' -File | ForEach-Object{Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $payload ('licenses\'+$_.Name))}
$notice=@"
Directory Synchronization 0.2.0 ALPHA, x64; for Samandarin.
ALPHA SOFTWARE -- RISK OF DATA LOSS. NOT FOR PRODUCTION USE.
NO WARRANTY. Authors and contributors accept NO RESPONSIBILITY OR LIABILITY
FOR ANY LOSS OR CORRUPTION OF DATA. Use only on disposable test copies with
an independent verified backup. Keep originals does not provide rollback.
ALFA VERZE -- NEBEZPECI ZTRATY DAT. BEZ ZARUKY A ODPOVEDNOSTI ZA ZTRATU DAT.
Copyright (c) 2026 Michal Várady; Open Salamander Authors. GPL-2.0-or-later.
Source revision: $revision
Source: https://github.com/majklvi/salamander-directory-sync/tree/$revision
Includes darkmodelib by ozone10; licenses are in licenses/.
Extract dirsync/ into the host plugins/ directory, then register dirsync.spl
using Plugins Manager > Add. English and Czech modules are in lang/.
No compilation is needed to install this package.
English: read PILOT.md before testing and README.md for the full guide.
Cesky: PILOT.cs.md a README.cs.md. Development records remain on GitHub.
Unsigned local build; no catalog release or installer registration changes.
"@
[IO.File]::WriteAllText((Join-Path $payload 'NOTICE.txt'),$notice,[Text.UTF8Encoding]::new($false))
$manifest=@(Get-ChildItem -LiteralPath $payload -File -Recurse | Sort-Object FullName | ForEach-Object{
 [ordered]@{path=$_.FullName.Substring($payload.Length+1).Replace('\','/');sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}
})
[IO.File]::WriteAllText((Join-Path $payload 'files.json'),($manifest|ConvertTo-Json),[Text.UTF8Encoding]::new($false))
$archive=Join-Path $output 'dirsync-0.2.0-x64.zip'
Compress-Archive -LiteralPath $payload -DestinationPath $archive
$extracted=Join-Path $output 'verified-extraction'
Expand-Archive -LiteralPath $archive -DestinationPath $extracted
$unpacked=Join-Path $extracted 'dirsync'
foreach($entry in $manifest){
 $hash=(Get-FileHash -LiteralPath (Join-Path $unpacked $entry.path)).Hash.ToLowerInvariant()
 if($hash -ne $entry.sha256){throw "Extracted hash mismatch: $($entry.path)"}
}
# Verify the installation archive contains only the intended user documentation.
if(Test-Path -LiteralPath (Join-Path $unpacked 'docs')){throw 'Development docs must not be packaged.'}
$unpackedFiles=@(Get-ChildItem -LiteralPath $unpacked -File -Recurse)
if($unpackedFiles.Count -ne $manifest.Count+1){throw 'Extracted file inventory mismatch.'}
foreach($guide in $guides){
 $path=Join-Path $unpacked $guide.target
 $text=[IO.File]::ReadAllText($path)
 foreach($match in [regex]::Matches($text,'\]\(([^)\r\n]+)\)')){
  $link=$match.Groups[1].Value
  if($link -match '^(?:[a-z][a-z0-9+.-]*:|#|/)'){continue}
  $file=$link.Split('#')[0]
  if(!(Test-Path -LiteralPath (Join-Path $unpacked $file) -PathType Leaf)){throw "Broken packaged guide link: $($guide.target) -> $link"}
 }
}
$zipHash=(Get-FileHash -LiteralPath $archive).Hash.ToLowerInvariant()
[IO.File]::WriteAllText((Join-Path $output 'package-sha256.txt'),"$zipHash  dirsync-0.2.0-x64.zip`n",[Text.UTF8Encoding]::new($false))
Write-Output "Verified $($manifest.Count) packaged files; $archive; SHA256 $zipHash"
