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

    ~GameObject()
    {
        ReleaseUnmanagedResources();
    }

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
