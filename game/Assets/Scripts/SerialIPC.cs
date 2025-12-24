using UnityEngine;

public class SerialIPC: MonoBehaviour
{
    public static SerialIPC Instance { get; private set; }

    private bool _initValInit = false;
    private int _initVal = 0;
    private bool _testValInit = false;
    private int _testVal = 0;

    void Awake()
    {
        if (Instance != null && Instance != this)
        {
            Destroy(gameObject);
            return;
        }

        Instance = this;
        DontDestroyOnLoad(gameObject);
    }

    public void setInitVal(int initVal)
    {
        _initVal = initVal;
        _initValInit = true;
    }

    public int getInitVal()
    {
        return _initVal;
    }

    public bool isInitValInit()
    {
        return _initValInit;
    }

    public void setTestVal(int testVal)
    {
        _initValInit = false;

        _testVal = testVal;
        _testValInit = true;
    }

    public int getTestVal()
    {
        _testValInit = false;
        
        return _testVal;
    }

    public bool isTestValInit()
    {
        return _testValInit;
    }
}
