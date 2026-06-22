@echo off
setlocal enabledelayedexpansion

set MSBuild="D:\visualstudio\MSBuild\Current\Bin\MSBuild.exe"
set ROOT=%~dp0

echo ===== Building BitwareDrv.sys =====
%MSBuild% "%ROOT%Bitware\BitwareDrv\BitwareDrv.vcxproj" /p:Configuration=Release /p:Platform=x64 /t:Rebuild /v:m
if %errorlevel% neq 0 ( echo Driver build FAILED & exit /b %errorlevel% )

echo ===== Generating driver resource header =====
set SYS_PATH=%ROOT%Bitware\Build\BitwareDrv.sys
set HPP_PATH=%ROOT%Bitware\Source\Driver\kdmapper\bitware_driver_resource.hpp

powershell -ExecutionPolicy Bypass -Command ^
$bytes = [System.IO.File]::ReadAllBytes('%SYS_PATH%'); ^
$sb = [System.Text.StringBuilder]::new(); ^
$null = $sb.AppendLine('#pragma once'); ^
$null = $sb.AppendLine('#include ^<stdint.h^>'); ^
$null = $sb.AppendLine(''); ^
$null = $sb.AppendLine('namespace bitware_driver_resource'); ^
$null = $sb.AppendLine('{'); ^
$null = $sb.AppendLine('    constexpr uint64_t size = ' + $bytes.Length + ';'); ^
$null = $sb.Append('    constexpr unsigned char driver[' + $bytes.Length + '] = { '); ^
$c = 0; ^
for ($i = 0; $i -lt $bytes.Length; $i++) { ^
    if ($c -eq 0) { $null = $sb.AppendLine(); $null = $sb.Append('        '); }; ^
    $null = $sb.Append('0x' + $bytes[$i].ToString('x2')); ^
    if ($i -lt $bytes.Length - 1) { $null = $sb.Append(', '); }; ^
    $c++; if ($c -ge 12) { $c = 0 }; ^
}; ^
$null = $sb.AppendLine(); ^
$null = $sb.AppendLine('    };'); ^
$null = $sb.AppendLine('}'); ^
Set-Content -Path '%HPP_PATH%' -Value $sb.ToString() -Encoding UTF8
if %errorlevel% neq 0 ( echo Resource generation FAILED & exit /b %errorlevel% )

echo ===== Building Bitware.exe =====
%MSBuild% "%ROOT%Bitware\Bitware.vcxproj" /p:Configuration=Release /p:Platform=x64 /t:Rebuild /v:m
if %errorlevel% neq 0 ( echo Main build FAILED & exit /b %errorlevel% )

echo ===== Done =====
echo Outputs:
dir "%ROOT%Build\Bitware.exe"
dir "%ROOT%Bitware\Build\BitwareDrv.sys"
