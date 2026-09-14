#Requires -Version 5.1
# Runs without installers, Qt, VS Code, Pester, or administrator access.
$ErrorActionPreference = 'Stop'

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw "Assertion failed: $Message" }
}

function Assert-Throws {
    param([scriptblock]$Action, [string]$Pattern)
    $message = $null
    try { & $Action } catch { $message = $_.Exception.Message }
    Assert-True ($message -like $Pattern) "Expected '$Pattern', received '$message'"
}

$originalEnvironment = @{}
foreach ($name in @('OS', 'PROCESSOR_ARCHITECTURE', 'ProgramFiles', 'LOCALAPPDATA', 'PATH')) {
    $originalEnvironment[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
}
$testRoot = Join-Path ([IO.Path]::GetTempPath()) ('MFG setup test ' + [Guid]::NewGuid().ToString('N'))
$sourceScript = Join-Path (Split-Path $PSScriptRoot) 'scripts/setup-windows.ps1'
$copiedScript = Join-Path $testRoot 'cpp/scripts/setup-windows.ps1'

try {
    New-Item -ItemType Directory -Path (Split-Path $copiedScript) -Force | Out-Null
    Copy-Item -LiteralPath $sourceScript -Destination $copiedScript
    . $copiedScript

    # Exercise the real native-command wrapper, including its nonzero exit check.
    $shellPath = Join-Path $PSHOME 'powershell.exe'
    if (-not (Test-Path -LiteralPath $shellPath)) { $shellPath = Join-Path $PSHOME 'pwsh' }
    Assert-Throws { Invoke-Checked $shellPath @('-NoProfile', '-Command', 'exit 7') } '*failed (exit 7)*'
    # GitHub's PowerShell wrapper checks this value after the script returns.
    $global:LASTEXITCODE = 0

    $env:OS = 'Windows_NT'
    $env:PROCESSOR_ARCHITECTURE = 'AMD64'
    $env:ProgramFiles = Join-Path $testRoot 'Program Files'
    $env:LOCALAPPDATA = Join-Path $testRoot 'Local App Data'
    $QtRoot = Join-Path $testRoot 'Qt kit'
    foreach ($relative in @('lib/cmake/Qt6/Qt6Config.cmake', 'lib/cmake/Qt6Widgets/Qt6WidgetsConfig.cmake',
        'lib/cmake/Qt6Test/Qt6TestConfig.cmake', 'lib/Qt6Core.lib', 'bin/Qt6Core.dll', 'bin/windeployqt.exe')) {
        $file = Join-Path $QtRoot $relative
        New-Item -ItemType Directory -Path (Split-Path $file) -Force | Out-Null
        New-Item -ItemType File -Path $file -Force | Out-Null
    }
    $script:cmakePath = Join-Path $testRoot 'CMake tools/cmake.exe'
    $script:vsPath = Join-Path $testRoot 'Visual Studio'
    $script:dotPath = Join-Path $testRoot 'Graphviz tools/dot.exe'
    $script:calls = New-Object 'System.Collections.Generic.List[object]'
    $script:lookups = New-Object 'System.Collections.Generic.List[string]'
    $script:missingCMake = $false
    $script:failBuild = $false
    $script:missingDot = $false

    # Mock only external discovery/invocation; retain setup orchestration and file output.
    function Update-ProcessPath { }
    function Get-VisualStudio { return $script:vsPath }
    function Find-CMake {
        param([string[]]$Candidates)
        if (-not $script:missingCMake) { return $script:cmakePath }
    }
    function Find-Program {
        param([string]$Name, [string[]]$Candidates)
        $script:lookups.Add($Name)
        if ($Name -eq 'dot.exe' -and -not $script:missingDot) { return $script:dotPath }
        if ($Name -eq 'code.cmd') { return (Join-Path $testRoot 'VS Code/code.cmd') }
        if ($Name -eq 'winget.exe') { return (Join-Path $testRoot 'winget.exe') }
    }
    function Invoke-Checked {
        param([string]$File, [string[]]$Arguments)
        $script:calls.Add([pscustomobject]@{ File = $File; Arguments = @($Arguments) })
        if ($script:failBuild -and $Arguments[0] -eq '--build') { throw 'Simulated build failure' }
    }

    $workspacePath = Write-MfgWorkspace $testRoot $QtRoot $cmakePath $vsPath 'RelWithDebInfo' $dotPath
    $workspace = Get-Content -LiteralPath $workspacePath -Raw | ConvertFrom-Json
    $launch = $workspace.launch.configurations[0]
    $task = $workspace.tasks.tasks[0]
    Assert-True ($launch.type -eq 'cppvsdbg') 'F5 must use the MSVC debugger'
    Assert-True ($launch.program -eq (Join-Path $testRoot 'cpp/build-windows/RelWithDebInfo/MatrixFillingGame.exe')) 'F5 GUI path'
    Assert-True ($launch.preLaunchTask -eq $task.label -and $task.type -eq 'process') 'F5 build task binding'
    Assert-True ($task.args -contains $copiedScript -and $task.args -contains '-BuildOnly') 'Task keeps the script path with spaces as one argument'
    $runtimePath = $launch.environment[0].value
    Assert-True ($runtimePath.StartsWith((Join-Path $QtRoot 'bin') + ';')) 'Qt DLL lookup path'
    Assert-True ($runtimePath.EndsWith('${env:PATH}')) 'Existing runtime PATH preserved'
    Remove-Item -LiteralPath $workspacePath

    $NoInstall = $true
    $missingCMake = $true
    Assert-Throws { Start-MfgSetup } 'Missing dependency: Kitware.CMake*'
    Assert-True ($calls.Count -eq 0 -and -not ($lookups -contains 'winget.exe')) 'NoInstall fails before invoking an installer'

    $NoInstall = $false
    $missingCMake = $false
    $missingDot = $true
    $BuildOnly = $true
    Start-MfgSetup
    Assert-True ($calls.Count -eq 2) 'BuildOnly performs configure/build only'
    Assert-True ($calls[0].Arguments[0] -eq '-S' -and $calls[1].Arguments[0] -eq '--build') 'BuildOnly command order'
    Assert-True ($calls[0].Arguments[1] -eq (Join-Path $testRoot 'cpp')) 'Configure preserves the source path with spaces'
    Assert-True (-not ($lookups -contains 'winget.exe') -and -not ($lookups -contains 'code.cmd')) 'BuildOnly does not install or open the editor'
    Assert-True (-not (Test-Path -LiteralPath $workspacePath)) 'BuildOnly does not rewrite the workspace'

    $calls.Clear()
    $lookups.Clear()
    $BuildOnly = $false
    $missingDot = $false
    $failBuild = $true
    Assert-Throws { Start-MfgSetup } 'Simulated build failure'
    Assert-True ($calls.Count -eq 2 -and -not ($lookups -contains 'code.cmd')) 'Failed build stops before tests, deployment, and editor launch'
    Assert-True (-not (Test-Path -LiteralPath $workspacePath)) 'Failed build does not publish a completed workspace'
    Write-Host 'Windows setup regression checks passed.'
} finally {
    foreach ($name in $originalEnvironment.Keys) {
        [Environment]::SetEnvironmentVariable($name, $originalEnvironment[$name], 'Process')
    }
    if (Test-Path -LiteralPath $testRoot) { Remove-Item -LiteralPath $testRoot -Recurse -Force }
}
