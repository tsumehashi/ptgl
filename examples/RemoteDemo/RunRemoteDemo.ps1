param(
    [Parameter(Position = 0)]
    [ValidateSet(1, 2, 3)]
    [int]$View = 2,
    [string]$Url = '',
    [ValidateRange(0, 65535)]
    [int]$Port = 9002,
    [string]$Configuration = 'Release',
    [string]$BuildDir = '',
    [ValidateRange(0, 1000000)]
    [int]$SmokeFrames = 0
)

function readSharedLog([string]$Path) {
    $stream = [System.IO.File]::Open($Path, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
    $reader = [System.IO.StreamReader]::new($stream)
    try { return $reader.ReadToEnd() } finally { $reader.Dispose() }
}

if (!$BuildDir) { $BuildDir = Split-Path -Parent $MyInvocation.MyCommand.Path }

$ErrorActionPreference = 'Stop'
$publisherProcess = $null
$stdoutPath = $null
$stderrPath = $null
$result = 0

try {
    $buildRoot = (Resolve-Path -LiteralPath $BuildDir).Path
    $demoRoot = Join-Path $buildRoot 'examples/RemoteDemo'
    $binaryDir = Join-Path $demoRoot $Configuration
    if (!(Test-Path -LiteralPath (Join-Path $binaryDir 'RemoteViewerDemo.exe'))) {
        $binaryDir = $demoRoot
    }
    $viewerPath = Join-Path $binaryDir 'RemoteViewerDemo.exe'
    $publisherPath = Join-Path $binaryDir 'RemotePublisherDemo.exe'
    if (!(Test-Path -LiteralPath $viewerPath)) {
        throw "RemoteViewerDemo.exe was not found in $demoRoot. Build the Remote demos first."
    }

    # Include ptgl.dll for shared builds; third-party DLLs may also be on PATH.
    $runtimePath = "$binaryDir;$buildRoot;$(Join-Path $buildRoot $Configuration);$env:PATH"
    # Some developer shells pass both Path and PATH; .NET rejects that on launch.
    [System.Environment]::SetEnvironmentVariable('Path', $null, 'Process')
    [System.Environment]::SetEnvironmentVariable('PATH', $null, 'Process')
    [System.Environment]::SetEnvironmentVariable('PATH', $runtimePath, 'Process')
    if (!$Url) {
        if (!(Test-Path -LiteralPath $publisherPath)) {
            throw "RemotePublisherDemo.exe was not found in $binaryDir."
        }
        if ($Port -ne 0) {
            $listeners = [System.Net.NetworkInformation.IPGlobalProperties]::GetIPGlobalProperties().GetActiveTcpListeners()
            if ($listeners | Where-Object { $_.Port -eq $Port }) {
                throw "Port $Port is already in use. Use -Port 0 for a free port, or -Url to connect to an existing sender."
            }
        }
        $stdoutPath = [System.IO.Path]::GetTempFileName()
        $stderrPath = [System.IO.Path]::GetTempFileName()
        $startOptions = @{
            FilePath = $publisherPath
            ArgumentList = @('--port', $Port)
            WorkingDirectory = $binaryDir
            WindowStyle = 'Hidden'
            RedirectStandardOutput = $stdoutPath
            RedirectStandardError = $stderrPath
            PassThru = $true
        }
        $publisherProcess = Start-Process @startOptions

        # Wait for our publisher's listen confirmation, including its chosen port.
        $deadline = [DateTime]::UtcNow.AddSeconds(10)
        while (!$Url) {
            if ($publisherProcess.HasExited) {
                $details = (readSharedLog $stderrPath).Trim()
                throw "Remote publisher exited ($($publisherProcess.ExitCode)). $details"
            }
            $output = (readSharedLog $stdoutPath)
            if ($output -match 'Remote publisher: ws://<this-pc>:(\d+)') {
                $Url = "ws://127.0.0.1:$($Matches[1])"
                break
            }
            if ([DateTime]::UtcNow -ge $deadline) {
                throw 'Timed out waiting for the Remote publisher.'
            }
            Start-Sleep -Milliseconds 100
        }
        Write-Host "Started local publisher: $Url"
    }

    Write-Host "Opening view $View (1=3D, 2=time series, 3=XY): $Url"
    Write-Host 'Close the viewer window to finish.'
    $viewerArguments = @('--url', $Url, '--view', $View)
    if ($SmokeFrames) { $viewerArguments += @('--smoke-frames', $SmokeFrames) }
    & $viewerPath @viewerArguments
    if ($LASTEXITCODE -ne 0) { throw "Remote viewer exited ($LASTEXITCODE)." }
} catch {
    Write-Host $_.Exception.Message -ForegroundColor Red
    $result = 1
} finally {
    # Stop only the sender started by this invocation, never another user's sender.
    if ($null -ne $publisherProcess) {
        if (!$publisherProcess.HasExited) {
            Stop-Process -Id $publisherProcess.Id -ErrorAction SilentlyContinue
            $publisherProcess.WaitForExit()
        }
        $publisherProcess.Dispose()
    }
    foreach ($logPath in @($stdoutPath, $stderrPath)) {
        if ($logPath -and (Test-Path -LiteralPath $logPath)) {
            Remove-Item -LiteralPath $logPath -ErrorAction SilentlyContinue
        }
    }
}
exit $result
