using UnityEngine;

public class FollowMouse : MonoBehaviour
{
    public Vector3 screenPosition;
    public float crossHairSpeed = 1f;

    // Start is called once before the first execution of Update after the MonoBehaviour is created
    void Start()
    {
        
    }

    void Update()
    {
        screenPosition = Input.mousePosition;
        screenPosition.z = crossHairSpeed;
        transform.position = Camera.main.ScreenToWorldPoint(screenPosition);
    }
}
