# Reproduce coverage and return-to-desktop with one temporary owned window.
# Temporarily minimize and restore applications; no screen capture or key injection.
$ErrorActionPreference='Stop'
Add-Type @'
using System;using System.Runtime.InteropServices;
public static class HextRecoveryProbe {
  [StructLayout(LayoutKind.Sequential)]public struct Rect{public int Left,Top,Right,Bottom;}
  [StructLayout(LayoutKind.Sequential)]public struct Point{public int X,Y;}
  [StructLayout(LayoutKind.Sequential)]public struct Msg{public IntPtr Hwnd;public uint Message;public UIntPtr Wp;public IntPtr Lp;public uint Time;public Point Pt;public uint Private;}
  [DllImport("user32.dll",CharSet=CharSet.Unicode)]public static extern IntPtr FindWindow(string cls,string title);
  [DllImport("user32.dll",CharSet=CharSet.Unicode)]public static extern IntPtr CreateWindowEx(int ex,string cls,string title,int style,int x,int y,int w,int h,IntPtr parent,IntPtr menu,IntPtr inst,IntPtr param);
  [DllImport("user32.dll")]public static extern bool DestroyWindow(IntPtr w);
  [DllImport("user32.dll")]public static extern bool SetWindowPos(IntPtr w,IntPtr after,int x,int y,int width,int height,uint flags);
  [DllImport("user32.dll")]public static extern bool SetForegroundWindow(IntPtr w);
  [DllImport("user32.dll")]public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")]public static extern IntPtr GetShellWindow();
  [DllImport("user32.dll")]public static extern int GetSystemMetrics(int index);
  [DllImport("user32.dll")]public static extern IntPtr SetThreadDpiAwarenessContext(IntPtr context);
  [DllImport("user32.dll")]public static extern uint GetWindowThreadProcessId(IntPtr w,IntPtr pid);
  [DllImport("kernel32.dll")]public static extern uint GetCurrentThreadId();
  [DllImport("user32.dll")]public static extern bool AttachThreadInput(uint a,uint b,bool attach);
  [DllImport("user32.dll")]public static extern bool GetWindowRect(IntPtr w,out Rect r);
  [DllImport("user32.dll")]public static extern bool IsWindowVisible(IntPtr w);
  [DllImport("user32.dll")]public static extern bool PostMessage(IntPtr w,uint msg,IntPtr wp,IntPtr lp);
  [DllImport("user32.dll")]public static extern IntPtr SendMessageTimeout(IntPtr w,uint msg,IntPtr wp,IntPtr lp,uint flags,uint timeout,out IntPtr result);
  [DllImport("user32.dll")]public static extern bool PeekMessage(out Msg msg,IntPtr w,uint first,uint last,uint remove);
  [DllImport("user32.dll")]public static extern bool TranslateMessage(ref Msg msg);
  [DllImport("user32.dll")]public static extern IntPtr DispatchMessage(ref Msg msg);
  public static void Pump(){Msg m;while(PeekMessage(out m,IntPtr.Zero,0,0,1)){TranslateMessage(ref m);DispatchMessage(ref m);}}
  public static void Focus(IntPtr w){var f=GetForegroundWindow();uint other=GetWindowThreadProcessId(f,IntPtr.Zero),own=GetCurrentThreadId();bool attached=other!=0&&other!=own&&AttachThreadInput(own,other,true);try{SetForegroundWindow(w);}finally{if(attached)AttachThreadInput(own,other,false);}}
}
'@
$taskHext=[HextRecoveryProbe]::FindWindow('HextIslandWindow','Hext')
if($taskHext -eq [IntPtr]::Zero){throw 'Hext is not running.'}
$taskOriginalForeground=[HextRecoveryProbe]::GetForegroundWindow()
# Use physical coordinates, matching Hext's PerMonitorV2 window. A DPI-unaware
# PowerShell host otherwise mixes virtual screen metrics and native frame bounds.
$taskPreviousDpi=[HextRecoveryProbe]::SetThreadDpiAwarenessContext([IntPtr](-4))
function Read-HextRecovery {
  [HextRecoveryProbe]::Pump()
  $taskFlags=[IntPtr]::Zero
  if([HextRecoveryProbe]::SendMessageTimeout($taskHext,0x8009,[IntPtr]::Zero,[IntPtr]::Zero,2,3000,[ref]$taskFlags) -eq [IntPtr]::Zero){throw 'Hext is not responding.'}
  $taskRect=New-Object HextRecoveryProbe+Rect
  [void][HextRecoveryProbe]::GetWindowRect($taskHext,[ref]$taskRect)
  [pscustomobject]@{Visible=[HextRecoveryProbe]::IsWindowVisible($taskHext);Flags=$taskFlags.ToInt64();Rect=$taskRect}
}
function Wait-HextRecovery([scriptblock]$Condition,[string]$Failure){
  $taskEnd=[DateTime]::UtcNow.AddSeconds(6)
  do{$taskState=Read-HextRecovery;if(& $Condition $taskState){return $taskState};Start-Sleep -Milliseconds 30}while([DateTime]::UtcNow -lt $taskEnd)
  throw ($Failure+': '+($taskState|ConvertTo-Json -Compress -Depth 3))
}
$taskInitial=Read-HextRecovery
if($taskInitial.Flags -band 8){throw 'Resume Hext desktop activation before running this test.'}
if($taskInitial.Flags -band 1){
  $taskExpandID=($taskInitial.Flags -shr 12)-band 15
  [void][HextRecoveryProbe]::PostMessage($taskHext,0x0312,[IntPtr]$taskExpandID,[IntPtr]::Zero)
  $null=Wait-HextRecovery {param($s) -not($s.Flags-band 3)} 'Initial collapse failed'
}
$taskShell=New-Object -ComObject Shell.Application
$taskProbe=[IntPtr]::Zero
try{
$taskShell.MinimizeAll()
Start-Sleep -Milliseconds 700
[HextRecoveryProbe]::Focus([HextRecoveryProbe]::GetShellWindow())
$taskStart=Wait-HextRecovery {param($s) $s.Visible -and ($s.Flags -band 64)} 'Desktop did not become available for the test'
$taskCenter=[int]([HextRecoveryProbe]::GetSystemMetrics(0)/2)
$taskTop=0
$taskProbe=[HextRecoveryProbe]::CreateWindowEx(0x80,'STATIC','Hext desktop recovery verification',0x10CF0000,$taskCenter-300,$taskTop,600,180,[IntPtr]::Zero,[IntPtr]::Zero,[IntPtr]::Zero,[IntPtr]::Zero)
if($taskProbe -eq [IntPtr]::Zero){throw 'Could not create the temporary verification window.'}
  for($taskCycle=0;$taskCycle -lt 3;$taskCycle++){
    [void][HextRecoveryProbe]::SetWindowPos($taskProbe,[IntPtr]::Zero,$taskCenter-300,$taskTop,600,180,0x40)
    [HextRecoveryProbe]::Focus($taskProbe)
    $null=Wait-HextRecovery {param($s)
      # Delayed shell focus handoffs after MinimizeAll can steal the first activation.
      if([HextRecoveryProbe]::GetForegroundWindow() -ne $taskProbe){[HextRecoveryProbe]::Focus($taskProbe);return $false}
      -not $s.Visible -and -not($s.Flags -band 16)
    } 'Covering application did not suppress Hext'
    [void][HextRecoveryProbe]::SetWindowPos($taskProbe,[IntPtr]::Zero,20,$taskTop+200,300,220,0x40)
    $null=Wait-HextRecovery {param($s) $s.Visible -and ($s.Flags-band 64) -and -not($s.Flags-band 1)} 'Exposed desktop area did not restore the compact island'
  }
  [void][HextRecoveryProbe]::DestroyWindow($taskProbe);$taskProbe=[IntPtr]::Zero
  [HextRecoveryProbe]::Focus([HextRecoveryProbe]::GetShellWindow())
  $null=Wait-HextRecovery {param($s) $s.Visible -and ($s.Flags-band 64) -and -not($s.Flags-band 1)} 'Returning to desktop did not restore Hext'
  'PASS: three application coverage/exposure cycles and return to desktop restore compact Hext without opening another app.'
}finally{
  if($taskProbe -ne [IntPtr]::Zero){[void][HextRecoveryProbe]::DestroyWindow($taskProbe)}
  $taskShell.UndoMinimizeAll()
  if($taskOriginalForeground -ne $taskHext){[HextRecoveryProbe]::Focus($taskOriginalForeground)}
  if($taskPreviousDpi -ne [IntPtr]::Zero){[void][HextRecoveryProbe]::SetThreadDpiAwarenessContext($taskPreviousDpi)}
}
