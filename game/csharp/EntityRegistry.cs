namespace WutGame;

public static class EntityRegistry
{
    public static Entity Create()
    {
        // OnEntityConstructed will be called
        var e = NativeMethods.CreateEntity();
        return e;
    }

    public static void Destroy(Entity e)
    {
        // OnEntityDestroyed will be called
        NativeMethods.DestroyEntity(e);
    }

    public static ComponentStore<T> GetStore<T>()
    {

        if (!_stores.TryGetValue(typeof(T), out var istore))
        {
            istore = new ComponentStore<T>();
            _stores.Add(typeof(T), istore);
        }

        var store = (ComponentStore<T>)istore!;
        return store;
    }

    internal static void OnEntityConstructed(Entity e)
    {
        _entities.Add(e);
    }

    internal static void OnEntityDestroyed(Entity e)
    {
        if (_entities.Remove(e))
        {
            // Remove components
            foreach (var store in _stores)
            {
                store.Value.Remove(e);
            }
        }
    }

    private static readonly HashSet<Entity> _entities = new();
    private static readonly Dictionary<Type, IComponentStore> _stores = new();
}

