using UnityEngine;
using System.Collections;

public class DogController: MonoBehaviour
{
    public float walkingSpeed = 2.0f;
    public Sprite dogFindDuckSprite;
    public Sprite dogJumping0Sprite;
    public Sprite dogJumping1Sprite;
    public Sprite dogGetDuckSprite;
    public AudioClip dogBarkAudioClip;
    public AudioClip dogGrinningAudioClip;

    private SpriteRenderer _sr;
    private Animator _anim;
    private AudioSource _as;

    void Awake()
    {
        _sr = GetComponent<SpriteRenderer>();
        _anim = GetComponent<Animator>();
        _as = GetComponent<AudioSource>();

        // 테스트
        // StartCoroutine(findDuck());
        // StartCoroutine(getDuck(0.0f));
        // StartCoroutine(laughing());
    }

    public IEnumerator findDuck()
    {
        if (!_sr)
        {
            _sr = GetComponent<SpriteRenderer>();
        }

        if (!_anim)
        {
            _anim = GetComponent<Animator>();
        }

        transform.position = new Vector3(-8.0f, -3.0f, 0.0f);
        _sr.sortingLayerName = "MiddleForeground";
        _sr.sortingOrder = 0;

        _anim.enabled = true;
        _anim.Play("DogWalking", 0, 0);

        float elapsed = 0f;
        while (elapsed < 3.0f)
        {
            elapsed += Time.deltaTime;
            transform.position += Vector3.right * walkingSpeed * Time.deltaTime;
            yield return null;
        }

        _anim.Play("DogSnipping", 0, 0);

        yield return new WaitForSeconds(2.0f);

        _as.clip = dogBarkAudioClip;
        _as.Play();

        _anim.enabled = false;
        _sr.sprite = dogFindDuckSprite;

        yield return new WaitForSeconds(1.0f);

        _sr.sprite = dogJumping0Sprite;

        elapsed  = 0f;
        while (elapsed < 0.5f)
        {
            var t = Time.deltaTime;

            elapsed += t;

            transform.position += Vector3.right * 4.0f * t;
            transform.position += Vector3.up * 6.0f * t;

            yield return null;
        }

        _sr.sprite = dogJumping1Sprite;
        _sr.sortingLayerName = "MiddleBackground";
        _sr.sortingOrder = 2;
        
        elapsed  = 0f;
        while (elapsed < 1.0f)
        {
            var t = Time.deltaTime;

            elapsed += t;

            transform.position += Vector3.right * 2.0f * t;
            transform.position += Vector3.down * 3.0f * t;

            yield return null;
        }
    }

    public IEnumerator getDuck(float posX)
    {
        transform.position = new Vector3(posX, -3.5f, 0.0f);
        _sr.sortingLayerName = "MiddleBackground";
        _sr.sortingOrder = 2;

        _anim.enabled = false;

        _sr.sprite = dogGetDuckSprite;

        float elapsed  = 0f;
        while (elapsed < 1.0f)
        {
            var t = Time.deltaTime;

            elapsed += t;
            transform.position += Vector3.up * 2.2f * t;

            yield return null;
        }

        yield return new WaitForSeconds(1.0f);
        
        elapsed  = 0f;
        while (elapsed < 1.0f)
        {
            var t = Time.deltaTime;

            elapsed += t;
            transform.position += Vector3.down * 2.2f * t;

            yield return null;
        }
    }

    public IEnumerator laughing()
    {
        transform.position = new Vector3(0.0f, -3.5f, 0.0f);
        _sr.sortingLayerName = "MiddleBackground";
        _sr.sortingOrder = 2;

        _anim.enabled = true;

        _anim.Play("DogLaughing", 0, 0);

        _as.clip = dogGrinningAudioClip;
        _as.Play();

        float elapsed  = 0f;
        while (elapsed < 2.0f)
        {
            var t = Time.deltaTime;

            elapsed += t;
            transform.position += Vector3.up * 1.1f * t;

            yield return null;
        }
    }
}
