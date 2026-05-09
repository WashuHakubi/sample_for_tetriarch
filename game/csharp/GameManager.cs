using System.Runtime.InteropServices;

namespace WutGame;

internal static partial class GameManager
{
    private static ulong ticksPerUpdate;

    public static ulong TicksPerUpdate => ticksPerUpdate;

    internal static List<IUpdateSystem> _updateSystems = new();

    internal static List<IRenderSystem> _renderSystems = new();

    private static void Fix<T>(ref T fn, IntPtr handle)
    {
        fn = Marshal.GetDelegateForFunctionPointer<T>(handle);
    }

    [UnmanagedCallersOnly]
    private static void OnEntityConstructed(Entity e)
    {
        Log.Trace($"Entity created: {e}");
        EntityRegistry.OnEntityConstructed(e);
    }

    [UnmanagedCallersOnly]
    private static void OnEntityDestroyed(Entity e)
    {
        Log.Trace($"Entity destroyed: {e}");
        EntityRegistry.OnEntityDestroyed(e);

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
        Fix(ref NativeMethods.CreateEntity, opts->createEntity);
        Fix(ref NativeMethods.DestroyEntity, opts->destroyEntity);

        Log.Trace($"C# {nameof(GameManager)}.{nameof(Initialize)} called");
        return 0;
    }

    [UnmanagedCallersOnly]
    private static void Update(ulong deltaTimeTicks)
    {
        foreach (var system in _updateSystems)
        {
            system.Update(deltaTimeTicks);
        }
    }

    [UnmanagedCallersOnly]
    private static void Render(ulong deltaTimeTicks)
    {
        foreach (var system in _renderSystems)
        {
            system.Render(deltaTimeTicks);
        }
    }

    [UnmanagedCallersOnly]
    private static void Shutdown()
    {
        Log.Trace($"C# {nameof(GameManager)}.{nameof(Shutdown)} called");
    }
}
