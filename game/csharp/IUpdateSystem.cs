namespace WutGame;

public interface IUpdateSystem
{
    protected void RegisterUpdateSystem()
    {
        GameManager._updateSystems.Add(this);
    }

    protected void UnregisterUpdateSystem()
    {
        GameManager._updateSystems.Remove(this);
    }

    public void Update(ulong deltaTimeTicks);
}

