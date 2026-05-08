namespace WutGame;

public class GameObject : IDisposable
{
    private IntPtr _handle;

    public GameObject()
    {
        _handle = NativeMethods.CreateGameObject(null);
    }

    public GameObject(string name)
    {
        _handle = NativeMethods.CreateGameObject(name);
    }

    internal GameObject(IntPtr handle)
    {
        _handle = handle;
        NativeMethods.AcquireGameObject(_handle);
    }

    ~GameObject()
    {
        ReleaseUnmanagedResources();
    }

    public string Name => NativeMethods.GameObjectName(_handle) ?? "[null]";

    private void ReleaseUnmanagedResources()
    {
        NativeMethods.ReleaseGameObject(_handle);
        _handle = IntPtr.Zero;
    }

    public void Dispose()
    {
        ReleaseUnmanagedResources();
        GC.SuppressFinalize(this);
    }
}
