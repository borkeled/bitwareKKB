param(
    [string]$ExePath = ".\Build\Bitware.exe",
    [string]$CertSubject = "CN=Bitware",
    [string]$PfxPassword = "bitware"
)

$ErrorActionPreference = "Stop"

$exeFullPath = Resolve-Path $ExePath -ErrorAction Stop

Write-Host "[sign] Creating self-signed code signing certificate..." -ForegroundColor Cyan
$cert = New-SelfSignedCertificate `
    -Type CodeSigning `
    -Subject $CertSubject `
    -CertStoreLocation Cert:\CurrentUser\My `
    -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3") `
    -NotAfter (Get-Date).AddYears(3)

Write-Host "[sign] Certificate thumbprint: $($cert.Thumbprint)" -ForegroundColor Green

$pfxPath = Join-Path $env:TEMP "Bitware_temp.pfx"
$securePass = ConvertTo-SecureString -String $PfxPassword -Force -AsPlainText

Export-PfxCertificate -Cert $cert -FilePath $pfxPath -Password $securePass | Out-Null

$signtoolPaths = @(
    "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe",
    "C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\signtool.exe",
    "C:\Program Files (x86)\Windows Kits\10\bin\10.0.22000.0\x64\signtool.exe",
    "C:\Program Files (x86)\Windows Kits\10\bin\10.0.20348.0\x64\signtool.exe",
    "C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\signtool.exe",
    "C:\Program Files (x86)\Microsoft SDKs\Windows\v10.0A\bin\NETFX 4.8 Tools\x64\signtool.exe"
)

$signtool = $null
foreach ($path in $signtoolPaths) {
    if (Test-Path $path) {
        $signtool = $path
        break
    }
}

if (-not $signtool) {
    $signtool = Get-Command "signtool.exe" -ErrorAction SilentlyContinue
}

if (-not $signtool) {
    Write-Warning "[sign] signtool.exe not found. Install Windows SDK or specify path manually."
    Write-Warning "[sign] Certificate is still available in Cert:\CurrentUser\My\$($cert.Thumbprint)"
    Remove-Item $pfxPath -Force
    return
}

Write-Host "[sign] Signing $exeFullPath ..." -ForegroundColor Cyan
& $signtool sign /fd SHA256 /a /f $pfxPath /p $PfxPassword /tr http://timestamp.digicert.com /td SHA256 "$exeFullPath"

if ($LASTEXITCODE -eq 0) {
    Write-Host "[sign] Successfully signed!" -ForegroundColor Green
} else {
    Write-Warning "[sign] Signing failed with exit code $LASTEXITCODE"
}

Remove-Item $pfxPath -Force
