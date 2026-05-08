using System.Runtime.InteropServices;

namespace WutGame;

public static class GameManager
{
    /// Options and pointers to native methods. The layout of this must match the layout on the native side.
    [StructLayout(LayoutKind.Sequential)]
    private struct GameInitializeOptions
    {
        public ulong ticksPerUpdate;
        public IntPtr logMessagePtr;
    };

    private static ulong ticksPerUpdate;

    public static ulong TicksPerUpdate => ticksPerUpdate;

    private static void Fix<T>(ref T fn, IntPtr handle)
    {
        fn = Marshal.GetDelegateForFunctionPointer<T>(handle);
    }

    [UnmanagedCallersOnly]
    private static unsafe int Initialize(IntPtr arg, int argLength)
    {
        if (argLength != Marshal.SizeOf(typeof(GameInitializeOptions)))
        {
            return -1;
        }

        var opts = (GameInitializeOptions*)arg.ToPointer();

        ticksPerUpdate = opts->ticksPerUpdate;

        Fix(ref NativeMethods.LogMessage, opts->logMessagePtr);

        Log.Info($"C# {nameof(GameManager)}.{nameof(Initialize)} called");

        return 0;
    }

    [UnmanagedCallersOnly]
    private static void Update(ulong deltaTimeTicks)
    {

    }

    [UnmanagedCallersOnly]
    private static void Render(ulong deltaTimeTicks)
    {

    }

    [UnmanagedCallersOnly]
    private static void Shutdown()
    {
        Log.Info($"C# {nameof(GameManager)}.{nameof(Shutdown)} called");
    }
}
