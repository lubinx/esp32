#include <rtos/user.h>
#include <i2c.h>

#include <stropts.h>
#include <stdint.h>
#include <string.h>

#include "led.h"

/****************************************************************************
 *  @consts
 ****************************************************************************/
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

// consts
static uint8_t const __startup[] = {SYSDIS, COM16PMOS, RCMODE1, SYSEN, PWM(5), LEDON};
// static uint8_t const __clear[49] = {0};

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

struct LED_context
{
    int i2c_nb;
    int fd;

    uint8_t da;
    uint8_t pwm;        // 0~15

    uint16_t year;
    uint8_t month;
    uint8_t mday;
    uint16_t mtime;

    uint32_t flags;
};
static struct LED_context LED_context = {0};

static void LED_write(void const *buf, size_t count);
static void LED_update_date(int year, int month, int mday);
static void LED_update_time(int mtime);

static void LED_update_digit(uint8_t seg, int no, int count);
static void LED_update_wday(int wday);
static void LED_update_flags(void);

/****************************************************************************
 *  @export
 ****************************************************************************/
void LED_init(int i2c_nb, uint8_t da)
{
    LED_context.fd = I2C_createfd(i2c_nb, da, 100, 0, 0);
    // ioctl(LED_context.fd, OPT_WR_TIMEO, 500);

    LED_context.mtime = (uint16_t)-1;
    LED_context.pwm = 10;

    LED_context.flags = FLAG_IND_ALARM | FLAG_IND_ALARM_1 |
        FLAG_IND_HUMIDITY | FLAG_IND_PERCENT |
        FLAG_IND_TMPR | FLAG_IND_TMPR_C | FLAG_IND_TMPR_DOT;

    if (-1 != LED_context.fd)
    {
        LED_context.i2c_nb = i2c_nb;
        LED_context.da = da;

        LED_write(__startup, sizeof(__startup));
        LED_update_flags();
    }
}

void LED_test(void)
{
    LED_context.flags = 0xFFFFFFFF;
    LED_update_flags();

    for (int i = 10000; i <= 19999; i += 1111)
    {
        // yyyy/mm/dd
        LED_update_digit(SEG(0), i, 4);
        LED_update_digit(SEG(5), i, 2);
        LED_update_digit(SEG(7), i, 2);
        LED_update_wday((uint8_t)(i % 10));
        // hh:nn
        LED_update_digit(SEG(10), i, 4);
        // humidity / tmpr
        LED_update_digit(SEG(14), i, 2);
        LED_update_digit(SEG(16), i, 3);

        msleep(100);
    }
}

void LED_update_clock(void)
{
    struct tm tv;
    {
        time_t ts = time(NULL);
        localtime_r(&ts, &tv);
    }

    LED_update_date(tv.tm_year + 1900, tv.tm_mon + 1, tv.tm_mday);
    LED_update_wday((uint8_t)tv.tm_wday);
    LED_update_time(tv.tm_hour * 100 + tv.tm_min);

    if (tv.tm_hour >= 12)
        LED_context.flags = (LED_context.flags & (uint32_t)~(FLAG_IND_AM)) | FLAG_IND_PM;
    else
        LED_context.flags = (LED_context.flags & (uint32_t)~(FLAG_IND_PM)) | FLAG_IND_AM;

    LED_update_flags();
}

void LED_update_tmpr(int tmpr)
{
    LED_update_digit(SEG(16), tmpr, 3);
}

void LED_update_humidity(int humidity)
{
    LED_update_digit(SEG(14), humidity, 2);
}

/****************************************************************************
 *  @internal
 ****************************************************************************/
static void LED_write(void const *buf, size_t count)
{
    while (true)
    {
        if (-1 == write(LED_context.fd, buf, count))
            msleep(10);
        else
            break;
    }
}

static void LED_update_date(int year, int month, int mday)
{
    LED_context.flags = LED_context.flags | FLAG_IND_YEAR | FLAG_IND_MONTH | FLAG_IND_MDAY;

    if (LED_context.year != year)
    {
        LED_context.year = (uint16_t)year;
        LED_update_digit(SEG(0), LED_context.year, 4);
    }
    if (LED_context.month != month)
    {
        LED_context.month = (uint8_t)month;
        LED_update_digit(SEG(5), LED_context.month, 2);
    }
    if (LED_context.mday != mday)
    {
        LED_context.mday = (uint8_t)mday;
        LED_update_digit(SEG(7), LED_context.mday, 2);
    }
}

static void LED_update_time(int mtime)
{
    if (LED_context.mtime != mtime)
    {
        LED_context.mtime = (uint16_t)mtime;
        LED_update_digit(SEG(10), LED_context.mtime, 4);

        if (1200 > mtime)
            LED_context.flags = LED_context.flags | FLAG_IND_SEC | FLAG_IND_AM;
        else
            LED_context.flags = LED_context.flags | FLAG_IND_SEC | FLAG_IND_PM;
    }
}

static void LED_update_wday(int wday)
{
    uint8_t buf[1 + sizeof(uint16_t)] = {0};
    buf[0] = SEG(9);

    if (7 > (uint32_t)wday)
    {
        uint16_t xlat = __xlat_wday[wday];
        memcpy(&buf[1], &xlat, sizeof(xlat));
    }
    LED_write(&buf, sizeof(buf));
}

static void LED_update_flags(void)
{
    static uint32_t flags;

    if (flags != LED_context.flags)
    {
        flags = LED_context.flags;

        uint8_t buf[1 + sizeof(uint32_t)] = {0};
        buf[0] = SEG(19);
        memcpy(&buf[1], &flags, sizeof(flags));

        LED_write(&buf, sizeof(buf));
    }
}

static void LED_update_digit(uint8_t seg, int no, int count)
{
    uint8_t buf[1 + 4 * sizeof(uint16_t)] = {0};
    buf[0] = seg;
    count *= 2;

    if (0 <= no)
    {
        usleep(10);

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
    LED_write(&buf, (unsigned)count + 1);
}
