using UnityEngine;

/// <summary>
/// 현재 화면을 기준으로 좌우로 왔다리갔다리하는 테스트 이동 Behaviour
/// </summary>
public class TestMover: MonoBehaviour
{
    public float speed = 3f;


    private float _leftLimit;
    private float _rightLimit;
    
    private int _direction = 1; // 1 = left


    void Start()
    {
        (_leftLimit, _rightLimit) = GetCameraLimit();
    }

    void Update()
    {
        transform.position += Vector3.right * _direction * speed * Time.deltaTime;

        if (transform.position.x + 1 > _rightLimit)
        {
            _direction = -1;
        }
        else if (transform.position.x - 1 < _leftLimit)
        {
            _direction = 1;
        }
    }


    private (float left, float right) GetCameraLimit()
    {
        Camera cam = Camera.main;

        // 오브젝트의 z 좌표 기준으로 카메라에서 거리 계산
        float distance = transform.position.z - cam.transform.position.z;

        // Viewport 범위는 0 ~ 1
        float leftLimit = cam.ViewportToWorldPoint(new Vector3(0f, 0.5f, distance)).x;
        float rightLimit = cam.ViewportToWorldPoint(new Vector3(1f, 0.5f, distance)).x;

        return (leftLimit, rightLimit);
    }
}
