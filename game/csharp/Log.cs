namespace WutGame;

public static class Log
{
    public static void Info(string message)
    {
        NativeMethods.LogMessage(LogLevel.Info, message);
    }

    public static void Warning(string message)
    {
        NativeMethods.LogMessage(LogLevel.Warn, message);
    }

    public static void Error(string message)
    {
        NativeMethods.LogMessage(LogLevel.Error, message);
    }

    public static void Critical(string message)
    {
        NativeMethods.LogMessage(LogLevel.Critical, message);
    }
}

