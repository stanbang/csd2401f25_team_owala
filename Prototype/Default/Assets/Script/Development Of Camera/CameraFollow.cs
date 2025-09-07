using System.Collections;
using System.Collections.Generic;
using UnityEngine;


public class CameraFollow : MonoBehaviour
{
    public Transform target;
    public float lerpSpeed;

    private Vector3 offset;

    private Vector3 targetPos;

    private bool bound = false;

    private void Start()
    {
        //findPlayerTarget(GameManager.Instance.CurrentPlayer.transform);
        //offset = transform.position - target.position;

        TryBindPlayer();
        if (!bound) StartCoroutine(WaitAndBindPlayer());
    }


    public void findPlayerTarget(Transform playerLocation)
    {
        target = playerLocation;
    }


    private void Update()
    {
        //if (target == null) return;

        //// Calculate desired XY position
        //targetPos = target.position + offset;

        //// Keep camera’s current Z
        //targetPos.z = transform.position.z;

        //// Smoothly move X and Y toward target, but keep Z current
        //float newX = Mathf.Lerp(transform.position.x, target.position.x + offset.x, lerpSpeed * Time.deltaTime);
        //float newY = Mathf.Lerp(transform.position.y, target.position.y + offset.y, lerpSpeed * Time.deltaTime);

        //transform.position = new Vector3(newX, newY, transform.position.z);

        if (!bound || target == null) return;

        // Desired target position (use offset if you want a shift)
        Vector3 desiredPos = target.position + offset;

        // Keep camera’s current Z value
        desiredPos.z = transform.position.z;

        // Smoothly move toward the target on X and Y
        transform.position = Vector3.Lerp(transform.position,desiredPos,lerpSpeed * Time.deltaTime);
    }

    private void TryBindPlayer()
    {
        var gm = GameManager.Instance;
        if (gm != null && gm.CurrentPlayer != null)
        {
            target = gm.CurrentPlayer.transform;
            transform.position = new Vector3(target.position.x, target.position.y, -10);
            offset = transform.position - target.position;
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
}
