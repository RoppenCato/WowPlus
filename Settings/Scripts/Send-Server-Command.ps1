# Types a command into the world server's window, as if you had typed it there yourself.
#
#     powershell -NoProfile -ExecutionPolicy Bypass -File Settings\Scripts\Send-Server-Command.ps1 "saveall"
#     powershell -NoProfile -ExecutionPolicy Bypass -File Settings\Scripts\Send-Server-Command.ps1 "server shutdown 5"
#
# Used by Test-Bots.py to have all characters saved before it reads the database. Run it as its own
# PowerShell process, as above: it has to let go of its own console window to reach the server's.
param([Parameter(Mandatory = $true)][string]$Text)

$server = Get-Process worldserver -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $server) { Write-Output 'The world server is not running.'; exit 1 }

Add-Type -TypeDefinition @'
using System; using System.Runtime.InteropServices;
public static class ServerConsole {
  [DllImport("kernel32.dll", SetLastError=true)] static extern bool FreeConsole();
  [DllImport("kernel32.dll", SetLastError=true)] static extern bool AttachConsole(uint pid);
  [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)] static extern IntPtr CreateFile(string n, uint a, uint s, IntPtr sec, uint c, uint f, IntPtr t);
  [StructLayout(LayoutKind.Explicit, CharSet=CharSet.Unicode)] struct KEY { [FieldOffset(0)] public int down; [FieldOffset(4)] public ushort rep; [FieldOffset(6)] public ushort vk; [FieldOffset(8)] public ushort sc; [FieldOffset(10)] public char ch; [FieldOffset(12)] public uint ctrl; }
  [StructLayout(LayoutKind.Explicit)] struct REC { [FieldOffset(0)] public ushort type; [FieldOffset(4)] public KEY key; }
  [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)] static extern bool WriteConsoleInput(IntPtr h, REC[] b, uint n, out uint w);
  public static int Send(uint pid, string text) {
    FreeConsole();
    if (!AttachConsole(pid)) return 2;
    IntPtr h = CreateFile("CONIN$", 0xC0000000, 3, IntPtr.Zero, 3, 0, IntPtr.Zero);
    if (h == new IntPtr(-1)) return 3;
    var recs = new REC[text.Length * 2];
    for (int i = 0; i < text.Length; i++) {
      char c = text[i]; ushort vk = (ushort)(c == '\r' ? 0x0D : 0);
      recs[i*2] = new REC { type = 1, key = new KEY { down = 1, rep = 1, vk = vk, ch = c } };
      recs[i*2+1] = new REC { type = 1, key = new KEY { down = 0, rep = 1, vk = vk, ch = c } };
    }
    uint w; bool ok = WriteConsoleInput(h, recs, (uint)recs.Length, out w); FreeConsole();
    return ok ? 0 : 4;
  }
}
'@
exit [ServerConsole]::Send([uint32]$server.Id, $Text + "`r")
