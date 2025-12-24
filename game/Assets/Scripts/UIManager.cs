using UnityEngine;
using UnityEngine.UI;
using TMPro;

using System;
using System.IO.Ports;

public class UIManager: MonoBehaviour
{
    public GameObject mainPanel;

    public GameObject settingsPanel;
    public TMP_Dropdown portDropdown;
    public TMP_InputField baudRateInputField;
    public TMP_InputField thresholdInputField;
    public AudioClip bgmAudioClip;
    public AudioClip bellAudioClip;

    private AudioSource _as;

    private string[] _ports = Array.Empty<string>();

    void Awake()
    {
        _as = GetComponent<AudioSource>();
    }

    void OnEnable()
    {
        refreshPorts();
    }

    public void refreshPorts()
    {
        _ports = SerialPort.GetPortNames();
        Array.Sort(_ports);

        portDropdown.ClearOptions();

        if (_ports.Length == 0)
        {
            portDropdown.options.Add(new TMP_Dropdown.OptionData("no ports"));
            portDropdown.value = 0;
            portDropdown.RefreshShownValue();
            return;
        }

        foreach (var p in _ports)
        {
            portDropdown.options.Add(new TMP_Dropdown.OptionData(p));
        }

        int idx = Array.IndexOf(_ports, GameConfig.Instance.port);
        portDropdown.value = (idx >= 0) ? idx : 0;
        portDropdown.RefreshShownValue();
    }

    public void onClickStart()
    {
        _as.loop = false;
        _as.clip = bellAudioClip;

        _as.Play();

        // if (!SerialManagerWin64.Instance.isSerialOpened()) {
        //     SerialManagerWin64.Instance.openSerial();
        // }
        
        mainPanel.SetActive(false);

        GameManager.Instance.gameStart();
    }

    public void onClickSettings()
    {
        mainPanel.SetActive(false);
        settingsPanel.SetActive(true);

        refreshPorts();
    }

    public void onClickCancel()
    {
        settingsPanel.SetActive(false);
        mainPanel.SetActive(true);
    }

    public void onClickApply()
    {
        if (_ports.Length == 0)
        {
            Debug.LogWarning("[UIManager] no ports to apply.");
        }
        else
        {
            string selectedPort = _ports[portDropdown.value];
            GameConfig.Instance.port = selectedPort;
            
            if (!string.IsNullOrEmpty(baudRateInputField.text))
            {
                GameConfig.Instance.baudRate = int.Parse(baudRateInputField.text);
            }
            
            if (!string.IsNullOrEmpty(thresholdInputField.text))
            {
                GameConfig.Instance.threshold = int.Parse(thresholdInputField.text);
            }
        }

        settingsPanel.SetActive(false);
        mainPanel.SetActive(true);
    }

    public void playBGM()
    {
        _as.clip = bgmAudioClip;
        _as.loop = true;

        _as.Play();
    }
}
