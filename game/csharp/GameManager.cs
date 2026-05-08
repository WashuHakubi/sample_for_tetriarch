using System.Runtime.InteropServices;

namespace WutGame;

public static class GameManager
{
    [UnmanagedCallersOnly]
    public static void Initialize()
    {
        Console.WriteLine("C# Game manager initialized");
    }
}
