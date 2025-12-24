using UnityEngine;
using System.Collections;

public enum DuckState
{
    idle,
    inBush,
    flying,
    hitTest,
    flyAway,
    shot,
    falling
}

public class DuckController: MonoBehaviour
{
    public Sprite duckShotSprite;
    public GameObject hitBox;
    public AudioClip duckFlapAudioClip;
    public AudioClip duckQuackAudioClip;
    public AudioClip duckScreamAudioClip;

    private SpriteRenderer _sr;
    private Animator _anim;
    private AudioSource _as;

    private DuckState _curState = DuckState.idle;
    private Vector3 _curDir;

    public void Awake()
    {
        _sr = GetComponent<SpriteRenderer>();
        _anim = GetComponent<Animator>();
        _as = GetComponent<AudioSource>();

        // 테스트
        // _curState = DuckState.shot;
        // StartCoroutine(fly());
        // StartCoroutine(flyAway());
        // shot();
        // StartCoroutine(fall());
    }

    public IEnumerator fly()
    {
        if (_curState != DuckState.idle)
            yield break;

        float startX = Random.Range(-4.0f, 4.0f);
        transform.position = new Vector2(startX, -3.0f);

        _anim.enabled = true;
        _anim.Play("DuckFlyingD");

        _curState = DuckState.inBush;

        Camera cam = Camera.main;

        float zDist = transform.position.z - cam.transform.position.z;
        Vector3 bl = cam.ViewportToWorldPoint(new Vector3(0f, 0f, zDist)); // bottom-left
        Vector3 tr = cam.ViewportToWorldPoint(new Vector3(1f, 1f, zDist)); // top-right

        // 이미지 크기 보간
        Vector2 ext = _sr.bounds.extents;

        float minX = bl.x + ext.x;
        float maxX = tr.x - ext.x;
        float minY = -1.5f;
        float maxY = tr.y - ext.y;

        // 방향
        float xSign = (Random.value < 0.5f) ? -1f : 1f;
        float ySign = 1f;

        // 기울기
        float xMag = Random.Range(0.6f, 1.0f);
        float yMag = Random.Range(0.4f, 1.0f);

        _curDir = new Vector3(xSign * xMag, ySign * yMag, 0f).normalized;

        const float horizThreshold = 0.35f; // y가 이 값보다 작으면 DuckFlyingH 애니메이션 사용

        float elapsed = 0f;

        _as.clip = duckFlapAudioClip;
        _as.Play();

        while (_curState == DuckState.flying || _curState == DuckState.inBush || _curState == DuckState.hitTest)
        {
            yield return new WaitWhile(() => _curState == DuckState.hitTest);

            elapsed += Time.deltaTime;

            Vector3 pos = transform.position;
            pos += _curDir * GameManager.Instance.getDuckSpeed() * Time.deltaTime;

            bool bounced = false;

            // 좌우 경계
            if (pos.x <= minX)
            {
                pos.x = minX;
                _curDir.x = -_curDir.x;
                bounced = true;
            }
            else if (pos.x >= maxX)
            {
                pos.x = maxX;
                _curDir.x = -_curDir.x;
                bounced = true;
            }

            // 상하 경계
            if (pos.y <= minY && _curState == DuckState.flying)
            {
                pos.y = minY;
                _curDir.y = -_curDir.y;
                bounced = true;
            }
            else if (pos.y >= maxY)
            {
                pos.y = maxY;
                _curDir.y = -_curDir.y;
                bounced = true;
            }

            // 수치 안정화
            if (bounced)
            {
                _curDir = _curDir.normalized;
                _curState = DuckState.flying;
            }
            // 랜덤 방향 전환
            else if(elapsed >= 1.0f && Random.value <= (0.01f + GameManager.Instance.getDuckSpeed() * 0.001f))
            {
                _as.clip = duckQuackAudioClip;
                _as.Play();

                xSign = (Random.value < 0.5f) ? -1f : 1f;
                ySign = (Random.value < 0.5f) ? -1f : 1f;

                xMag = Random.Range(0.6f, 1.0f);
                yMag = Random.Range(0.4f, 1.0f);

                _curDir = new Vector3(xSign * xMag, ySign * yMag, 0f).normalized;

                elapsed = 0f;
            }

            transform.position = pos;

            if (_curDir.x < 0f) _sr.flipX = true;
            else if (_curDir.x > 0f) _sr.flipX = false;

            string clip = (_curDir.y < horizThreshold) ? "DuckFlyingH" : "DuckFlyingD";
            _anim.Play(clip);

            yield return null;
        }
    }

    public IEnumerator flyAway()
    {
        if (_curState != DuckState.flying && _curState != DuckState.inBush)
            yield break;

        _anim.enabled = true;
        _anim.Play("DuckFlyingH");

        _curState = DuckState.flyAway;

        Camera cam = Camera.main;

        float zDist = transform.position.z - cam.transform.position.z;
        Vector3 tr = cam.ViewportToWorldPoint(new Vector3(1f, 1f, zDist));

        Vector2 ext = _sr.bounds.extents;

        _curDir = Vector3.right;

        _as.clip = duckFlapAudioClip;
        _as.Play();

        while (transform.position.x <= tr.x + ext.x + 0.1f)
        {
            transform.position += _curDir * 16.0f * Time.deltaTime;

            yield return null;
        }

        _curState = DuckState.idle;
    }

    public void hitTest()
    {
        _curState = DuckState.hitTest;

        hitBox.SetActive(true);
    }

    public void exitHitTest()
    {
        hitBox.SetActive(false);

        _curState = DuckState.flying;
    }

    public IEnumerator shot()
    {
        if (_curState != DuckState.flying && _curState != DuckState.inBush)
            yield break;

        _as.clip = duckScreamAudioClip;
        _as.Play();
        
        _curState = DuckState.shot;

        _anim.enabled = false;

        _curDir = Vector3.zero;

        _sr.sprite = duckShotSprite;

        yield return new WaitForSeconds(0.5f);
    }

    public IEnumerator fall()
    {
        if (_curState != DuckState.shot)
            yield break;
        
        _curState = DuckState.falling;

        _anim.enabled = true;
        _anim.Play("DuckFalling");

        _curDir = Vector3.down;

        while (transform.position.y >= -3.0)
        {
            transform.position += _curDir * 8.0f * Time.deltaTime;

            yield return null;
        }

        _curState = DuckState.idle;
    }

    public Vector3 getCurPos()
    {
        return transform.position;
    }
}
