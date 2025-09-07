using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.SceneManagement;

public class ButtonChangeScenes : MonoBehaviour
{
    public Button btnPlay;
    public string SceneName = "You Win";

    // Start is called before the first frame update
    void Start()
    {
        btnPlay.onClick.AddListener(OnPlayClick);
    }

    private void OnPlayClick()
    {
        // UIManager.Instance.Close(this.gameObject);
        // AudioManager.Instance.SFXSound(SoundID.Confirm);
        SceneManager.LoadScene(sceneName: SceneName);
    }
}
