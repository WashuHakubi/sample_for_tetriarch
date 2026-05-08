using System.Runtime.InteropServices;

namespace WutGame;

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

    public static unsafe int Initialize(IntPtr arg, int argLength)
    {
        if (argLength != Marshal.SizeOf(typeof(GameInitializeOptions)))
        {
            return -1;
        }

        var opts = (GameInitializeOptions*)arg.ToPointer();

        Fix(ref NativeMethods.LogMessage, opts->logMessagePtr);
        Fix(ref NativeMethods.LogMessage, opts->logMessagePtr);
        Fix(ref NativeMethods.CreateGameObjectInt, opts->createGameObjectPtr);
        Fix(ref NativeMethods.ReleaseGameObject, opts->releaseGameObjectPtr);
        Fix(ref NativeMethods.AcquireGameObject, opts->acquireGameObjectPtr);
        Fix(ref NativeMethods.GameObjectNameInt, opts->gameObjectNamePtr);

        Log.Info($"C# {nameof(GameManager)}.{nameof(Initialize)} called");

        using var go = new GameObject("my game object");
        Log.Warning($"C# access to: {go.Name}");

        return 0;
    }
}
