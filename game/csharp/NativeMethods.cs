using System.Runtime.InteropServices;

namespace WutGame;

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

