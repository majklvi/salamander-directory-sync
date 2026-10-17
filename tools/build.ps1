#Requires -Version 7.3
# SPDX-License-Identifier: GPL-2.0-or-later
[CmdletBinding()]
param(
 [ValidateSet('Debug','Release')][string]$Configuration='Debug',
 [ValidateSet('Win32','x64','ARM64')][string]$Platform='x64',
 [ValidateSet('Plugin','Tests')][string]$Mode='Plugin',
 [string]$Project,
 [ValidateRange(30,900)][int]$BuildTimeoutSeconds=105
)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if(!(Test-Path -LiteralPath $vswhere)){throw 'Install Visual Studio 2026 C++ desktop tools and Windows SDK.'}
$installation=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$installation){throw 'Visual Studio C++ tools were not found.'}
$msbuild=Join-Path $installation 'MSBuild\Current\Bin\MSBuild.exe'
$projects=if($Mode -eq 'Plugin'){@('dirsync')}else{@('dirsync_tests','dirsync_copy_tests','dirsync_lifecycle_tests','dirsync_ui_tests','dirsync_ui_czech_tests')}
if($Project){if($Project -notin ($projects+@('dirsync_lifecycle_probe','lang_dirsync','lang_dirsync_czech'))){throw 'Unknown project.'};$projects=@($Project)}
if($Mode -eq 'Tests' -and $Platform -ne 'x64'){throw 'The standard runtime test suite is x64 only.'}
$logs=Join-Path $repo "out\logs\${Configuration}_${Platform}"
[void][IO.Directory]::CreateDirectory($logs)
foreach($name in $projects){
 $arguments=@((Join-Path $repo "build-support\$name.vcxproj"),'/m','/nr:false','/nologo','/noconsolelogger',"/p:Configuration=$Configuration","/p:Platform=$Platform",'/p:TreatWarningAsError=true',"/bl:$logs\$name.binlog",'/fl',"/flp:logfile=$logs\$name.log;verbosity=normal;encoding=UTF-8")
 $start=[Diagnostics.ProcessStartInfo]::new($msbuild);$start.WorkingDirectory=$repo;$start.UseShellExecute=$false;$start.CreateNoWindow=$true
 $start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
 foreach($argument in $arguments){[void]$start.ArgumentList.Add($argument)}
 $process=[Diagnostics.Process]::Start($start)
 $stdout=$process.StandardOutput.ReadToEndAsync();$stderr=$process.StandardError.ReadToEndAsync()
 if(!$process.WaitForExit($BuildTimeoutSeconds * 1000)){$process.Kill($true);throw "Build timed out: $name"}
 $stdout.GetAwaiter().GetResult() | Write-Output
 $errors=$stderr.GetAwaiter().GetResult();if($errors){Write-Output $errors}
 if($process.ExitCode -ne 0){[IO.File]::ReadAllText((Join-Path $logs "$name.log"),[Text.Encoding]::UTF8) | Write-Output;throw "Build failed: $name ($($process.ExitCode))"}
 Write-Output "Build PASS: $name ($Configuration|$Platform); UTF-8 log saved."
}
