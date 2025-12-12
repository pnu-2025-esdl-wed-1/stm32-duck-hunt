using UnityEngine;

public class SerialConfig: MonoBehaviour
{
    public static SerialConfig Instance { get; private set; }

    public string port = "COM29";
    public int baudRate = 9600;

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
}
