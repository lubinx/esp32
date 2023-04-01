#include <unistd.h>
#include <pthread.h>
#include <sys/errno.h>

#include <i2c.h>
#include <stropts.h>
#include <stdint.h>
#include <string.h>

#include "panel.h"

/****************************************************************************
 *  @def
 ****************************************************************************/
    #define I2C_NB                      (0)
    #define DA                          (0x73)
    #define DEF_PWM                     (10)

    #define SYSDIS                      (0x80)
    #define SYSEN                       (0x81)
    #define LEDOFF                      (0x82)
    #define LEDON                       (0x83)
    #define BLINKOFF                    (0x88)
    #define BLINK2HZ                    (0x89)
    #define BLINK1HZ                    (0x8A)
    #define BLINK0_5HZ                  (0x8B)

    #define SLAVEMODE                   (0x90)
    #define RCMODE0                     (0x98)
    #define RCMODE1                     (0x9A)
    #define EXTCLK0                     (0x9C)
    #define EXTCLK1                     (0x9E)

    #define COM8NMOS                    (0xA0)
    #define COM16NMOS                   (0xA4)
    #define COM8PMOS                    (0xA8)
    #define COM16PMOS                   (0xAC)

    #define PWM(N)                      (0xB0 | (0 > N ? 0 : (15 < N ? 15 : N)))
    #define SEG(N)                      (0 < N ? (N - 1) * 4 : N)

    #define COM_A                       (1UL << 4)
    #define COM_B                       (1UL << 5)
    #define COM_C                       (1UL << 6)
    #define COM_D                       (1UL << 7)
    #define COM_E                       (1UL << 0)
    #define COM_F                       (1UL << 1)
    #define COM_G                       (1UL << 2)

    #define COM_AA                      (1UL << 3 | COM_A)
    #define COM_BB                      (1UL << 12 | COM_B)
    #define COM_CC                      (1UL << 13 | COM_C)
    #define COM_DD                      (1UL << 14 | COM_D)
    #define COM_EE                      (1UL << 15 | COM_E)
    #define COM_FF                      (1UL << 8  | COM_F)
    #define COM_GG                      (1UL << 9  | COM_G)

    #define FLAG_IND_YEAR               (1UL << 20)
    #define FLAG_IND_MONTH              (1UL << 21)
    #define FLAG_IND_MDAY               (1UL << 22)

    #define FLAG_IND_AM                 (1UL << 4 | 1UL << 5)
    #define FLAG_IND_PM                 (1UL << 6 | 1UL << 7)
    #define FLAG_IND_SEC                (1UL << 16 | 1UL << 23)

    #define FLAG_IND_ALARM              (1UL << 0 | 1UL << 1)
    #define FLAG_IND_ALARM_1            (1UL << 2)
    #define FLAG_IND_ALARM_2            (1UL << 3)
    #define FLAG_IND_ALARM_3            (1UL << 11)
    #define FLAG_IND_ALARM_4            (1UL << 12)

    #define FLAG_IND_HUMIDITY           (1UL << 14 | 1UL << 15)
    #define FLAG_IND_PERCENT            (1UL << 17)

    #define FLAG_IND_TMPR               (1 << 18 | 1 << 19)
    #define FLAG_IND_TMPR_DOT           (1UL << 28)
    #define FLAG_IND_TMPR_C             (1UL << 30)
    #define FLAG_IND_TMPR_F             (1UL << 31)

/****************************************************************************
 *  @def
 ****************************************************************************/
struct PANEL_context
{
    pthread_mutex_t lock;

    int i2c_fd;
    uint8_t pwm;            // 0~15

    uint8_t mday;
    uint8_t month;
    uint16_t year;
    uint16_t mtime;
    uint32_t flags;
};

/****************************************************************************
 *  @internal
 ****************************************************************************/
static void *PANEL_clock_thread(pthread_mutex_t *update_lock) __attribute__((noreturn));

static void PANEL_write(void const *buf, size_t count);
static void PANEL_update_date(int year, int month, int mday);
static void PANEL_update_time(int mtime);

static void PANEL_update_digit(uint8_t seg, int no, int count);
static void PANEL_update_wday(int wday);
static void PANEL_update_flags(void);

// var
static struct PANEL_context PANEL_context = {0};
static uint32_t PANEL_clock_thread_stack[2048 / sizeof(uint32_t)];

/****************************************************************************
 *  @implements
 ****************************************************************************/
void PANEL_init()
{
    PANEL_context.i2c_fd = I2C_createfd(I2C_NB, DA, 200, 0, 0);
    if (-1 == PANEL_context.i2c_fd)
    {
    }
    ioctl(PANEL_context.i2c_fd, OPT_WR_TIMEO, 500);

    PANEL_context.mtime = (uint16_t)-1;
    PANEL_context.pwm = DEF_PWM;

    PANEL_context.flags = FLAG_IND_ALARM | FLAG_IND_ALARM_1 |
        FLAG_IND_HUMIDITY | FLAG_IND_PERCENT |
        FLAG_IND_TMPR | FLAG_IND_TMPR_C | FLAG_IND_TMPR_DOT;

    static uint8_t const __startup[] = {SYSDIS, COM16PMOS, RCMODE1, SYSEN, PWM(DEF_PWM), LEDON};
    PANEL_write(__startup, sizeof(__startup));
    PANEL_update_flags();

    if (true)
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&PANEL_context.lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }
    if (true)
    {
        pthread_t id;
        pthread_attr_t attr;

        pthread_attr_init(&attr);
        pthread_attr_setstack(&attr, PANEL_clock_thread_stack, sizeof(PANEL_clock_thread_stack));
        pthread_create(&id, &attr, (void *)PANEL_clock_thread, &PANEL_context.lock);
        pthread_attr_destroy(&attr);
    }
}

int PANEL_pwm(uint8_t val)
{
    if (15 < val)
        return EINVAL;

    if (val != PANEL_context.pwm)
    {
        uint8_t pwm =  PWM(val);
        PANEL_write(&pwm, sizeof(pwm));
    }
    return 0;
}

void PANEL_test(void)
{
    PANEL_context.flags = 0xFFFFFFFF;
    PANEL_update_flags();

    for (int i = 10000; i <= 19999; i += 1111)
    {
        // yyyy/mm/dd
        PANEL_update_digit(SEG(0), i, 4);
        PANEL_update_digit(SEG(5), i, 2);
        PANEL_update_digit(SEG(7), i, 2);
        PANEL_update_wday((uint8_t)(i % 10));
        // hh:nn
        PANEL_update_digit(SEG(10), i, 4);
        // humidity / tmpr
        PANEL_update_digit(SEG(14), i, 2);
        PANEL_update_digit(SEG(16), i, 3);

        msleep(300);
    }
}

void PANEL_update_tmpr(int tmpr)
{
    PANEL_update_digit(SEG(16), tmpr, 3);
}

void PANEL_update_humidity(int humidity)
{
    PANEL_update_digit(SEG(14), humidity, 2);
}

/****************************************************************************
 *  @internal
 ****************************************************************************/
static void *PANEL_clock_thread(pthread_mutex_t *lock)
{
    struct tm tv = {0};

    while (1)
    {
        time_t ts = time(NULL);
        localtime_r(&ts, &tv);

        printf("%04d/%02d/%02d %02d:%02d:%02d\n",
            1900 + tv.tm_year, 1 + tv.tm_mon, tv.tm_mday,
            tv.tm_hour, tv.tm_min, tv.tm_sec
        );
        fflush(stdout);

        pthread_mutex_lock(lock);
        {
            PANEL_update_date(tv.tm_year + 1900, tv.tm_mon + 1, tv.tm_mday);
            PANEL_update_wday((uint8_t)tv.tm_wday);
            PANEL_update_time(tv.tm_hour * 100 + tv.tm_min);

            if (tv.tm_hour >= 12)
                PANEL_context.flags = (PANEL_context.flags & (uint32_t)~(FLAG_IND_AM)) | FLAG_IND_PM;
            else
                PANEL_context.flags = (PANEL_context.flags & (uint32_t)~(FLAG_IND_PM)) | FLAG_IND_AM;

            PANEL_update_flags();
        }
        pthread_mutex_unlock(lock);

        msleep(1000);
    }
}

static void PANEL_write(void const *buf, size_t count)
{
    while (true)
    {
        if (-1 == write(PANEL_context.i2c_fd, buf, count))
        {
            // write error?
        }
        break;
    }
}

static void PANEL_update_date(int year, int month, int mday)
{
    PANEL_context.flags = PANEL_context.flags | FLAG_IND_YEAR | FLAG_IND_MONTH | FLAG_IND_MDAY;

    if (PANEL_context.year != year)
    {
        PANEL_context.year = (uint16_t)year;
        PANEL_update_digit(SEG(0), PANEL_context.year, 4);
    }
    if (PANEL_context.month != month)
    {
        PANEL_context.month = (uint8_t)month;
        PANEL_update_digit(SEG(5), PANEL_context.month, 2);
    }
    if (PANEL_context.mday != mday)
    {
        PANEL_context.mday = (uint8_t)mday;
        PANEL_update_digit(SEG(7), PANEL_context.mday, 2);
    }
}

static void PANEL_update_time(int mtime)
{
    if (PANEL_context.mtime != mtime)
    {
        PANEL_context.mtime = (uint16_t)mtime;
        PANEL_update_digit(SEG(10), PANEL_context.mtime, 4);

        if (1200 > mtime)
            PANEL_context.flags = PANEL_context.flags | FLAG_IND_SEC | FLAG_IND_AM;
        else
            PANEL_context.flags = PANEL_context.flags | FLAG_IND_SEC | FLAG_IND_PM;
    }
}

static void PANEL_update_wday(int wday)
{
    static uint16_t const __xlat_wday[] =
    {
        [0] = 1 << 8 | 1 << 9,
        [1] = 1 << 4 | 1 << 5,
        [2] = 1 << 6 | 1 << 7,
        [3] = 1 << 0 | 1 << 1,
        [4] = 1 << 2 | 1 << 3,
        [5] = 1 << 12 | 1 << 13,
        [6] = 1 << 14 | 1 << 15,
    };

    uint8_t buf[1 + sizeof(uint16_t)] = {0};
    buf[0] = SEG(9);

    if (7 > (uint32_t)wday)
    {
        uint16_t xlat = __xlat_wday[wday];
        memcpy(&buf[1], &xlat, sizeof(xlat));
    }
    PANEL_write(&buf, sizeof(buf));
}

static void PANEL_update_flags(void)
{
    static uint32_t flags;

    if (flags != PANEL_context.flags)
    {
        flags = PANEL_context.flags;

        uint8_t buf[1 + sizeof(uint32_t)] = {0};
        buf[0] = SEG(19);
        memcpy(&buf[1], &flags, sizeof(flags));

        PANEL_write(&buf, sizeof(buf));
    }
}

static void PANEL_update_digit(uint8_t seg, int no, int count)
{
    static uint8_t const __xlat_digit[] =
    {
        [0] = COM_A | COM_B | COM_C | COM_D | COM_E | COM_F,
        [1] = COM_B | COM_C,
        [2] = COM_A | COM_B | COM_G | COM_E | COM_D,
        [3] = COM_A | COM_B | COM_C | COM_D | COM_G,
        [4] = COM_F | COM_G | COM_B | COM_C,
        [5] = COM_A | COM_F | COM_G | COM_C | COM_D,
        [6] = COM_A | COM_F | COM_G | COM_E | COM_C | COM_D,
        [7] = COM_A | COM_B | COM_C,
        [8] = COM_A | COM_B | COM_C | COM_D | COM_E | COM_F | COM_G,
        [9] = COM_A | COM_B | COM_C | COM_D | COM_F | COM_G,
    };

    static uint16_t const __xlat_digit_b[] =
    {
        [0] = COM_AA | COM_BB | COM_CC | COM_DD | COM_EE | COM_FF,
        [1] = COM_BB | COM_CC,
        [2] = COM_AA | COM_BB | COM_GG | COM_EE | COM_DD,
        [3] = COM_AA | COM_BB | COM_CC | COM_DD | COM_GG,
        [4] = COM_FF | COM_GG | COM_BB | COM_CC,
        [5] = COM_AA | COM_FF | COM_GG | COM_CC | COM_DD,
        [6] = COM_AA | COM_FF | COM_GG | COM_EE | COM_CC | COM_DD,
        [7] = COM_AA | COM_BB | COM_CC,
        [8] = COM_AA | COM_BB | COM_CC | COM_DD | COM_EE | COM_FF | COM_GG,
        [9] = COM_AA | COM_BB | COM_CC | COM_DD | COM_FF | COM_GG,
    };

    uint8_t buf[1 + 4 * sizeof(uint16_t)] = {0};
    buf[0] = seg;
    count *= 2;

    if (0 <= no)
    {
        if (SEG(10) <= seg && SEG(13) >= seg)
        {
            int digit = 1;
            for (int i = count - 1; i >= 0; i -= 2)
            {
                uint16_t xlat = __xlat_digit_b[no % 10];
                memcpy(&buf[i], &xlat, sizeof(xlat));

                no /= 10;
                if (3 < ++ digit && 0 == no) break;
            }
        }
        else
        {
            for (int i = count - 1; i >= 0; i -= 2)
            {
                buf[i] = __xlat_digit[no % 10];

                no /= 10;
                if (0 == no) break;
            }
        }
    }

    PANEL_write(&buf, (unsigned)count + 1);
}
