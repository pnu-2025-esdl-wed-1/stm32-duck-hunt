using UnityEngine;
using UnityEngine.InputSystem;
using TMPro;
using System.Collections;

public enum GameStatus
{
    idle,
    wait,
    playing,
    over
}

public class GameManager: MonoBehaviour
{
    public static GameManager Instance { get; private set; }

    public GameObject blackScreen;
    public GameObject hitBox;
    public DogController dog;
    public DuckController duck;
    public Canvas inGameUICanvas;
    public ScoreTextController scoreTextPrefab;
    public GameObject gameOverText;
    public GameObject mainPanel;
    public GameObject serialAmbientText;
    public UIManager uiManager;
    public GameObject scoreText;

    public float initialDuckSpeed = 8.0f;

    private Camera _cam;
    private AudioSource _as;

    private GameStatus _curStatus = GameStatus.idle;
    private int _score = 0;
    private int _shots = 3;
    private float _duckSpeed = 8.0f;
    private int _strict = 0;

    void Awake()
    {
        if (Instance != null && Instance != this)
        {
            Destroy(gameObject);
            return;
        }

        Instance = this;
        DontDestroyOnLoad(gameObject);

        _cam = Camera.main;
        _as = GetComponent<AudioSource>();

        // 테스트
        // gameStart();
    }

    void Update()
    {
        if (Keyboard.current != null && Keyboard.current.escapeKey.wasPressedThisFrame)
        {
            quitGame();
        }

        if (_curStatus == GameStatus.playing && Mouse.current != null && Mouse.current.leftButton.wasPressedThisFrame)
        {
            StartCoroutine(clickHitTest(Mouse.current.position.ReadValue()));
        }

        if (_curStatus == GameStatus.playing && SerialIPC.Instance.isInitValInit())
        {
            StartCoroutine(serialHitTest(SerialIPC.Instance.getInitVal()));
        }
        
        if (_curStatus == GameStatus.over && Mouse.current != null && Mouse.current.leftButton.wasPressedThisFrame)
        {
            _curStatus = GameStatus.idle;

            serialAmbientText.SetActive(false);
            scoreText.SetActive(false);

            gameOverText.SetActive(false);

            uiManager.playBGM();
            mainPanel.SetActive(true);
        }
    }

    public void gameStart()
    {
        StartCoroutine(gameStartSequence());
    }

    public int getScore()
    {
        return _score;
    }

    public int getShots()
    {
        return _shots;
    }

    public float getDuckSpeed()
    {
        return _duckSpeed;
    }

    private void quitGame()
    {
#if UNITY_EDITOR
        // 에디터에서 게임을 실행하는 경우
        UnityEditor.EditorApplication.isPlaying = false;
#else
        Application.Quit();
#endif
    }

    private IEnumerator gameStartSequence()
    {
        _score = 0;
        _shots = 3;
        _duckSpeed = initialDuckSpeed;
        _strict = 0;

        // if (SerialManagerWin64.Instance.isSerialOpened())
        // {
        //     serialAmbientText.SetActive(true);
        //     scoreText.SetActive(false);
        // }
        // else
        // {
            serialAmbientText.SetActive(false);
            scoreText.SetActive(true);
            scoreText.GetComponent<TMP_Text>().text = _score.ToString();
        // }

        _curStatus = GameStatus.wait;
        yield return StartCoroutine(dog.findDuck());

        _curStatus = GameStatus.playing;
        StartCoroutine(duck.fly());
    }

    private IEnumerator clickHitTest(Vector2 clickPos)
    {
        _as.Play();

        _shots--;

        _curStatus = GameStatus.wait;

        turnOnBlackscreen();

        yield return new WaitForSeconds(0.12f);

        bool isHit = false;

        Ray ray = _cam.ScreenPointToRay(clickPos);
        RaycastHit2D hit = Physics2D.GetRayIntersection(ray, Mathf.Infinity, 1 << 0);

        if (hit.collider != null && hit.collider.gameObject == hitBox)
        {
            isHit = true;
        }

        turnOffBlackscreen();

        if (isHit)
        {
            int new_score = scoreUp();

            ScoreTextController scoreText = Instantiate(scoreTextPrefab, inGameUICanvas.transform);
            scoreText.GetComponent<RectTransform>().position = clickPos;
            scoreText.init("+" + new_score);

            if (_shots == 2)
            {
                _strict++;
            }

            yield return StartCoroutine(duck.shot());
            
            yield return StartCoroutine(duck.fall());

            yield return StartCoroutine(dog.getDuck(duck.getCurPos().x));
        }
        else if(_shots == 0)
        {
            _curStatus = GameStatus.wait;

            gameOverText.SetActive(true);

            yield return StartCoroutine(duck.flyAway());

            yield return StartCoroutine(dog.laughing());
            
            _curStatus = GameStatus.over;

            yield break;
        }
        else
        {
            _strict = 0;
        }

        _curStatus = GameStatus.playing;

        if (isHit)
        {
            duckSpeedUp();
            _shots = 3;

            StartCoroutine(duck.fly());
        }
    }

    private void turnOnBlackscreen()
    {
        blackScreen.SetActive(true);

        duck.hitTest();
    }

    private void turnOffBlackscreen()
    {
        duck.exitHitTest();

        blackScreen.SetActive(false);
    }

    private void duckSpeedUp()
    {
        _duckSpeed += 2.0f;
    }

    private int scoreUp()
    {
        int new_score = (100 + (int) _duckSpeed * 10) * (_shots + 1) + _strict * 100;

        _score += new_score;

        scoreText.GetComponent<TMP_Text>().text = _score.ToString();

        return new_score;
    }

    private IEnumerator serialHitTest(int initVal)
    {
        _shots--;

        _curStatus = GameStatus.wait;

        turnOnBlackscreen();

        yield return new WaitUntil(() => SerialIPC.Instance.isTestValInit());

        int testVal = SerialIPC.Instance.getTestVal();

        serialAmbientText.GetComponent<TMP_Text>().text = (initVal - testVal).ToString();

        bool isHit = initVal - testVal >= GameConfig.Instance.threshold;

        turnOffBlackscreen();

        if (isHit)
        {
            int new_score = scoreUp();

            Vector3 duck_cur_pos = Camera.main.WorldToScreenPoint(duck.getCurPos());

            ScoreTextController scoreText = Instantiate(scoreTextPrefab, inGameUICanvas.transform);
            scoreText.GetComponent<RectTransform>().position = duck_cur_pos + new Vector3(20f, 20f, 0f);
            scoreText.init("+" + new_score);

            if (_shots == 2)
            {
                _strict++;
            }

            yield return StartCoroutine(duck.shot());
            
            yield return StartCoroutine(duck.fall());

            yield return StartCoroutine(dog.getDuck(duck_cur_pos.x));
        }
        else if(_shots == 0)
        {
            _curStatus = GameStatus.wait;

            gameOverText.SetActive(true);

            yield return StartCoroutine(duck.flyAway());

            yield return StartCoroutine(dog.laughing());
            
            _curStatus = GameStatus.over;

            yield break;
        }
        else
        {
            _strict = 0;
        }

        _curStatus = GameStatus.playing;

        if (isHit)
        {
            duckSpeedUp();
            _shots = 3;

            StartCoroutine(duck.fly());
        }
    }
}
