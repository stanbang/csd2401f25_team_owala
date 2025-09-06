using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using static UnityEngine.GraphicsBuffer;

public class CrossHair : MonoBehaviour
{
    private SpriteRenderer spCrossHair;
    
    public Vector3 screenPosition;
    public float crossHairSpeed = 1f;

    public Sprite crossHair01Blue;
    public Sprite crossHair02Red;

    

    // Start is called before the first frame update
    void Start()
    {
        spCrossHair = GetComponent<SpriteRenderer>();
        spCrossHair.sprite = crossHair01Blue;
    }


    // Update is called once per frame
    void Update()
    {
        screenPosition = Input.mousePosition;
        screenPosition.z = crossHairSpeed;
        transform.position = Camera.main.ScreenToWorldPoint(screenPosition);
    }
}
