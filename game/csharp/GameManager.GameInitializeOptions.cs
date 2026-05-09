using System.Runtime.InteropServices;

namespace WutGame;

internal static partial class GameManager
{
    /// Options and pointers to native methods. The layout of this must match the layout on the native side.
    [StructLayout(LayoutKind.Sequential)]
    private struct GameInitializeOptions
    {
        public ulong ticksPerUpdate;
        public IntPtr entityRegistry;

        public IntPtr logMessagePtr;
        public IntPtr createEntity;
        public IntPtr destroyEntity;
    };
}

