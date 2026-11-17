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
  $taskStateBits = [IntPtr]::Zero
  if ([HextShortcutRuntime]::SendMessageTimeout($taskWindow,0x8009,[IntPtr]::Zero,[IntPtr]::Zero,2,8000,[ref]$taskStateBits) -eq [IntPtr]::Zero) { throw 'Hext state query timed out.' }
  $taskStateFlags=$taskStateBits.ToInt64()
  $taskNativeStyle=[IntPtr]::Zero
  [void][HextShortcutRuntime]::SendMessageTimeout($taskWindow,0x8009,[IntPtr]1,[IntPtr]::Zero,2,8000,[ref]$taskNativeStyle)
  $taskLayerError=[IntPtr]::Zero
  [void][HextShortcutRuntime]::SendMessageTimeout($taskWindow,0x8009,[IntPtr]2,[IntPtr]::Zero,2,8000,[ref]$taskLayerError)
  $taskAbove=[IntPtr]::Zero
  [void][HextShortcutRuntime]::SendMessageTimeout($taskWindow,0x8009,[IntPtr]3,[IntPtr]::Zero,2,8000,[ref]$taskAbove)
  [pscustomobject]@{ Visible=[HextShortcutRuntime]::IsWindowVisible($taskWindow); Topmost=(([HextShortcutRuntime]::GetWindowLong($taskWindow,-20) -band 8) -ne 0); AbovePrevious=($taskAbove -ne [IntPtr]::Zero); NativeStyle=$taskNativeStyle.ToInt64();ExternalStyle=[HextShortcutRuntime]::GetWindowLong($taskWindow,-20); LayerError=$taskLayerError.ToInt64(); Height=($taskBounds.Bottom-$taskBounds.Top); Flags=$taskStateFlags; Expanded=(($taskStateFlags -band 1) -ne 0); HideID=(($taskStateFlags -shr 8) -band 15); ExpandID=(($taskStateFlags -shr 12) -band 15) }
}
function Invoke-HextShortcut([int]$slot) {
  $taskIDs=Read-HextWindow
  $taskID=if($slot -eq 1){$taskIDs.HideID}else{$taskIDs.ExpandID}
  if(-not $taskID){throw ('Shortcut is not registered: '+($taskIDs | ConvertTo-Json -Compress))}
  $taskKey = if ($slot -eq 1) { 72 } else { 32 }
  [void][HextShortcutRuntime]::PostMessage($taskWindow,0x0312,[IntPtr]$taskID,[IntPtr](($taskKey -shl 16) -bor 3))
}
function Wait-HextState([scriptblock]$Predicate,[string]$Failure) {
  $taskDeadline=[DateTime]::UtcNow.AddSeconds(8)
  do {
    $taskProbe=Read-HextWindow
    if(& $Predicate $taskProbe){return $taskProbe}
    Start-Sleep -Milliseconds 50
  }while([DateTime]::UtcNow -lt $taskDeadline)
  throw ($Failure+': '+($taskProbe | ConvertTo-Json -Compress))
}
try {
  # The pointer can have expanded normal desktop presence before the test starts.
  if((Read-HextWindow).Expanded){
    Invoke-HextShortcut 2
    $null=Wait-HextState {param($s) -not $s.Expanded -and -not $s.Topmost -and $s.Height -lt 130} 'Initial collapse did not settle'
  }
  Invoke-HextShortcut 2
  $null=Wait-HextState {param($s) $s.Expanded -and $s.Visible -and ($s.Topmost -or $s.AbovePrevious) -and $s.Height -ge 200} 'Expand did not open above the current application'
  'PASS: expand opens the real Hext window above the current application.'
  Invoke-HextShortcut 2
  $null=Wait-HextState {param($s) -not $s.Expanded -and -not $s.Topmost -and $s.Height -lt 130} 'Collapse did not restore normal desktop presentation'
  'PASS: collapse removes topmost and restores compact geometry.'
  Invoke-HextShortcut 1
  $null=Wait-HextState {param($s) -not $s.Visible -and ($s.Flags -band 8)} 'Hide did not hide the window'
  Invoke-HextShortcut 1
  $null=Wait-HextState {param($s) -not ($s.Flags -band 8)} 'Show did not resume normal presence'
  Invoke-HextShortcut 2
  $null=Wait-HextState {param($s) $s.Expanded -and $s.Visible -and ($s.Topmost -or $s.AbovePrevious)} 'Expand could not restore the hidden island'
  'PASS: visibility and expansion remain independent after hide/show.'
  Invoke-HextShortcut 1
  $null=Wait-HextState {param($s) -not $s.Visible -and -not $s.Topmost -and ($s.Flags -band 8)} 'Hiding the overlay left it visible or topmost'
  Invoke-HextShortcut 1
  $null=Wait-HextState {param($s) -not ($s.Flags -band 8)} 'Could not restore normal presence'
  'PASS: hiding the expanded overlay clears its topmost state; normal mode restored.'
} finally {
  if ((Read-HextWindow).Topmost) {
    [void][HextShortcutRuntime]::PostMessage($taskWindow,0x8005,[IntPtr]10,[IntPtr]::Zero)
  }
}
