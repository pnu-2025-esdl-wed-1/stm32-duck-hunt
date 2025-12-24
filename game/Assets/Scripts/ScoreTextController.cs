using UnityEngine;
using TMPro;
using System.Collections;

public class ScoreTextController: MonoBehaviour
{
    public float lifeTime = 1.0f;

    private TMP_Text _text;
    private RectTransform _rt;

    void Awake()
    {
        _text = GetComponent<TMP_Text>();
        _rt = GetComponent<RectTransform>();
    }

    public void init(string msg)
    {
        _text.text = msg;
        StartCoroutine(destroyText());
    }

    private IEnumerator destroyText()
    {
        yield return new WaitForSeconds(lifeTime);
        
        Destroy(gameObject);
    }
}
