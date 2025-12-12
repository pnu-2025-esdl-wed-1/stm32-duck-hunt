using UnityEngine;
using UnityEngine.InputSystem;

/// <summary>
/// HIT 모드 껐다 켰다하는 컨트롤러
/// </summary>
public class HitTestController: MonoBehaviour
{
    // 싱글톤 패턴
    public static HitTestController Instance { get; private set; }


    public GameObject blackScreen;
    public GameObject[] rootsToHide;


    private GameObject _currentTarget;


    void Awake()
    {
        if (Instance != null && Instance != this)
        {
            Destroy(this.gameObject);
            return;
        }

        Instance = this;
    }

    void Update()
    {
        if (Mouse.current != null && Mouse.current.rightButton.wasPressedThisFrame)
        {
            ExitHitTest();
        }
    }


    public void HitTest(GameObject target)
    {
        _currentTarget = target;

        blackScreen?.SetActive(true);

        foreach (var root in rootsToHide)
            root?.SetActive(false);

        _currentTarget.SetActive(true);
        _currentTarget.GetComponent<TestMover>().enabled = false;
    }

    public void ExitHitTest()
    {
        if (!blackScreen.activeInHierarchy) return;
        
        blackScreen?.SetActive(false);

        foreach (var root in rootsToHide)
            root?.SetActive(true);
            
        _currentTarget.GetComponent<TestMover>().enabled = true;

        _currentTarget = null;
    }
}
