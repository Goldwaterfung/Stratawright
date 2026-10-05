# Stratawright Third-Party SDK Downloader Script (Windows PowerShell)
# Automatically fetches required third-party C++ libraries and plugin SDKs into src/third_party

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ThirdPartyDir = Join-Path $ProjectRoot "src\third_party"

if (-not (Test-Path $ThirdPartyDir)) {
    New-Item -ItemType Directory -Path $ThirdPartyDir -Force | Out-Null
}

Write-Host "Setting up third-party SDKs in $ThirdPartyDir..." -ForegroundColor Cyan

function Clone-Repo {
    param (
        [string]$FolderName,
        [string]$RepoUrl
    )
    $TargetDir = Join-Path $ThirdPartyDir $FolderName
    if (Test-Path $TargetDir) {
        $items = Get-ChildItem -Path $TargetDir
        if ($items.Count -gt 0) {
            Write-Host "✓ $FolderName already exists, skipping." -ForegroundColor Green
            return
        }
    }
    Write-Host "--> Cloning $FolderName from $RepoUrl..." -ForegroundColor Yellow
    git clone --depth 1 --recursive $RepoUrl $TargetDir
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✓ $FolderName setup complete." -ForegroundColor Green
    } else {
        Write-Error "Failed to clone $FolderName from $RepoUrl"
    }
}

# 1. RtAudio (Audio Driver HAL)
Clone-Repo "rtaudio" "https://github.com/thestk/rtaudio.git"

# 2. RtMidi (MIDI Driver HAL)
Clone-Repo "rtmidi" "https://github.com/thestk/rtmidi.git"

# 3. VST3 SDK (Steinberg VST3 Host/Plugin SDK)
Clone-Repo "VST3_SDK" "https://github.com/steinbergmedia/vst3sdk.git"

# 4. CLAP SDK (CLever Audio Plugin SDK)
Clone-Repo "CLAP_SDK" "https://github.com/free-audio/clap.git"

# 5. Eigen (Linear Algebra & FFT)
Clone-Repo "eigen-5.0.0" "https://gitlab.com/libeigen/eigen.git"

# 6. Rubber Band (Pitch Shifting & Time Stretching)
Clone-Repo "rubberband-default" "https://github.com/breakfastquay/rubberband.git"
$RubberbandCmake = Join-Path $ThirdPartyDir "rubberband-default\CMakeLists.txt"
$SourceCmake = Join-Path $ProjectRoot "cmake\rubberband\CMakeLists.txt"
if ((-not (Test-Path $RubberbandCmake)) -and (Test-Path $SourceCmake)) {
    Write-Host "--> Installing RubberBand CMakeLists.txt..." -ForegroundColor Yellow
    Copy-Item $SourceCmake $RubberbandCmake
}

# 7. SoundTouch (Time Stretching & Jitter Buffer)
Clone-Repo "soundtouch" "https://codeberg.org/soundtouch/soundtouch.git"

# 8. ASIO SDK (Windows Low-Latency Audio Driver)
Clone-Repo "ASIOSDK" "https://github.com/audiosdk/asio.git"

Write-Host "`n✓ All third-party SDKs are successfully configured in $ThirdPartyDir!" -ForegroundColor Green
