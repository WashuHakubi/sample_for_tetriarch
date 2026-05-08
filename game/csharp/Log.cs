namespace WutGame;

public static class Log
{
    public static void Info(string message)
    {
        GameManager.LogMessage(GameManager.LogLevel.Info, message);
    }

    public static void Warning(string message)
    {
        GameManager.LogMessage(GameManager.LogLevel.Warn, message);
    }

    public static void Error(string message)
    {
        GameManager.LogMessage(GameManager.LogLevel.Error, message);
    }

    public static void Critical(string message)
    {
        GameManager.LogMessage(GameManager.LogLevel.Critical, message);
    }
}

