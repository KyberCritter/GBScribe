# Launch mGBA with a ROM, screenshot its window after a delay, then close it.
# Usage: powershell -File tools/capture_mgba.ps1 -Rom <path> -Out <png> [-DelaySec 5] [-SendKeys "..."]
#
# Focus-safe input: the window is moved on-screen but is NEVER focused and never
# made topmost (no SetForegroundWindow / HWND_TOPMOST), and keys are posted
# straight to the mGBA window with PostMessage instead of the system-wide
# keybd_event. System-wide injection leaked keys into whatever owned the
# foreground (an editor/TUI) and interrupted the caller; posting to the window
# cannot leak. The frame is grabbed with PrintWindow so it still works when the
# window is occluded, falling back to a screen grab if that yields a blank frame.
param(
    [Parameter(Mandatory=$true)][string]$Rom,
    [Parameter(Mandatory=$true)][string]$Out,
    [int]$DelaySec = 5,
    [string]$SendKeys = ""   # SendKeys sequence sent after boot (mGBA binds: X=A, Z=B, arrows, Enter=Start, Backspace=Select)
)

Add-Type -ReferencedAssemblies System.Drawing @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
public class Win32Cap {
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint msg, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint code, uint mapType);
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr hWnd, IntPtr after, int x, int y, int cx, int cy, uint flags);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, out RECT r);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT r);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT p);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint flags);
    public struct RECT { public int Left, Top, Right, Bottom; }
    public struct POINT { public int X, Y; }

    // WM_KEYDOWN/WM_KEYUP lParam: repeat=1, scancode in bits 16-23, extended flag
    // for the arrow keys, and the up/transition bits for key-up.
    static IntPtr LP(uint vk, bool up) {
        uint sc = MapVirtualKey(vk, 0);                 // MAPVK_VK_TO_VSC
        long lp = 1 | ((long)sc << 16);
        long[] ext = { 0x25, 0x26, 0x27, 0x28 };        // left/up/right/down
        foreach (long e in ext) if (e == vk) lp |= (1L << 24);
        if (up) lp |= (1L << 30) | (1L << 31);
        return (IntPtr)lp;
    }
    public static void KeyDown(IntPtr h, uint vk) { PostMessage(h, 0x0100, (IntPtr)vk, LP(vk, false)); }
    public static void KeyUp(IntPtr h, uint vk)   { PostMessage(h, 0x0101, (IntPtr)vk, LP(vk, true)); }

    public static Bitmap PrintClient(IntPtr h) {
        RECT wr, cr; GetWindowRect(h, out wr); GetClientRect(h, out cr);
        POINT p = new POINT(); ClientToScreen(h, ref p);
        int dx = p.X - wr.Left, dy = p.Y - wr.Top;
        int w = cr.Right - cr.Left, ht = cr.Bottom - cr.Top;
        int ww = wr.Right - wr.Left, wh = wr.Bottom - wr.Top;
        if (ww <= 0 || wh <= 0 || w <= 0 || ht <= 0) throw new Exception("bad rects");
        using (Bitmap full = new Bitmap(ww, wh)) {
            using (Graphics g = Graphics.FromImage(full)) {
                IntPtr hdc = g.GetHdc();
                PrintWindow(h, hdc, 2);   // PW_RENDERFULLCONTENT
                g.ReleaseHdc(hdc);
            }
            Rectangle r = new Rectangle(dx, dy, w, ht);
            if (r.X < 0) { r.Width += r.X; r.X = 0; }
            if (r.Y < 0) { r.Height += r.Y; r.Y = 0; }
            if (r.Right > full.Width)  r.Width  = full.Width  - r.X;
            if (r.Bottom > full.Height) r.Height = full.Height - r.Y;
            Bitmap outc = new Bitmap(Math.Max(1, r.Width), Math.Max(1, r.Height));
            using (Graphics g2 = Graphics.FromImage(outc))
                g2.DrawImage(full, new Rectangle(0, 0, outc.Width, outc.Height), r, GraphicsUnit.Pixel);
            return outc;
        }
    }
    public static bool IsBlank(Bitmap b) {
        Color c0 = b.GetPixel(0, 0);
        for (int y = 0; y < b.Height; y += Math.Max(1, b.Height / 12))
            for (int x = 0; x < b.Width; x += Math.Max(1, b.Width / 16))
                if (b.GetPixel(x, y).ToArgb() != c0.ToArgb()) return false;
        return true;
    }
}
'@
Add-Type -AssemblyName System.Drawing
[Win32Cap]::SetProcessDPIAware() | Out-Null

$mgba = "C:\Program Files\mGBA\mGBA.exe"
# stray instances hold a lock on the ROM and break the next build
try { Get-Process mGBA -ErrorAction Stop | Stop-Process -Force -Confirm:$false } catch {}
$proc = Start-Process $mgba -ArgumentList "-3","`"$Rom`"" -PassThru
$myPid = $proc.Id
try {
    # wait for the window to exist (do not assume it is up after DelaySec)
    $hwnd = [IntPtr]::Zero
    for ($i = 0; $i -lt 40; $i++) {
        Start-Sleep -Milliseconds 250
        $proc.Refresh()
        if ($proc.MainWindowHandle -ne [IntPtr]::Zero) { $hwnd = $proc.MainWindowHandle; break }
    }
    if ($hwnd -eq [IntPtr]::Zero) { throw "mGBA window not found" }
    Start-Sleep -Seconds $DelaySec

    # place on-screen but do not activate or reorder: SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE
    [Win32Cap]::SetWindowPos($hwnd, [IntPtr]::Zero, 60, 60, 0, 0, 0x0015) | Out-Null
    Start-Sleep -Milliseconds 400

    if ($SendKeys -ne "") {
        # tokens separated by spaces: letter/digit, left/right/up/down/enter/bksp, pause
        # chords: "a+left" presses A then LEFT together (mGBA L shoulder = A key)
        $vkmap = @{ 'left'=0x25; 'up'=0x26; 'right'=0x27; 'down'=0x28; 'enter'=0x0D; 'bksp'=0x08 }
        function Get-VK([string]$name) {
            if ($vkmap.ContainsKey($name)) { return [uint32]$vkmap[$name] }
            return [uint32][char]$name.ToUpper()[0]
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
            foreach ($k in $parts) { [Win32Cap]::KeyDown($hwnd, (Get-VK $k)); Start-Sleep -Milliseconds 40 }
            Start-Sleep -Milliseconds $holdMs
            [array]::Reverse($parts)
            foreach ($k in $parts) { [Win32Cap]::KeyUp($hwnd, (Get-VK $k)); Start-Sleep -Milliseconds 40 }
            Start-Sleep -Milliseconds 90
        }
        Start-Sleep -Milliseconds 500
    }

    $bmp = [Win32Cap]::PrintClient($hwnd)
    if ([Win32Cap]::IsBlank($bmp)) {
        # PrintWindow gave nothing (some GL surfaces); fall back to the on-screen rect
        $bmp.Dispose()
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
    }
    $dir = Split-Path -Parent $Out
    if ($dir -and -not (Test-Path $dir)) { New-Item -ItemType Directory -Force $dir | Out-Null }
    $bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Output "CAPTURED $Out"
} finally {
    try { Stop-Process -Id $myPid -Force -Confirm:$false -ErrorAction Stop } catch {}
    Start-Sleep -Milliseconds 300
    try { Get-Process mGBA -ErrorAction Stop | Stop-Process -Force -Confirm:$false } catch {}
}
