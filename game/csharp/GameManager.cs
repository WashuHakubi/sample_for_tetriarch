using System.Runtime.InteropServices;

namespace WutGame;

public static class GameManager
{
    /// Options and pointers to native methods. The layout of this must match the layout on the native side.
    [StructLayout(LayoutKind.Sequential)]
    private struct GameInitializeOptions
    {
        public ulong ticksPerUpdate;
        public IntPtr entityRegistry;

        public IntPtr logMessagePtr;
        public IntPtr createEntity;
        public IntPtr destroyEntity;
    };

    [StructLayout(LayoutKind.Sequential)]
    private struct GameInitializeResults
    {
        public IntPtr onEntityConstructed;
        public IntPtr onEntityDestroyed;
    }

    private static ulong ticksPerUpdate;

    public static ulong TicksPerUpdate => ticksPerUpdate;

    private static void Fix<T>(ref T fn, IntPtr handle)
    {
        fn = Marshal.GetDelegateForFunctionPointer<T>(handle);
    }

    [UnmanagedCallersOnly]
    private static void OnEntityConstructed(Entity e)
    {
        Log.Info($"Entity created: {e}");
    }

    [UnmanagedCallersOnly]
    private static void OnEntityDestroyed(Entity e)
    {
        Log.Info($"Entity destroyed: {e}");

    }

    [UnmanagedCallersOnly]
    private static unsafe int Initialize(GameInitializeOptions* opts, int gameInitOptsSize)
    {
        if (gameInitOptsSize != Marshal.SizeOf(typeof(GameInitializeOptions)))
        {
            Console.Error.WriteLine($"GameInitializeOptions size mismatch. Expected: {Marshal.SizeOf(typeof(GameInitializeOptions))}, got: {gameInitOptsSize}");
            return -1;
        }

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
