# Captures the real running window of a process to a PNG (Definition of done, item 6).
# PrintWindow, not a screen grab: works when the window is covered. DPI-aware because
# this machine runs at 300% scale and a non-aware capture returns only the top-left corner.
# Usage: powershell -ExecutionPolicy Bypass -File scripts\capture-window.ps1 -Out shot.png [-Name giga-strata] [-Close]
param([Parameter(Mandatory)][string]$Out, [string]$Name = "giga-strata", [switch]$Close)
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System; using System.Runtime.InteropServices;
public class W { [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
[DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint f);
[DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
public struct RECT { public int L, T, R, B; } }
"@
[W]::SetProcessDPIAware() | Out-Null
$p = $null
for ($i = 0; $i -lt 40 -and -not $p; $i++) { Start-Sleep -Milliseconds 250; $p = Get-Process $Name -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1 }
if (-not $p) { Write-Output "NO WINDOW"; exit 1 }
Start-Sleep -Milliseconds 800
$r = New-Object W+RECT; [W]::GetWindowRect($p.MainWindowHandle, [ref]$r) | Out-Null
$bmp = New-Object System.Drawing.Bitmap ($r.R - $r.L), ($r.B - $r.T)
$g = [System.Drawing.Graphics]::FromImage($bmp); $dc = $g.GetHdc()
[W]::PrintWindow($p.MainWindowHandle, $dc, 2) | Out-Null; $g.ReleaseHdc($dc); $bmp.Save($Out)
Write-Output "PID=$($p.Id) title=$($p.MainWindowTitle) size=$($bmp.Width)x$($bmp.Height)"
if ($Close) { Stop-Process -Id $p.Id }
