using UnityEngine;
using UnityEngine.InputSystem;

public class Player : MonoBehaviour
{
    public enum PlayerAction
    {
        None,
        Attack,
        Block,
        Roll,
        Jump,
        Fall,
        Hurt,
        Death,
        Run,
        Idle
    }

    public PlayerAction action = PlayerAction.Idle;

    public static Player Instance;


    [SerializeField] float m_speed = 4.0f;
    [SerializeField] float m_jumpForce = 7.5f;
    [SerializeField] float m_rollForce = 6.0f;
    [SerializeField] bool m_noBlood = false;
    [SerializeField] GameObject m_slideDust;

    [SerializeField] private int maxHealthPoint = 100;
    [SerializeField] private int maxManaPoint = 100;

    public int MaxHealthPoint
    {
        get { return maxHealthPoint; }
        set
        {
            maxHealthPoint = Mathf.Max(1, value);
        }
    }
    public int MaxManaPoint
    {
        get { return maxManaPoint; }
        set
        {
            maxManaPoint = Mathf.Max(1, value);
        }
    }

    public int healthPoint;
    public int manaPoint;

    private SpriteRenderer sp;
    private Animator m_animator;
    private Rigidbody2D m_body2d;

    private Sensor_HeroKnight m_groundSensor;
    private Sensor_HeroKnight m_wallSensorR1;
    private Sensor_HeroKnight m_wallSensorR2;
    private Sensor_HeroKnight m_wallSensorL1;
    private Sensor_HeroKnight m_wallSensorL2;

    private bool m_isWallSliding = false;
    private bool m_grounded = false;
    private bool m_rolling = false;
    private int m_facingDirection = 1;
    private int m_currentAttack = 0;
    private float m_timeSinceAttack = 0.0f;
    private float m_delayToIdle = 0.0f;
    private float m_rollDuration = 8.0f / 14.0f;
    private float m_rollCurrentTime;

    private void Awake()
    {
        Instance = this; // Inserting this into the Static Pigeon hole.

        healthPoint = maxHealthPoint;
        manaPoint = maxManaPoint;
        m_body2d = GetComponent<Rigidbody2D>();
        sp = GetComponent<SpriteRenderer>();
        m_animator = GetComponent<Animator>();
    }

    // Start is called once before the first execution of Update after the MonoBehaviour is created
    void Start()
    {
        m_groundSensor = transform.Find("GroundSensor").GetComponent<Sensor_HeroKnight>();
        m_wallSensorR1 = transform.Find("WallSensor_R1").GetComponent<Sensor_HeroKnight>();
        m_wallSensorR2 = transform.Find("WallSensor_R2").GetComponent<Sensor_HeroKnight>();
        m_wallSensorL1 = transform.Find("WallSensor_L1").GetComponent<Sensor_HeroKnight>();
        m_wallSensorL2 = transform.Find("WallSensor_L2").GetComponent<Sensor_HeroKnight>();
    }

    // Update is called once per frame
    void Update()
    {
        GroundCheck();

        // -- Handle input and movement --
        float inputX = Input.GetAxis("Horizontal");

        // Swap direction of sprite depending on walk direction
        if (inputX > 0)
        {
            GetComponent<SpriteRenderer>().flipX = false;
            m_facingDirection = 1;
        }
        else if (inputX < 0)
        {
            GetComponent<SpriteRenderer>().flipX = true;
            m_facingDirection = -1;
        }

        // Example input checks → assign action
        //if (Input.GetKeyDown("e") && !m_rolling)
        //    action = PlayerAction.Death;
        //else if (Input.GetKeyDown("q") && !m_rolling)
        //    action = PlayerAction.Hurt;
        if (Input.GetMouseButtonDown(0) && m_timeSinceAttack > 0.25f && !m_rolling)
            action = PlayerAction.Attack;
        else if (Input.GetMouseButtonDown(1) && !m_rolling)
            action = PlayerAction.Block;
        else if (Input.GetMouseButtonUp(1))
            m_animator.SetBool("IdleBlock", false);
        else if (Input.GetKeyDown("left shift") && !m_rolling && !m_isWallSliding)
            action = PlayerAction.Roll;
        else if (Input.GetKeyDown("space") && m_grounded && !m_rolling)
            action = PlayerAction.Jump;
        else if (m_grounded && !m_groundSensor.State())
            action = PlayerAction.Fall;
        else if (Mathf.Abs(inputX) > Mathf.Epsilon)
            action = PlayerAction.Run;
        else
            action = PlayerAction.Idle;

        switch (action)
        {
            case PlayerAction.Death:
                m_animator.SetBool("noBlood", m_noBlood);
                m_animator.SetTrigger("Death");
                break;

            case PlayerAction.Hurt:
                m_animator.SetTrigger("Hurt");
                break;

            case PlayerAction.Attack:
                m_currentAttack++;

                if (m_currentAttack > 3) m_currentAttack = 1;
                if (m_timeSinceAttack > 1.0f) m_currentAttack = 1;

                m_animator.SetTrigger("Attack" + m_currentAttack);
                m_timeSinceAttack = 0.0f;
                break;

            case PlayerAction.Block:
                m_animator.SetTrigger("Block");
                m_animator.SetBool("IdleBlock", true);
                break;

            case PlayerAction.Roll:
                m_rolling = true;
                m_animator.SetTrigger("Roll");
                AE_SlideDust();
                m_body2d.linearVelocity = new Vector2(m_facingDirection * m_rollForce, m_body2d.linearVelocity.y);
                break;

            case PlayerAction.Jump:
                m_animator.SetTrigger("Jump");
                m_grounded = false;
                m_animator.SetBool("Grounded", m_grounded);
                m_body2d.linearVelocity = new Vector2(m_body2d.linearVelocity.x, m_jumpForce);
                m_groundSensor.Disable(0.2f);
                break;

            case PlayerAction.Fall:
                m_grounded = false;
                m_animator.SetBool("Grounded", m_grounded);
                break;

            case PlayerAction.Run:
                // Move
                if (!m_rolling)
                    m_body2d.linearVelocity = new Vector2(inputX * m_speed, m_body2d.linearVelocity.y);
                //Set AirSpeed in animator
                m_animator.SetFloat("AirSpeedY", m_body2d.linearVelocity.y);
                //Run
                if (Mathf.Abs(inputX) > Mathf.Epsilon)
                {
                    // Reset timer
                    m_delayToIdle = 0.05f;
                    m_animator.SetInteger("AnimState", 1);
                }
                break;

            case PlayerAction.Idle:
                m_delayToIdle -= Time.deltaTime;
                if (m_delayToIdle < 0)
                    m_animator.SetInteger("AnimState", 0);
                if (m_delayToIdle < -0.5f)
                    m_delayToIdle = -0.5f;
                break;

            case PlayerAction.None:
            default:
                // do nothing
                break;
        }
    }

    public void GroundCheck()
    {
        // Increase timer that controls attack combo
        m_timeSinceAttack += Time.deltaTime;

        // Increase timer that checks roll duration
        if (m_rolling)
            m_rollCurrentTime += Time.deltaTime;

        // Disable rolling if timer extends duration
        if (m_rollCurrentTime > m_rollDuration)
            m_rolling = false;

        //Check if character just landed on the ground
        if (!m_grounded && m_groundSensor.State())
        {
            m_grounded = true;
            m_animator.SetBool("Grounded", m_grounded);
        }

        //Check if character just started falling
        if (m_grounded && !m_groundSensor.State())
        {
            m_grounded = false;
            m_animator.SetBool("Grounded", m_grounded);
        }
    }

    // Animation Events
    // Called in slide animation.
    void AE_SlideDust()
    {
        Vector3 spawnPosition;

        if (m_facingDirection == 1)
            spawnPosition = m_groundSensor.transform.position;
        else
            spawnPosition = m_groundSensor.transform.position;

        if (m_slideDust != null)
        {
            // Set correct arrow spawn position
            GameObject dust = Instantiate(m_slideDust, spawnPosition, Quaternion.Euler(0, 0, 90)) as GameObject;
            // Turn arrow in correct direction
            dust.transform.localScale = new Vector3(-1, m_facingDirection, 1);
        }
    }
}
