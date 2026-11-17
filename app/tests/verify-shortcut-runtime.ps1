# Verifies only Hext's own window messages and styles. No desktop capture or key injection.
$ErrorActionPreference = 'Stop'
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class HextShortcutRuntime {
  [StructLayout(LayoutKind.Sequential)] public struct Rect { public int Left,Top,Right,Bottom; }
  [DllImport("user32.dll",CharSet=CharSet.Unicode)] public static extern IntPtr FindWindow(string cls,string title);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr window,out uint process);
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr window,uint message,IntPtr wp,IntPtr lp);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr window);
  [DllImport("user32.dll")] public static extern int GetWindowLong(IntPtr window,int index);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr window,out Rect rect);
  [DllImport("user32.dll")] public static extern IntPtr SendMessageTimeout(IntPtr window,uint message,IntPtr wp,IntPtr lp,uint flags,uint timeout,out IntPtr result);
}
'@
$taskExe = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\dist\Hext.exe'))
$taskOwner = @(Get-Process -Name Hext | Where-Object { $_.Path -eq $taskExe })
if ($taskOwner.Count -ne 1) { throw 'Expected one verified Hext process.' }
$taskWindow = [HextShortcutRuntime]::FindWindow('HextIslandWindow','Hext')
[uint32]$taskOwnerID = 0
[void][HextShortcutRuntime]::GetWindowThreadProcessId($taskWindow,[ref]$taskOwnerID)
if ($taskOwnerID -ne $taskOwner[0].Id) { throw 'Unexpected window owner.' }
$taskReadyResult = [IntPtr]::Zero
if ([HextShortcutRuntime]::SendMessageTimeout($taskWindow,0,[IntPtr]::Zero,[IntPtr]::Zero,2,8000,[ref]$taskReadyResult) -eq [IntPtr]::Zero) { throw 'Hext message loop is not ready.' }
function Read-HextWindow {
  $taskBounds = New-Object HextShortcutRuntime+Rect
  [void][HextShortcutRuntime]::GetWindowRect($taskWindow,[ref]$taskBounds)
  [pscustomobject]@{ Visible=[HextShortcutRuntime]::IsWindowVisible($taskWindow); Topmost=(([HextShortcutRuntime]::GetWindowLong($taskWindow,-20) -band 8) -ne 0); Height=($taskBounds.Bottom-$taskBounds.Top) }
}
function Invoke-HextShortcut([int]$slot) {
  # Startup reservations: hide=1, expand=2. This script runs immediately after deployment.
  $taskKey = if ($slot -eq 1) { 72 } else { 32 }
  [void][HextShortcutRuntime]::PostMessage($taskWindow,0x0312,[IntPtr]$slot,[IntPtr](($taskKey -shl 16) -bor 3))
}
try {
  # The pointer can have expanded normal desktop presence before the test starts.
  [void][HextShortcutRuntime]::PostMessage($taskWindow,0x8005,[IntPtr]10,[IntPtr]::Zero)
  Start-Sleep -Milliseconds 750
  Invoke-HextShortcut 2
  Start-Sleep -Milliseconds 750
  $taskState = Read-HextWindow
  if (-not $taskState.Visible -or -not $taskState.Topmost -or $taskState.Height -lt 200) { throw ('Expand did not open above the current application: ' + ($taskState | ConvertTo-Json -Compress)) }
  'PASS: expand opens the real Hext window above the current application.'
  Invoke-HextShortcut 2
  Start-Sleep -Milliseconds 1100
  $taskState = Read-HextWindow
  if ($taskState.Topmost -or $taskState.Height -gt 130) { throw 'Collapse did not restore normal desktop presentation.' }
  'PASS: collapse removes topmost and restores compact geometry.'
  Invoke-HextShortcut 1
  Start-Sleep -Milliseconds 550
  if ((Read-HextWindow).Visible) { throw 'Hide did not hide the window.' }
  Invoke-HextShortcut 1
  Start-Sleep -Milliseconds 450
  Invoke-HextShortcut 2
  Start-Sleep -Milliseconds 750
  $taskState = Read-HextWindow
  if (-not $taskState.Visible -or -not $taskState.Topmost) { throw 'Expand could not restore the hidden island.' }
  'PASS: visibility and expansion remain independent after hide/show.'
  Invoke-HextShortcut 1
  Start-Sleep -Milliseconds 650
  $taskState = Read-HextWindow
  if ($taskState.Visible -or $taskState.Topmost) { throw 'Hiding the overlay left it visible or topmost.' }
  Invoke-HextShortcut 1
  Start-Sleep -Milliseconds 450
  'PASS: hiding the expanded overlay clears its topmost state; normal mode restored.'
} finally {
  if ((Read-HextWindow).Topmost) {
    [void][HextShortcutRuntime]::PostMessage($taskWindow,0x8005,[IntPtr]10,[IntPtr]::Zero)
  }
}
