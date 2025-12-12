using UnityEngine;
using UnityEngine.InputSystem;

/// <summary>
/// 클릭으로 HitTest를 수행하는 객체의 Behaviour
/// </summary>
[RequireComponent(typeof(Collider2D))] // Unity에서는 마우스 클릭을 raycasting으로 인식하기 때문에 콜라이더 컴포넌트가 반드시 필요
public class ClickableTarget: MonoBehaviour
{
    private Camera _cam;

    void Start()
    {
        _cam = Camera.main;
    }

    void Update()
    {
        // 입력 관련 시스템이 최신 버전으로 바뀌면서 기존의 레거시 입력으로 메시지 기반의 OnMouseDown 같은 함수는 못 씀...
        if (Mouse.current != null && Mouse.current.leftButton.wasPressedThisFrame)
        {
            Vector2 mousePos = Mouse.current.position.ReadValue();
            Ray ray = _cam.ScreenPointToRay(mousePos);

            RaycastHit2D hit = Physics2D.GetRayIntersection(ray, Mathf.Infinity, 1 << 0);

            if (hit.collider != null && hit.collider.gameObject == this.gameObject)
            {
                HitTestController.Instance?.HitTest(this.gameObject);
            }
        }
    }
}
