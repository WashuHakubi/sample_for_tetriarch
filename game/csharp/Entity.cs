namespace WutGame;

public struct Entity
{
    public Entity(uint v)
    {
        value = v;
    }


    public override string ToString()
    {
        return value.ToString();
    }

    private uint value;
}

