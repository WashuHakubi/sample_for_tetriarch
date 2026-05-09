namespace WutGame;

public class ComponentStore<T> : IComponentStore
{
    public void Add(Entity e, T value)
    {
        _components.Add(e, value);
    }

    public bool Contains(Entity e)
    {
        return _components.ContainsKey(e);
    }

    public bool Remove(Entity e)
    {
        return _components.Remove(e);
    }

    public T this[Entity key]
    {
        get => _components[key];
        set => _components[key] = value;
    }

    // Very crude storage of entities and component data.
    private readonly Dictionary<Entity, T> _components = new();
}

