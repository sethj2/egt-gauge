# Builds the firmware and publishes it as a GitHub release the gauge can pull.
# Usage: .\release.ps1 -Version 0.2.0
param([Parameter(Mandatory=$true)][string]$Version)
$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot

$fqbn = 'esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,CDCOnBoot=cdc'
$lvgl = 'ESP32-S3-Touch-LCD-1.85-Demo\Arduino\libraries\lvgl'
$out  = Join-Path $PSScriptRoot 'build\release'

# 1. stamp the version into the firmware
(Get-Content src\version.h) -replace '#define FW_VERSION ".*"', "#define FW_VERSION `"$Version`"" | Set-Content src\version.h

# 2. build (app-only image, not the merged one)
# RELEASE_BUILD keeps the Wi-Fi password out of the public firmware.bin (the gauge reads it from NVS)
$marker = Join-Path $PSScriptRoot 'src\release_build.h'
Set-Content -Path $marker -Value '// temporary marker created by release.ps1'
try {
  arduino-cli compile --fqbn $fqbn --library $lvgl --build-path $out .
  if ($LASTEXITCODE -ne 0) { throw 'build failed' }
} finally {
  Remove-Item $marker -ErrorAction SilentlyContinue
}

# 3. assets the gauge downloads
$dist = Join-Path $PSScriptRoot 'build\dist'
New-Item -ItemType Directory -Force $dist | Out-Null
Copy-Item (Join-Path $out 'egt_gauge.ino.bin') (Join-Path $dist 'firmware.bin') -Force
Set-Content -NoNewline -Path (Join-Path $dist 'version.txt') -Value $Version

# safety net: refuse to publish if the Wi-Fi SSID/password text is in the binary
$bin = [Text.Encoding]::GetEncoding(28591).GetString([IO.File]::ReadAllBytes((Join-Path $dist 'firmware.bin')))
foreach ($m in (Select-String -Path src\secrets.h -Pattern '#define WIFI_(SSID|PASS) "(.+)"')) {
  if ($bin.Contains($m.Matches[0].Groups[2].Value)) { throw "Wi-Fi $($m.Matches[0].Groups[1].Value) found in firmware.bin, refusing to publish" }
}

# 4. commit the version bump and publish
git add src\version.h
git commit -m "Release v$Version"
git push
gh release create "v$Version" "$dist\firmware.bin" "$dist\version.txt" --title "v$Version" --notes "Firmware v$Version"
