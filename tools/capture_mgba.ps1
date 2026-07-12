# Launch mGBA with a ROM, screenshot its window after a delay, then close it.
# Usage: powershell -File tools/capture_mgba.ps1 -Rom <path> -Out <png> [-DelaySec 5] [-HoldKeys "..."]
param(
    [Parameter(Mandatory=$true)][string]$Rom,
    [Parameter(Mandatory=$true)][string]$Out,
    [int]$DelaySec = 5
)

Add-Type @'
using System;
using System.Runtime.InteropServices;
public class Win32Cap {
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, out RECT r);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT p);
    public struct RECT { public int Left, Top, Right, Bottom; }
    public struct POINT { public int X, Y; }
}
'@
Add-Type -AssemblyName System.Drawing
[Win32Cap]::SetProcessDPIAware() | Out-Null

$mgba = "C:\Program Files\mGBA\mGBA.exe"
$p = Start-Process $mgba -ArgumentList "-3","`"$Rom`"" -PassThru
try {
    Start-Sleep -Seconds $DelaySec
    $p.Refresh()
    $hwnd = $p.MainWindowHandle
    if ($hwnd -eq [IntPtr]::Zero) { throw "mGBA window not found" }
    [Win32Cap]::SetForegroundWindow($hwnd) | Out-Null
    Start-Sleep -Milliseconds 500

    $rect = New-Object Win32Cap+RECT
    [Win32Cap]::GetClientRect($hwnd, [ref]$rect) | Out-Null
    $pt = New-Object Win32Cap+POINT
    [Win32Cap]::ClientToScreen($hwnd, [ref]$pt) | Out-Null
    $w = $rect.Right - $rect.Left; $h = $rect.Bottom - $rect.Top
    if ($w -le 0 -or $h -le 0) { throw "bad client rect" }

    $bmp = New-Object System.Drawing.Bitmap($w, $h)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($pt.X, $pt.Y, 0, 0, $bmp.Size)
    $g.Dispose()
    $dir = Split-Path -Parent $Out
    if ($dir -and -not (Test-Path $dir)) { New-Item -ItemType Directory -Force $dir | Out-Null }
    $bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Output "CAPTURED $Out"
} finally {
    try { Stop-Process -Id $p.Id -Force -Confirm:$false -ErrorAction Stop } catch {}
}
