# Launch mGBA with a ROM, screenshot its window after a delay, then close it.
# Usage: powershell -File tools/capture_mgba.ps1 -Rom <path> -Out <png> [-DelaySec 5] [-HoldKeys "..."]
param(
    [Parameter(Mandatory=$true)][string]$Rom,
    [Parameter(Mandatory=$true)][string]$Out,
    [int]$DelaySec = 5,
    [string]$SendKeys = ""   # SendKeys sequence sent after boot (mGBA binds: X=A, Z=B, arrows, Enter=Start, Backspace=Select)
)

Add-Type @'
using System;
using System.Runtime.InteropServices;
public class Win32Cap {
    [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
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
# stray instances hold a lock on the ROM and break the next build
try { Get-Process mGBA -ErrorAction Stop | Stop-Process -Force -Confirm:$false } catch {}
$p = Start-Process $mgba -ArgumentList "-3","`"$Rom`"" -PassThru
try {
    Start-Sleep -Seconds $DelaySec
    $p.Refresh()
    $hwnd = $p.MainWindowHandle
    if ($hwnd -eq [IntPtr]::Zero) { throw "mGBA window not found" }
    [Win32Cap]::SetForegroundWindow($hwnd) | Out-Null
    Start-Sleep -Milliseconds 500

    if ($SendKeys -ne "") {
        # tokens separated by spaces: letter/digit, left/right/up/down/enter/bksp, pause
        # chords: "a+left" presses A then LEFT together (mGBA L shoulder = A key)
        $vkmap = @{ 'left'=0x25; 'up'=0x26; 'right'=0x27; 'down'=0x28; 'enter'=0x0D; 'bksp'=0x08 }
        function Get-VK([string]$name) {
            if ($vkmap.ContainsKey($name)) { return [byte]$vkmap[$name] }
            return [byte][char]$name.ToUpper()[0]
        }
        foreach ($tok in $SendKeys.ToLower() -split ' ') {
            if ($tok -eq '') { continue }
            if ($tok -eq 'pause') { Start-Sleep -Milliseconds 600; continue }
            $holdMs = 130
            if ($tok -match '^hold(\d+):(.+)$') {   # holdN:key = hold for N tenths of a second
                $holdMs = [int]$Matches[1] * 100
                $tok = $Matches[2]
            }
            $parts = @($tok -split '\+')
            foreach ($p in $parts) { [Win32Cap]::keybd_event((Get-VK $p), 0, 0, [UIntPtr]::Zero); Start-Sleep -Milliseconds 40 }
            Start-Sleep -Milliseconds $holdMs
            [array]::Reverse($parts)
            foreach ($p in $parts) { [Win32Cap]::keybd_event((Get-VK $p), 0, 2, [UIntPtr]::Zero); Start-Sleep -Milliseconds 40 }
            Start-Sleep -Milliseconds 90
        }
        Start-Sleep -Milliseconds 500
    }

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
    Start-Sleep -Milliseconds 300
    try { Get-Process mGBA -ErrorAction Stop | Stop-Process -Force -Confirm:$false } catch {}
}
