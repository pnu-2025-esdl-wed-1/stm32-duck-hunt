using UnityEngine;

public class GameConfig: MonoBehaviour
{
    public static GameConfig Instance { get; private set; }

    public string port = "COMX";
    public int baudRate = 115200;
    public int threshold = -20; // 이후 조정

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
