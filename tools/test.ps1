#Requires -Version 7.3
# SPDX-License-Identifier: GPL-2.0-or-later
[CmdletBinding()]
param([ValidateSet('Debug','Release')][string]$Configuration='Debug')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$names=@('dirsync_tests','dirsync_copy_tests','dirsync_lifecycle_tests','dirsync_ui_tests','dirsync_ui_czech_tests')
$results=@()
foreach($name in $names){
 $exe=Join-Path $repo "out\tests\${Configuration}_x64\$name.exe"
 if(!(Test-Path -LiteralPath $exe)){throw "Build test first: $name"}
 $start=[Diagnostics.ProcessStartInfo]::new($exe);$start.WorkingDirectory=[IO.Path]::GetDirectoryName($exe)
 $start.UseShellExecute=$false;$start.CreateNoWindow=$true;$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
 $process=[Diagnostics.Process]::Start($start);$stdout=$process.StandardOutput.ReadToEndAsync();$stderr=$process.StandardError.ReadToEndAsync()
 if(!$process.WaitForExit(90000)){$process.Kill($true);throw "Test timed out: $name"}
 $text=$stdout.GetAwaiter().GetResult()+$stderr.GetAwaiter().GetResult()
 Write-Output "$name : $text"
 $results+=@{name=$name;exitCode=$process.ExitCode;output=$text}
 if($process.ExitCode -ne 0){throw "Test failed: $name"}
}
$output=Join-Path $repo "out\test-results-$Configuration.json"
[IO.File]::WriteAllText($output,($results|ConvertTo-Json -Depth 5),[Text.UTF8Encoding]::new($false))
