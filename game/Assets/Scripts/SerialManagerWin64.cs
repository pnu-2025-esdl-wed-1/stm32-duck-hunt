using System;
using System.Collections.Concurrent;
using System.IO.Ports;
using System.Threading;

using UnityEngine;


/// <summary>
/// 시리얼 통신을 처리하는 매니저
/// Windows(AMD64)에서만 동작을 보장
/// </summary>
public class SerialManagerWin64: MonoBehaviour
{
    // 싱글톤
    public static SerialManagerWin64 Instance { get; private set; }


    public GameObject currentTarget;


    private SerialPort _sp;

    private Thread _readThread;
    private bool _running = false;

    private ConcurrentQueue<string> _msgQueue = new ConcurrentQueue<string>();


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

    void OnApplicationQuit()
    {
        CloseSerial();
    }


    public void OpenSerial()
    {
        try
        {
            string port = SerialConfig.Instance.port;
            int baudRate = SerialConfig.Instance.baudRate;

            _sp = new SerialPort(port, baudRate);
            _sp.NewLine = "\n";
            _sp.ReadTimeout = 50;

            _sp.Open();

            _running = true;
            _readThread = new Thread(ReadLoop);

            _readThread.Start();

            Debug.Log("[Serial] Opened " + port);
        }
        catch (Exception e)
        {
            Debug.LogError("[Serial] Failed to open: " + e);
        }
    }


    private void CloseSerial()
    {
        try
        {
            _running = false;

            if (_readThread != null && _readThread.IsAlive)
                _readThread.Join(100);

            if (_sp != null && _sp.IsOpen)
                _sp.Close();
        }
        catch
        {
            // pass
        }
    }

    private void ReadLoop()
    {
        while (_running)
        {
            try
            {
                string line = _sp.ReadLine().Trim();
                if (!string.IsNullOrEmpty(line))
                {
                    _msgQueue.Enqueue(line);
                }
            }
            catch (TimeoutException)
            {
                // pass
            }
            catch (Exception e)
            {
                Debug.LogError("[Serial] Read Error: " + e);
                _running = false;
            }
        }
    }

    void Update()
    {
        while (_msgQueue.TryDequeue(out string msg))
        {
            ProcessMessage(msg);
        }
    }

    private void ProcessMessage(string msg)
    {
        if (msg.StartsWith("TYPE"))
        {
            Debug.Log("[Serial] TRIG received: " + msg);
            _sp.WriteLine("[Serial] TRIG activated: " + msg);

            HitTestController.Instance.HitTest(currentTarget);
        }
    }
}
