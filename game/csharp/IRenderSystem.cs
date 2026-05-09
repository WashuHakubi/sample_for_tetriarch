namespace WutGame;

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

