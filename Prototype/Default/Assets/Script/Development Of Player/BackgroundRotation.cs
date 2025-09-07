using UnityEngine;
using System.Collections;

public class BackgroundRotation : MonoBehaviour
{
    public float rotationDuration = 0.5f; // How long the rotation takes
    private bool isRotating = false;
    private float currentZ = 0f; // tracks total rotation, can exceed 360
    private float targetZ;

    void Start()
    {
        currentZ = transform.localEulerAngles.z; // start from current rotation
        targetZ = transform.eulerAngles.z;
    }

    void Update()
    {
        if (!isRotating)
        {
            if (Input.GetKeyDown(KeyCode.Q)) StartCoroutine(RotateStep(+1));
            if (Input.GetKeyDown(KeyCode.E)) StartCoroutine(RotateStep(-1));
        }
    }

    IEnumerator RotateStep(int dir)
    {
        isRotating = true;

        Quaternion start = transform.rotation;

        // Build one 90° step (dir = +1 or -1) around the WORLD Z axis
        Quaternion delta = Quaternion.AngleAxis(90f * dir, Vector3.forward);
        Quaternion target = start * delta;

        float elapsed = 0f;
        while (elapsed < rotationDuration)
        {
            elapsed += Time.deltaTime;
            float t = Mathf.Clamp01(elapsed / rotationDuration);

            // Smooth ease in/out
            float smoothT = Mathf.SmoothStep(0f, 1f, t);
            transform.rotation = Quaternion.Slerp(start, target, smoothT);

            yield return null;
        }

        transform.rotation = target; // snap exactly to avoid drift
        isRotating = false;
    }
}
