namespace WutGame;

internal static class NativeMethods
{
    internal delegate void LogMessageFn(LogLevel level, string message);

    internal static LogMessageFn LogMessage = null!;

    internal delegate Entity CreateEntityFn();

    internal static CreateEntityFn CreateEntity = null!;

    internal delegate void DestroyEntityFn(Entity e);

    internal static DestroyEntityFn DestroyEntity = null!;

}

