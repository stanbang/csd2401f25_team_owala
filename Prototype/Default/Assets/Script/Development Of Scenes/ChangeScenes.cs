using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.SceneManagement;

public class ChangeScenes : MonoBehaviour
{
    public string SceneName = "You Win";
    [SerializeField] private LayerMask TouchToWin;

    // Start is called once before the first execution of Update after the MonoBehaviour is created
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
        
    }

    private void OnTriggerEnter2D(Collider2D col)
    {
        // Check if the colliding object's layer is in the mask
        if (((1 << col.gameObject.layer) & TouchToWin) != 0)
        {
            // Load scene with the same name as this object
            SceneManager.LoadScene(sceneName: SceneName);
        }
    }
}
