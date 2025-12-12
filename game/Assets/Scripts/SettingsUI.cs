using System;
using System.IO.Ports;

using UnityEngine;
using UnityEngine.UI;
using TMPro;

/// <summary>
/// 게임 UI(세팅) 관련 작업을 처리하는 Behaviour
/// </summary>
public class SettingsUI: MonoBehaviour
{
    public GameObject mainPanel;
    public GameObject mainTarget;

    public GameObject settingsPanel;
    public TMP_Dropdown portDropdown;
    public TMP_InputField baudRateInputField;


    private string[] _ports = Array.Empty<string>();


    void OnEnable()
    {
        RefreshPorts();
    }


    public void OnClickStart()
    {
        mainPanel.SetActive(false);

        mainTarget.SetActive(true);
        SerialManagerWin64.Instance.OpenSerial();
    }

    public void OnClickSettings()
    {
        mainPanel.SetActive(false);
        settingsPanel.SetActive(true);

        RefreshPorts();
    }

    public void OnClickCancel()
    {
        settingsPanel.SetActive(false);
        mainPanel.SetActive(true);
    }

    public void OnClickApply()
    {
        if (_ports.Length == 0)
        {
            Debug.LogWarning("[SettingsUI] no ports to apply.");
        }
        else
        {
            string selectedPort = _ports[portDropdown.value];
            SerialConfig.Instance.port = selectedPort;
            SerialConfig.Instance.baudRate = int.Parse(baudRateInputField.text);
            Debug.Log("[SettingsUI] selected port: " + selectedPort);
        }

        settingsPanel.SetActive(false);
        mainPanel.SetActive(true);
    }

    public void RefreshPorts()
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

        int idx = Array.IndexOf(_ports, SerialConfig.Instance.port);
        portDropdown.value = (idx >= 0) ? idx : 0;
        portDropdown.RefreshShownValue();
    }
}
