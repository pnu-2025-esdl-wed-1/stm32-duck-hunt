using UnityEngine;
using UnityEngine.InputSystem;

/// <summary>
/// 게임 진행 관련 로직을 처리하는 매니저
/// </summary>
public class GameManager: MonoBehaviour
{
    void Update()
    {
        if (Keyboard.current != null && Keyboard.current.escapeKey.wasPressedThisFrame)
        {
            QuitGame();
        }
    }


    public void QuitGame()
    {
#if UNITY_EDITOR
        // 에디터에서 게임을 실행하는 경우
        UnityEditor.EditorApplication.isPlaying = false;
#else
        Application.Quit();
#endif
    }
}
