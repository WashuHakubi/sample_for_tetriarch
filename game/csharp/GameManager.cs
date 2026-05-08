using System.Runtime.InteropServices;

namespace WutGame;

public enum LogLevel : uint
{
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4,
    Critical = 5,
};

internal static class NativeMethods
{
    internal delegate void LogMessageFn(LogLevel level, string message);

    internal static LogMessageFn LogMessage = null!;

    internal delegate IntPtr CreateGameObjectFn(IntPtr name);

    internal static CreateGameObjectFn CreateGameObjectInt = null!;

    internal static IntPtr CreateGameObject(string? name)
    {
        IntPtr namePtr = IntPtr.Zero;
        if (name != null)
        {
            namePtr = Marshal.StringToCoTaskMemUTF8(name);
        }

        var handle = CreateGameObjectInt(namePtr);
        Marshal.ZeroFreeCoTaskMemUTF8(namePtr);
        return handle;
    }

    internal delegate IntPtr ReleaseGameObjectFn(IntPtr handle);

    internal static ReleaseGameObjectFn ReleaseGameObject = null!;

    internal delegate void AcquireGameObjectFn(IntPtr handle);

    internal static AcquireGameObjectFn AcquireGameObject = null!;

    internal delegate IntPtr GameObjectNameFn(IntPtr handle);

    internal static GameObjectNameFn GameObjectNameInt = null!;

    internal static string? GameObjectName(IntPtr handle)
    {
        var namePtr = GameObjectNameInt(handle);
        if (namePtr != IntPtr.Zero)
        {
            return Marshal.PtrToStringUTF8(namePtr);
        }
        return null;
    }
}

public static class GameManager
{
    /// Options and pointers to native methods. The layout of this must match the layout on the native side.
    [StructLayout(LayoutKind.Sequential)]
    public struct GameInitializeOptions
    {
        public IntPtr logMessagePtr;

        public IntPtr createGameObjectPtr;

        public IntPtr releaseGameObjectPtr;

        public IntPtr acquireGameObjectPtr;

        public IntPtr gameObjectNamePtr;
    };

    private static void Fix<T>(ref T fn, IntPtr handle)
    {
        fn = Marshal.GetDelegateForFunctionPointer<T>(handle);
    }

    [UnmanagedCallersOnly]
    public static void Initialize(GameInitializeOptions opts)
    {
        Fix(ref NativeMethods.LogMessage, opts.logMessagePtr);
        Fix(ref NativeMethods.LogMessage, opts.logMessagePtr);
        Fix(ref NativeMethods.CreateGameObjectInt, opts.createGameObjectPtr);
        Fix(ref NativeMethods.ReleaseGameObject, opts.releaseGameObjectPtr);
        Fix(ref NativeMethods.AcquireGameObject, opts.acquireGameObjectPtr);
        Fix(ref NativeMethods.GameObjectNameInt, opts.gameObjectNamePtr);

        Log.Info($"C# {nameof(GameManager)}.{nameof(Initialize)} called");

        using var go = new GameObject("my game object");
        Log.Warning($"C# access to: {go.Name}");
    }
}
