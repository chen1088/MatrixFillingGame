#Requires -Version 5.1
<#
.SYNOPSIS
Install Windows dependencies, build the desktop app, and open VS Code.
.DESCRIPTION
Run as your normal Windows user. Individual installers may request elevation.
Qt and the Python virtual environment stay in cpp/.tools; system dependencies
are installed only when missing. NoInstall forbids package installations.
BuildOnly configures/builds using existing dependencies, for the VS Code task.
#>
[CmdletBinding()]
param(
    [string]$QtRoot,
    [ValidateSet('Debug', 'Release', 'RelWithDebInfo')]
    [string]$Configuration = 'RelWithDebInfo',
    [switch]$NoInstall,
    [switch]$SkipEditor,
    [switch]$SkipTests,
    [switch]$BuildOnly
)

function Invoke-Checked {
    param([string]$File, [string[]]$Arguments)
    & $File @Arguments | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "$File failed (exit $LASTEXITCODE). Fix the error above and rerun setup."
    }
}

function Update-ProcessPath {
    # Retain the caller's environment (e.g. Developer PowerShell), adding newly
    # installed paths only to this process. Do not rewrite the user's PATH.
    $env:PATH = @(
        $env:PATH
        [Environment]::GetEnvironmentVariable('Path', 'Machine')
        [Environment]::GetEnvironmentVariable('Path', 'User')
    ) -join ';'
}

function Find-Program {
    param([string]$Name, [string[]]$Candidates = @())
    $command = Get-Command $Name -CommandType Application -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($command -and $command.Source -notlike '*\Microsoft\WindowsApps\python*.exe') {
        return $command.Source
    }
    foreach ($candidate in $Candidates) {
        if ($candidate -and (Test-Path -LiteralPath $candidate -PathType Leaf)) {
            return $candidate
        }
    }
    return $null
}

function Find-CMake {
    param([string[]]$Candidates)
    $onPath = Find-Program 'cmake.exe'
    foreach ($candidate in @($onPath) + $Candidates) {
        if (-not $candidate -or -not (Test-Path -LiteralPath $candidate -PathType Leaf)) { continue }
        $versionText = @(& $candidate --version)
        if ($LASTEXITCODE -eq 0 -and $versionText.Count -and
            $versionText[0] -match 'cmake version (\d+\.\d+\.\d+)' -and
            [version]$Matches[1] -ge [version]'3.21.0') {
            return $candidate
        }
    }
    return $null
}

function Install-WingetPackage {
    param([string]$Id, [string[]]$Extra = @())
    if ($NoInstall -or $BuildOnly) {
        throw "Missing dependency: $Id. Run setup-windows.cmd without -NoInstall/-BuildOnly."
    }
    $winget = Find-Program 'winget.exe'
    if (-not $winget) {
        throw 'Windows App Installer (winget) is missing. Install/update App Installer from Microsoft Store, then rerun setup-windows.cmd.'
    }
    Write-Host "Installing $Id ..."
    Invoke-Checked $winget (@('install', '--id', $Id, '--exact', '--source', 'winget',
        '--accept-package-agreements', '--accept-source-agreements', '--disable-interactivity') + $Extra)
    Update-ProcessPath
}

function Find-VisualStudio {
    param([switch]$WithCompiler)
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) { return $null }
    $query = @('-latest', '-products', '*', '-version', '[17.0,18.0)', '-property', 'installationPath')
    if ($WithCompiler) { $query += @('-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64') }
    $result = @(& $vswhere @query)
    if ($LASTEXITCODE -ne 0) { throw 'vswhere could not inspect the Visual Studio installation.' }
    if ($result.Count) { return $result[0].Trim() }
    return $null
}

function Get-VisualStudio {
    $installation = Find-VisualStudio -WithCompiler
    $sdkRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits/10'
    $sdkHeaders = @(Get-ChildItem "$sdkRoot/Include/*/um/Windows.h" -ErrorAction SilentlyContinue)
    $sdkLibraries = @(Get-ChildItem "$sdkRoot/Lib/*/um/x64/kernel32.lib" -ErrorAction SilentlyContinue)
    if ($installation -and $sdkHeaders.Count -and $sdkLibraries.Count) { return $installation }
    if ($NoInstall -or $BuildOnly) { throw 'Visual Studio 2022 C++ build tools are missing. Run setup-windows.cmd.' }
    $existing = Find-VisualStudio
    if ($existing) {
        Write-Host 'Adding C++ tools to the existing Visual Studio 2022 installation ...'
        $installer = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\setup.exe'
        # Both the BuildTools and full IDE products accept the individual C++
        # components. Use their respective workload IDs to include a Windows SDK.
        $workload = 'Microsoft.VisualStudio.Workload.NativeDesktop'
        $vswhere = Join-Path (Split-Path $installer) 'vswhere.exe'
        $buildTools = @(& $vswhere -products Microsoft.VisualStudio.Product.BuildTools `
            -version '[17.0,18.0)' -property installationPath)
        if ($existing -in $buildTools) { $workload = 'Microsoft.VisualStudio.Workload.VCTools' }
        # Installed setup.exe does not accept the bootstrapper's --wait flag.
        $arguments = "modify --installPath `"$existing`" --add $workload --includeRecommended --passive --norestart"
        $process = Start-Process -FilePath $installer -ArgumentList $arguments -Verb RunAs -Wait -PassThru
        if ($process.ExitCode -eq 3010) { throw 'Visual Studio needs a reboot. Restart Windows, then rerun setup-windows.cmd.' }
        if ($process.ExitCode -ne 0) { throw "Visual Studio modification failed (exit $($process.ExitCode))." }
    } else {
        Install-WingetPackage 'Microsoft.VisualStudio.2022.BuildTools' @('--override',
            '--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --norestart')
    }
    $installation = Find-VisualStudio -WithCompiler
    if (-not $installation) { throw 'C++ tools are not ready. Restart Windows if requested by the installer, then rerun setup-windows.cmd.' }
    return $installation
}

function Find-Python {
    $candidates = @()
    foreach ($version in @('312', '313', '311', '310', '39')) {
        $candidates += Join-Path $env:LOCALAPPDATA "Programs/Python/Python$version/python.exe"
        $candidates += Join-Path $env:ProgramFiles "Python$version/python.exe"
    }
    # Do not invoke Windows Store aliases. Reuse a compatible real Python if present.
    $onPath = Find-Program 'python.exe'
    if ($onPath) { $candidates += $onPath }
    foreach ($candidate in $candidates) {
        if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) { continue }
        & $candidate -c 'import sys; sys.exit(0 if (3,9) <= sys.version_info[:2] < (3,14) and sys.maxsize > 2**32 else 1)' 2>$null
        if ($LASTEXITCODE -eq 0) { return $candidate }
    }
    return $null
}

function Test-QtKit {
    param([string]$Root)
    if (-not $Root) { return $false }
    foreach ($file in @('lib/cmake/Qt6/Qt6Config.cmake', 'lib/cmake/Qt6Widgets/Qt6WidgetsConfig.cmake',
        'lib/cmake/Qt6Test/Qt6TestConfig.cmake', 'lib/Qt6Core.lib', 'bin/Qt6Core.dll', 'bin/windeployqt.exe')) {
        if (-not (Test-Path -LiteralPath (Join-Path $Root $file) -PathType Leaf)) { return $false }
    }
    return $true
}

function Write-MfgWorkspace {
    param([string]$Repo, [string]$Kit, [string]$CMake, [string]$VS, [string]$Config, [string]$Dot)
    $build = Join-Path $Repo 'cpp/build-windows'
    $scriptFile = Join-Path $Repo 'cpp/scripts/setup-windows.ps1'
    $taskArgs = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $scriptFile,
        '-BuildOnly', '-SkipEditor', '-Configuration', $Config)
    $runtimePath = (Join-Path $Kit 'bin') + ';'
    if ($Dot) { $runtimePath += (Split-Path $Dot) + ';' }
    $runtimePath += '${env:PATH}'
    $workspace = [ordered]@{
        folders = @(@{ path = '.' })
        settings = [ordered]@{
            'cmake.sourceDirectory' = (Join-Path $Repo 'cpp')
            'cmake.buildDirectory' = $build
            'cmake.cmakePath' = $CMake
            'cmake.generator' = 'Visual Studio 17 2022'
            'cmake.platform' = 'x64'
            'cmake.useCMakePresets' = 'never'
            'cmake.configureOnOpen' = $false
            'cmake.configureSettings' = @{
                CMAKE_PREFIX_PATH = $Kit
                CMAKE_GENERATOR_INSTANCE = $VS
                MFG_BUILD_GUI = $true
                BUILD_TESTING = $true
            }
            'cmake.installPrefix' = (Join-Path $Repo 'cpp/install-windows')
            'cmake.defaultBuildTarget' = 'mfg_gui'
            'cmake.environment' = @{ PATH = $runtimePath }
            'C_Cpp.default.configurationProvider' = 'ms-vscode.cmake-tools'
            'terminal.integrated.env.windows' = @{ PATH = $runtimePath }
            'files.watcherExclude' = @{ '**/cpp/.tools/**' = $true; '**/cpp/build*/**' = $true }
            'search.exclude' = @{ '**/cpp/.tools/**' = $true; '**/cpp/build*/**' = $true; '**/cpp/install*/**' = $true }
        }
        extensions = @{ recommendations = @('ms-vscode.cpptools', 'ms-vscode.cmake-tools') }
        tasks = @{
            version = '2.0.0'
            tasks = @(@{
                label = 'MFG: Build Windows'
                type = 'process'
                command = 'powershell.exe'
                args = $taskArgs
                options = @{ cwd = $Repo }
                problemMatcher = @('$msCompile')
                group = @{ kind = 'build'; isDefault = $true }
            })
        }
        launch = @{
            version = '0.2.0'
            configurations = @(@{
                name = 'Matrix Filling Game'
                type = 'cppvsdbg'
                request = 'launch'
                program = (Join-Path $build "$Config/MatrixFillingGame.exe")
                cwd = $Repo
                preLaunchTask = 'MFG: Build Windows'
                environment = @(@{ name = 'PATH'; value = $runtimePath })
                console = 'internalConsole'
            })
        }
    }
    $workspacePath = Join-Path $Repo 'MatrixFillingGame.local.code-workspace'
    $workspace | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $workspacePath -Encoding UTF8
    return $workspacePath
}

function Start-MfgSetup {
    if ($env:OS -ne 'Windows_NT' -or -not [Environment]::Is64BitOperatingSystem -or
        $env:PROCESSOR_ARCHITECTURE -ne 'AMD64') {
        throw 'This bootstrap requires Windows 10/11 x64 and 64-bit PowerShell. See cpp/README.md for macOS/Linux builds.'
    }
    $repo = Split-Path (Split-Path $PSScriptRoot)
    $source = Join-Path $repo 'cpp'
    $toolDir = Join-Path $source '.tools'
    $statePath = Join-Path $toolDir 'windows-setup.json'
    New-Item -ItemType Directory -Path $toolDir -Force | Out-Null
    Write-Host 'Matrix Filling Game - Windows setup'
    Write-Host 'The first run downloads development tools and may require Windows administrator approval.'
    Update-ProcessPath
    $vs = Get-VisualStudio
    $cmakeCandidates = @(
        (Join-Path $env:ProgramFiles 'CMake/bin/cmake.exe')
        (Join-Path $vs 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe')
    )
    $cmake = Find-CMake $cmakeCandidates
    if (-not $cmake) {
        Install-WingetPackage 'Kitware.CMake'
        $cmake = Find-CMake $cmakeCandidates
    }
    if (-not $cmake) { throw 'CMake 3.21+ is required but could not be located.' }
    $ctest = Join-Path (Split-Path $cmake) 'ctest.exe'
    $kit = $QtRoot
    if (-not $kit -and (Test-Path -LiteralPath $statePath)) {
        $state = Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
        $kit = $state.qtRoot
    }
    if (-not $kit) { $kit = Join-Path $toolDir 'Qt/6.8.3/msvc2022_64' }
    if (-not (Test-QtKit $kit)) {
        if ($QtRoot) { throw "-QtRoot must point to a complete Qt MSVC x64 kit (Widgets, Test, deployment tools): $kit" }
        if ($NoInstall -or $BuildOnly) { throw 'Qt is missing or incomplete. Run setup-windows.cmd to install it.' }
        $kit = Join-Path $toolDir 'Qt/6.8.3/msvc2022_64'
        $python = Find-Python
        if (-not $python) {
            Install-WingetPackage 'Python.Python.3.12' @('--scope', 'user', '--architecture', 'x64')
            $python = Find-Python
        }
        if (-not $python) { throw 'A compatible 64-bit Python could not be located after installation.' }
        $venv = Join-Path $toolDir 'aqt-venv'
        $venvPython = Join-Path $venv 'Scripts/python.exe'
        $venvWorks = $false
        if (Test-Path -LiteralPath $venvPython) {
            try {
                & $venvPython -c 'pass' 2>$null
                $venvWorks = ($LASTEXITCODE -eq 0)
            } catch { $venvWorks = $false }
        }
        if (-not $venvWorks) {
            # This directory is exclusively the bootstrap's generated environment.
            Invoke-Checked $python @('-m', 'venv', '--clear', $venv)
        }
        Invoke-Checked $venvPython @('-m', 'pip', 'install', '--disable-pip-version-check', 'aqtinstall==3.3.0')
        Write-Host 'Downloading prebuilt Qt 6.8.3 (this can take several minutes) ...'
        Invoke-Checked $venvPython @('-m', 'aqt', 'install-qt', 'windows', 'desktop', '6.8.3',
            'win64_msvc2022_64', '--outputdir', (Join-Path $toolDir 'Qt'))
        if (-not (Test-QtKit $kit)) { throw 'Qt installation is incomplete. Rerun setup-windows.cmd to retry.' }
    }
    $kit = (Resolve-Path -LiteralPath $kit).Path
    @{ qtRoot = $kit } | ConvertTo-Json | Set-Content -LiteralPath $statePath -Encoding UTF8
    $env:PATH = (Join-Path $kit 'bin') + ';' + $env:PATH

    $dot = Find-Program 'dot.exe' @((Join-Path $env:ProgramFiles 'Graphviz/bin/dot.exe'))
    if (-not $dot -and -not $NoInstall -and -not $BuildOnly) {
        Install-WingetPackage 'Graphviz.Graphviz'
        $dot = Find-Program 'dot.exe' @((Join-Path $env:ProgramFiles 'Graphviz/bin/dot.exe'))
    }
    if ($dot) { $env:PATH = (Split-Path $dot) + ';' + $env:PATH }

    $build = Join-Path $source 'build-windows'
    $install = Join-Path $source 'install-windows'
    Write-Host "Configuring and building ($Configuration) ..."
    Invoke-Checked $cmake @('-S', $source, '-B', $build, '-G', 'Visual Studio 17 2022', '-A', 'x64',
        "-DCMAKE_GENERATOR_INSTANCE=$vs", "-DCMAKE_PREFIX_PATH=$kit", '-DMFG_BUILD_GUI=ON', '-DBUILD_TESTING=ON')
    Invoke-Checked $cmake @('--build', $build, '--config', $Configuration, '--parallel', '4')
    if ($BuildOnly) { return }
    if (-not $SkipTests) {
        Invoke-Checked $ctest @('--test-dir', $build, '-C', $Configuration, '--output-on-failure')
    }
    Invoke-Checked $cmake @('--install', $build, '--config', $Configuration, '--prefix', $install)
    $workspacePath = Write-MfgWorkspace $repo $kit $cmake $vs $Configuration $dot

    if (-not $SkipEditor) {
        $codeCandidates = @(
            (Join-Path $env:LOCALAPPDATA 'Programs/Microsoft VS Code/bin/code.cmd')
            (Join-Path $env:ProgramFiles 'Microsoft VS Code/bin/code.cmd')
        )
        $code = Find-Program 'code.cmd' $codeCandidates
        if (-not $code) {
            Install-WingetPackage 'Microsoft.VisualStudioCode' @('--scope', 'user')
            $code = Find-Program 'code.cmd' $codeCandidates
        }
        if (-not $code) { throw 'VS Code could not be located after installation.' }
        if (-not $NoInstall) {
            Invoke-Checked $code @('--install-extension', 'ms-vscode.cpptools')
            Invoke-Checked $code @('--install-extension', 'ms-vscode.cmake-tools')
        }
        Invoke-Checked $code @('--new-window', $workspacePath)
    }
    Write-Host "Setup complete. Open $workspacePath and press F5 to build and run."
    Write-Host "Standalone application: $(Join-Path $install 'bin/MatrixFillingGame.exe')"
}

# Dot-sourcing loads functions for the regression harness without installing anything.
if ($MyInvocation.InvocationName -ne '.') {
    $ErrorActionPreference = 'Stop'
    try { Start-MfgSetup }
    catch {
        Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
        exit 1
    }
}
