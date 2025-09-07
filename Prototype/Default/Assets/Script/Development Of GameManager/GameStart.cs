using UnityEngine;

public class GameStart : MonoBehaviour
{
    public Transform Location;
    private void Start()
    {
        if (Location != null)
        {
            GameManager.Instance.spawnPlayerOnce(Location.position);
        }
        else
        {
            Debug.LogWarning("GameStart: No Location assigned! Using default (0,0,0).");
            GameManager.Instance.spawnPlayerOnce(Vector3.zero);
        }
    }
}
