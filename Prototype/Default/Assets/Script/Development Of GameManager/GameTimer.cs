using System.Collections;
using TMPro;
using UnityEngine;
using UnityEngine.SceneManagement;
using UnityEngine.UI;

public class GameTimer : SimpleSingleton<GameTimer>
{
    public TMP_Text timerText; // Assign this in the Inspector
    public string sceneAfterWinning = "putSceneName";
    public float timeRemaining = 60f; // 1 minute in seconds
    private bool timerIsRunning = true;

    public Player pm;
    private bool bound = false;

    protected virtual void Start()
    {
        pm = FindObjectOfType<Player>();

        TryBindPlayer();
        if (!bound) StartCoroutine(WaitAndBindPlayer());
    }

    void Update()
    {
        if (timerIsRunning)
        {
            if (pm.healthPoint <= 0)
            {
                timerIsRunning = false;
                DisplayTime(timeRemaining);
                // UIManager.Instance.OpenReplace(GameUIID.YouLose);
                Destroy(pm.gameObject);
                Time.timeScale = 0f;
                SceneManager.LoadScene(sceneName: "You Lose");
            }
            else
            {
                if (timeRemaining > 0 && pm != null)
                {
                    timeRemaining -= Time.deltaTime;
                    DisplayTime(timeRemaining);
                }
                else
                {
                    Debug.Log("Time has run out!");
                    timeRemaining = 0;
                    timerIsRunning = false;
                    // UIManager.Instance.OpenReplace(GameUIID.YouWin);
                    // DestroyAllEnemies();
                    Time.timeScale = 0f;
                    // DisplayTime(0);
                    SceneManager.LoadScene(sceneName: "You Lose");
                }
            }
        }
    }

    public string GetSceneName()
    {
        return sceneAfterWinning;
    }

    private void TryBindPlayer()
    {
        var gm = GameManager.Instance;
        if (gm != null && gm.CurrentPlayer != null)
        {
            pm = gm.CurrentPlayer;
            bound = true;
        }
    }

    private IEnumerator WaitAndBindPlayer()
    {
        // Poll until the player exists (e.g., spawned this frame or next)
        while (true)
        {
            TryBindPlayer();
            if (bound) yield break;
            yield return null; // wait a frame
        }
    }

    void DisplayTime(float timeToDisplay)
    {
        timeToDisplay = Mathf.Clamp(timeToDisplay, 0, Mathf.Infinity);

        int minutes = Mathf.FloorToInt(timeToDisplay / 60);
        int seconds = Mathf.FloorToInt(timeToDisplay % 60);

        timerText.text = string.Format("{0:00}:{1:00}", minutes, seconds);
    }

    //void DestroyAllEnemies()
    //{
    //    Enemy[] allEnemies = FindObjectsOfType<Enemy>();
    //    foreach (Enemy enemy in allEnemies)
    //    {
    //        Destroy(enemy.gameObject);
    //    }
    //}
}