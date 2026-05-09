namespace WutGame;

/// A system should implement this interface and call RegisterUpdateSystem in the constructor to register for update callbacks.
/// UnregisterUpdateSystem should be called when the system should no longer receive update callbacks.
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

