namespace WutGame;

/// A system should implement this interface and call RegisterRenderSystem in the constructor to register for render callbacks.
/// UnregisterRenderSystem should be called when the system should no longer receive render callbacks.
public interface IRenderSystem
{
    protected void RegisterRenderSystem()
    {
        GameManager._renderSystems.Add(this);
    }

    protected void UnregisterRenderSystem()
    {
        GameManager._renderSystems.Remove(this);
    }

    public void Render(ulong deltaTimeTicks);
}

