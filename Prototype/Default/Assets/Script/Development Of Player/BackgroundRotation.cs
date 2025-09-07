using UnityEngine;
using System.Collections;

public class BackgroundRotation : MonoBehaviour
{
    [Tooltip("Degrees per second to rotate")]
    public float rotationSpeed; // adjust for how fast you want

    void Update()
    {
        // Rotate clockwise while holding RightArrow
        if (Input.GetKey(KeyCode.E))
        {
            transform.Rotate(0f, 0f, -rotationSpeed * Time.deltaTime);
        }

        // Rotate counter-clockwise while holding LeftArrow
        if (Input.GetKey(KeyCode.Q))
        {
            transform.Rotate(0f, 0f, rotationSpeed * Time.deltaTime);
        }
    }
}
