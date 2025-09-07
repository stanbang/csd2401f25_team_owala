using UnityEngine;

public class Tutorial : MonoBehaviour
{
    public SpriteRenderer SR;
    public float detectionRange = 10f;
    public LayerMask PlayerLayer;
    public float Op;
    public int speed = 1;

    public float _Op
    {
        get { return Op; }
        set
        {
            if (value > 1f)  // Check max value
                Op = 1f;
            else if (value < 0f)  // Check min value
                Op = 0f;
            else
                Op = value;
        }
    }

    // Start is called once before the first execution of Update after the MonoBehaviour is created
    void Start()
    {
        SR = GetComponent<SpriteRenderer>();
        Op = _Op;
        
    }

    // Update is called once per frame
    void Update()
    {
        SR.color = new Color(1f, 1f, 1f, Op);
        PlayerNearby();
    }

    public void PlayerNearby()
    {
        Collider2D C = Physics2D.OverlapCircle(transform.position, detectionRange, PlayerLayer);

        if (C != null)
        {
            print(C.transform.name);
            if (Op < 1f)
            {
                _Op += Time.deltaTime * speed;
            }
        }
        else
        {
            if (Op > 0f)
            {
                _Op -= Time.deltaTime * speed;
            }
        }
    }

    void OnDrawGizmosSelected()
    {
        Gizmos.color = Color.green;
        Gizmos.DrawWireSphere(transform.position, detectionRange);
    }
}
