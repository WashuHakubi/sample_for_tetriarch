using System.Runtime.InteropServices;

namespace WutGame;

public static class GameManager
{
    public enum LogLevel : uint
    {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warn = 3,
        Error = 4,
        Critical = 5,
    };

    [StructLayout(LayoutKind.Explicit)]
    public struct GameInitializeOptions
    {
        // void (*logMessage) (LogLevel level, char const* msg);
        [FieldOffset(0)]
        public IntPtr logMessagePtr;
    };

    public delegate void LogMessageFn(LogLevel level, string message);
    private static LogMessageFn? logMessage;

    public static void LogMessage(LogLevel level, string message)
    {
        if (logMessage != null)
        {
            logMessage(level, message);
        }
    }

    [UnmanagedCallersOnly]
    public static void Initialize(GameInitializeOptions opts)
    {
        logMessage = Marshal.GetDelegateForFunctionPointer<LogMessageFn>(opts.logMessagePtr);
        if (logMessage == null)
        {
            Console.Error.WriteLine("Failed to get native log handling function.");
        }

        Log.Info($"C# {nameof(GameManager)}.{nameof(Initialize)} called");
    }
}
