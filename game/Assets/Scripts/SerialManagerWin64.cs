using System;
using System.Collections;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO.Ports;
using System.Threading;

using UnityEngine;
using TMPro;

public class SerialManagerWin64: MonoBehaviour
{
    public static SerialManagerWin64 Instance { get; private set; }

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

    void Update()
    {
        while (_msgQueue.TryDequeue(out string msg))
        {
            ProcessMessage(msg);
        }
    }

    void OnApplicationQuit()
    {
        closeSerial();
    }

    public void openSerial()
    {
        try
        {
            string port = GameConfig.Instance.port;
            int baudRate = GameConfig.Instance.baudRate;

            _sp = new SerialPort(port, baudRate);
            _sp.NewLine = "\n";
            _sp.ReadTimeout = 50;

            _sp.Open();

            _running = true;
            _readThread = new Thread(readThread);

            _readThread.Start();

            StartCoroutine(writeRoutine());

            Debug.Log("[SerialManager] Opened " + port);
        }
        catch (Exception e)
        {
            Debug.LogWarning("[SerialManager] Failed to open: " + e);
        }
    }

    public bool isSerialOpened()
    {
        return _sp != null && _sp.IsOpen;
    }

    private void closeSerial()
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

    private void readThread()
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
                Debug.LogError("[SerialManager] Read Error: " + e);
                _running = false;
            }
        }
    }

    private IEnumerator writeRoutine()
    {
        while (true)
        {
            _sp.WriteLine("TYPE=200;SCORE=" + GameManager.Instance.getScore() + ";SHOTS=" + GameManager.Instance.getShots());

            yield return new WaitForSeconds(0.5f);
        }
    }

    private void ProcessMessage(string msg)
    {
        Dictionary<string, int> parsedMsg = parseMsg(msg);

        switch (parsedMsg["TYPE"])
        {
            case 100: // TYPE_TRIGGER
                SerialIPC.Instance.setInitVal(parsedMsg["AMB"]);

                break;
            case 102: // TYPE_PEAK
                SerialIPC.Instance.setTestVal(parsedMsg["AMB"]);

                break;
            default:
                Debug.Log("[SerialManager] Unknown Message Recieved: " + msg);

                break;
        }
    }

    private Dictionary<string, int> parseMsg(string msg)
    {
        Dictionary<string, int> parsedMsg = new Dictionary<string, int>();

        string[] pairs = msg.Split(';');

        foreach (string pair in pairs)
        {
            string[] k_v = pair.Split('=');
            if (k_v.Length != 2)
                continue;

            string key = k_v[0].Trim();
            string valueStr = k_v[1].Trim();

            if (int.TryParse(valueStr, out int value))
            {
                parsedMsg[key] = value;
            }
        }

        return parsedMsg;
    }
}
