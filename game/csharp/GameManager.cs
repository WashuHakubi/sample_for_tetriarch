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

    internal delegate IntPtr CreateGameObjectFn([MarshalAs(UnmanagedType.LPStr)] string? name);

    internal static CreateGameObjectFn CreateGameObject = null!;

    internal delegate IntPtr ReleaseGameObjectFn(IntPtr handle);

    internal static ReleaseGameObjectFn ReleaseGameObject = null!;
}

public static class GameManager
{
    [StructLayout(LayoutKind.Sequential)]
    public struct GameInitializeOptions
    {
        // void (*log_message) (LogLevel level, char_t const* msg);
        public IntPtr logMessagePtr;

        // game_object_ptr* (*create_game_object) (char_t const* msg);
        public IntPtr createGameObjectPtr;

        // void (*release_game_object)(game_object_ptr*);
        public IntPtr releaseGameObjectPtr;
    };

    [UnmanagedCallersOnly]
    public static void Initialize(GameInitializeOptions opts)
    {
        NativeMethods.LogMessage = Marshal.GetDelegateForFunctionPointer<NativeMethods.LogMessageFn>(opts.logMessagePtr);
        NativeMethods.CreateGameObject = Marshal.GetDelegateForFunctionPointer<NativeMethods.CreateGameObjectFn>(opts.createGameObjectPtr);
        NativeMethods.ReleaseGameObject = Marshal.GetDelegateForFunctionPointer<NativeMethods.ReleaseGameObjectFn>(opts.releaseGameObjectPtr);

        Log.Info($"C# {nameof(GameManager)}.{nameof(Initialize)} called");

        using var go = new GameObject("my game object");
    }
}
