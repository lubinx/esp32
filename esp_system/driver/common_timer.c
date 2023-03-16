/****************************************************************************
  This file is part of UltraCore

  Copyright by UltraCreation Co Ltd 2018
-------------------------------------------------------------------------------
    The contents of this file are used with permission, subject to the Mozilla
  Public License Version 1.1 (the "License"); you may not use this file except
  in compliance with the License. You may  obtain a copy of the License at
  http://www.mozilla.org/MPL/MPL-1.1.html

    Software distributed under the License is distributed on an "AS IS" basis,
  WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for
  the specific language governing rights and limitations under the License.
****************************************************************************/
#include <rtos/kernel.h>

#include "hw/timer.h"
#include "hw/hal/timer_hal.h"

/***************************************************************************
 * @const
 ***************************************************************************/
/// @designated last matching point reset the TIMER counter
#define TIMER_MR_RST                    (TIMER_HAL_feature.MATCH_count - 1)

/***************************************************************************
 *  @declaration
 ***************************************************************************/
struct TIMER_config
{
    struct TIMER_config *link_next;
    uint32_t pad[3];
};

/// @matching section
struct TIMER_match_section
{
    struct TIMER_match_section *link_next;
    uint32_t repeat;

    struct TIMER_match_point *match_entry;
    uint16_t match_count;

    uint16_t id;
};

/// @matching point
struct TIMER_match_point
{
    struct TIMER_match_point *link_next;
    uint32_t value;

    void *arg;
    uint16_t id;
};

/***************************************************************************
 *  @internal
 ***************************************************************************/
static void *TIMER_MATCH_config_get(void);
static bool CTX_match_load_next(struct TIMER_context *ctx);

static glist_t MATCH_config_pool;
static struct TIMER_config PREALLOC[8];

/***************************************************************************
 *  @constructor
 ***************************************************************************/
#ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmissing-prototypes"
#endif

__attribute__((constructor(102)))
static void TIMER_initialize(void)
{
    glist_initialize(&MATCH_config_pool);
    for (size_t i = 0; i < lengthof(PREALLOC); i ++)
        glist_push_back(&MATCH_config_pool, &PREALLOC[i]);

    TIMER_HAL_init();
}

#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif

/***************************************************************************
 *  TIMER @common
 ***************************************************************************/
int TIMER_configure(int nb, uint32_t const freq, enum TIMER_mode_t mode, TIMER_callback_t callback)
{
    if ((uint32_t)nb >= TIMER_HAL_feature.DEV_count)
        return ENODEV;

    struct TIMER_context *ctx = &TIMER_context[nb];
    int retval;
    KERNEL_lock();

    if (TIMER_UNCONFIGURED != ctx->mode)
    {
        retval = EBUSY;
        goto timer_configure_exit;
    }

    retval = TIMER_HAL_configure(ctx->dev, nb, freq, mode);
    if (0 == retval)
    {
        ctx->mode = mode;

        if (NULL != callback)
        {
            ctx->callback = callback;
            TIMER_HAL_intr_enable(ctx->dev, nb, mode);
        }

        if (TIMER_MATCH == mode)
            glist_initialize(&ctx->config.MATCH.list);
    }

timer_configure_exit:
    KERNEL_unlock();
    return retval;
}

bool TIMER_is_configured(int nb)
{
    if ((uint32_t)nb >= TIMER_HAL_feature.DEV_count)
        return false;
    else
        return TIMER_UNCONFIGURED != TIMER_context[nb].mode;
}

int TIMER_deconfigure(int nb)
{
    if ((uint32_t)nb >= TIMER_HAL_feature.DEV_count)
        return ENODEV;

    struct TIMER_context *ctx = &TIMER_context[nb];

    if (TIMER_UNCONFIGURED != ctx->mode)
    {
        TIMER_HAL_intr_disable(ctx->dev, nb);
        TIMER_HAL_stop(ctx->dev);

        switch(ctx->mode)
        {
        case TIMER_MATCH:
            TIMER_match_clear(nb);
            break;
        case TIMER_PWM:
            TIMER_PWM_HAL_release(nb, ctx->config.PWM.gpio, ctx->config.PWM.pin);
            break;
        default:
        // TODO: TIMER_deconfigure() case TIMER_CAPTURE:
            break;
        }

        TIMER_HAL_deconfigure(ctx->dev, nb);
        ctx->mode = TIMER_UNCONFIGURED;
    }
    return 0;
}

int TIMER_start(int nb)
{
    if ((uint32_t)nb >= TIMER_HAL_feature.DEV_count)
        return ENODEV;

    // PWM use TIMER_PWM_update() to start
    struct TIMER_context *ctx = &TIMER_context[nb];
    if (TIMER_UNCONFIGURED == ctx->mode || TIMER_PWM == ctx->mode)
        return EPERM;
    if (TIMER_HAL_is_running(ctx->dev))
        return EACCES;

    int retval = 0;
    KERNEL_lock();

    switch(ctx->mode)
    {
        struct TIMER_match_section *section;

    case TIMER_MATCH :
        section = ctx->config.MATCH.section_executing;
        if (NULL == section)
        {
            section = ctx->config.MATCH.section_executing = glist_pop(&ctx->config.MATCH.list);
            ctx->config.MATCH.list_count --;

            if (section && section->match_entry)
            {
                ctx->config.MATCH.curr = section->match_entry;
                CTX_match_load_next(ctx);
            }
            else
                retval = ENOENT;
        }
        break;

    case TIMER_FREE_RUNNING:
    case TIMER_PWM:
        // nothing todo
        break;

    default:
    /*
    TODO: TIMER_start() case TIMER_CAPTURE:
    */
        retval = ENOSYS;
        break;
    }

    if (0 == retval)
    {
        ctx->curr_repeating = 0;
        TIMER_HAL_start(ctx->dev);
    }
    KERNEL_unlock();
    return retval;
}

int TIMER_stop(int nb)
{
    if ((uint32_t)nb >= TIMER_HAL_feature.DEV_count)
        return ENODEV;

    // PWM mode can not stop
    struct TIMER_context *ctx = &TIMER_context[nb];
    if (TIMER_UNCONFIGURED == ctx->mode || TIMER_PWM == ctx->mode)
        return EPERM;

    TIMER_HAL_stop(TIMER_context[nb].dev);
    return 0;
}

bool TIMER_is_running(int nb)
{
    if ((uint32_t)nb >= TIMER_HAL_feature.DEV_count)
        return false;
    else
        return TIMER_HAL_is_running(TIMER_context[nb].dev);
}

uint32_t TIMER_tick(int nb)
{
    if ((uint32_t)nb >= TIMER_HAL_feature.DEV_count)
        return 0;
    else
        return TIMER_HAL_tick(TIMER_context[nb].dev);
}

int TIMER_spin(int nb, uint32_t ticks)
{
    if ((uint32_t)nb >= TIMER_HAL_feature.DEV_count)
        return ENODEV;

    void *const dev = TIMER_context[nb].dev;
    uint32_t tick_start = TIMER_HAL_tick(dev);

    while (TIMER_HAL_is_running(dev) && TIMER_HAL_tick(dev) - tick_start < ticks)
        {}
    return 0;
}

/***************************************************************************
 *  TIMER @free running
 ***************************************************************************/
int TIMER_free_running_configure(int nb, uint32_t const freq)
{
    return TIMER_configure(nb, freq, TIMER_FREE_RUNNING, NULL);
}

/***************************************************************************
 *  TIMER @match
 ***************************************************************************/
int TIMER_match_configure(int nb, uint32_t const freq, TIMER_callback_t callback)
{
    return TIMER_configure(nb, freq, TIMER_MATCH, callback);
}

int TIMER_match_clear(int nb)
{
    if ((uint32_t)nb >= TIMER_HAL_feature.DEV_count)
        return ENODEV;

    struct TIMER_context *ctx = &TIMER_context[nb];
    if (TIMER_MATCH != ctx->mode || TIMER_HAL_is_running(TIMER_context[nb].dev))
        return EPERM;

    KERNEL_lock();

    if (ctx->config.MATCH.section_executing)
        glist_push_back(&ctx->config.MATCH.list, ctx->config.MATCH.section_executing);

    while (! glist_is_empty(&ctx->config.MATCH.list))
    {
        struct TIMER_match_section *config = glist_pop(&ctx->config.MATCH.list);
        struct TIMER_match_point *match_entry = config->match_entry;

        while (match_entry)
        {
            struct TIMER_match_point *tmp = match_entry;
            match_entry = match_entry->link_next;

            glist_push_back(&MATCH_config_pool, tmp);
        }
        glist_push_back(&MATCH_config_pool, config);
    }

    ctx->config.MATCH.list_count = 0;
    ctx->config.MATCH.section_executing = NULL;
    ctx->config.MATCH.section_pushing = NULL;
    ctx->config.MATCH.curr = NULL;
    ctx->config.MATCH.tick_shift = 0;

    KERNEL_unlock();
    return 0;
}

int TIMER_match_section(int nb, uint16_t id, uint32_t repeat)
{
    if ((uint32_t)nb >= TIMER_HAL_feature.DEV_count)
        return ENODEV;

    struct TIMER_context *ctx = &TIMER_context[nb];
    if (TIMER_MATCH != ctx->mode)
        return EPERM;

    struct TIMER_match_section *section = TIMER_MATCH_config_get();
    if (section)
    {
        section->match_entry = NULL;
        section->match_count = 0;

        section->id = id;
        section->repeat = repeat;
    }
    else
        return ENOMEM;

    KERNEL_lock();
    ctx->config.MATCH.section_pushing = section;

    glist_push_back(&ctx->config.MATCH.list, section);
    ctx->config.MATCH.list_count ++;

    KERNEL_unlock();
    return 0;
}

int TIMER_match(int nb, uint16_t id, uint32_t val, void *arg)
{
    if ((uint32_t)nb >= TIMER_HAL_feature.DEV_count)
        return ENODEV;

    struct TIMER_context *ctx = &TIMER_context[nb];
    if (TIMER_MATCH != ctx->mode)
        return EPERM;

    struct TIMER_match_point *ptr = TIMER_MATCH_config_get();
    if (ptr)
    {
        ptr->id = id;
        ptr->value = val;
        ptr->arg = arg;
    }
    else
        return ENOMEM;

    // no section? assume its a INFINITE matching
    if (! ctx->config.MATCH.section_pushing)
        TIMER_match_section(nb, (uint16_t)-1, INFINITE);

    KERNEL_lock();
    struct TIMER_match_section *section = ctx->config.MATCH.section_pushing;
    struct TIMER_match_point **iter = &section->match_entry;

    while(NULL != *iter)
    {
        if ((*iter)->value > val)
            break;
        else
            iter = &(*iter)->link_next;
    }

    ptr->link_next = *iter;
    *iter = ptr;
    section->match_count ++;

    KERNEL_unlock();
    return 0;
}

uint32_t TIMER_match_cached_sections(int nb)
{
    if ((uint32_t)nb >= TIMER_HAL_feature.DEV_count)
        return 0;
    else
        return TIMER_context[nb].config.MATCH.list_count;
}

/***************************************************************************
 *  TIMER @PWM
 ***************************************************************************/
int TIMER_PWM_get(uint32_t const freq, void *const gpio, uint32_t pin, TIMER_callback_t callback)
{
    KERNEL_lock();
    int retval = TIMER_PWM_HAL_get_channel(gpio, pin);

    if (-1 != retval)
    {
        int err = TIMER_configure(retval, freq, TIMER_PWM, callback);
        if (0 == err)
        {
            struct TIMER_context *ctx = &TIMER_context[retval];
            ctx->config.PWM.gpio = gpio;
            ctx->config.PWM.pin = pin;

            TIMER_PWM_HAL_polarity(ctx->dev, gpio, pin);
        }
        else
        {
            TIMER_PWM_HAL_release(retval, gpio, pin);
            retval = __set_errno_neg(err);
        }
    }

    KERNEL_unlock();
    return retval;
}

int TIMER_PWM_update(int nb, uint32_t duty, uint32_t cycle)
{
    if ((uint32_t)nb >= TIMER_HAL_feature.DEV_count)
        return ENODEV;
    if (0 == duty || 0 == cycle || duty >= cycle)
        return EINVAL;

    struct TIMER_context *ctx = &TIMER_context[nb];
    if (TIMER_PWM != ctx->mode)
        return EPERM;

    TIMER_PWM_HAL_update(ctx->dev, duty, cycle);
    return 0;
}

int TIMER_PWM_update_duty(int nb, uint32_t duty)
{
    if ((uint32_t)nb >= TIMER_HAL_feature.DEV_count)
        return ENODEV;

    struct TIMER_context *ctx = &TIMER_context[nb];
    if (TIMER_PWM != ctx->mode)
        return EPERM;

    TIMER_PWM_HAL_update_duty(ctx->dev, duty);
    return 0;
}

/***************************************************************************
 *  @internal implementation
 ***************************************************************************/
static void *TIMER_MATCH_config_get(void)
{
    KERNEL_lock();
    if (glist_is_empty(&MATCH_config_pool))
    {
        struct TIMER_config *ptr = KERNEL_malloc(sizeof(PREALLOC));
        if (ptr)
        {
            for (size_t i = 0; i < lengthof(PREALLOC); i ++)
                glist_push_back(&MATCH_config_pool, &ptr[i]);
        }
    }
    struct TIMER_config *retval = glist_pop(&MATCH_config_pool);

    KERNEL_unlock();
    return retval;
}

static bool CTX_match_load_next(struct TIMER_context *ctx)
{
    struct TIMER_match_section *section_executing = ctx->config.MATCH.section_executing;
    if (! section_executing)
        return false;

    if (! ctx->config.MATCH.curr)
    {
        ctx->curr_repeating ++;

        if (ctx->curr_repeating >= section_executing->repeat && INFINITE != ctx->curr_repeating)
        {
            ctx->callback(section_executing->id, NULL, ctx->curr_repeating);
            while (section_executing->match_entry)
            {
                struct TIMER_match_point *tmp = section_executing->match_entry;
                section_executing->match_entry = tmp->link_next;

                glist_push_back(&MATCH_config_pool, tmp);
            }
            glist_push_back(&MATCH_config_pool, section_executing);

            section_executing = glist_pop(&ctx->config.MATCH.list);
            ctx->config.MATCH.list_count --;

            ctx->curr_repeating = 0;
        }
        ctx->config.MATCH.section_executing = section_executing;

        if (! section_executing || ! section_executing->match_entry)
            return false;
        if (0 != ctx->curr_repeating && TIMER_HAL_feature.MATCH_count >= ctx->config.MATCH.section_executing->match_count)
            return true;

        ctx->config.MATCH.tick_shift = 0;
        ctx->config.MATCH.curr = section_executing->match_entry;
    }

    uint8_t idx = 0;
    while (idx < TIMER_HAL_feature.MATCH_count)
    {
        struct TIMER_match_point *curr = ctx->config.MATCH.curr;
        ctx->config.MATCH.curr = curr->link_next;

        if (! curr->link_next)
        {
            /// @fill INFINITE
            for (; idx < TIMER_MR_RST; idx ++)
                TIMER_HAL_setmatch(ctx->dev, idx, (uint32_t)-1);

            ctx->dev_match_points[idx] = curr;
            TIMER_HAL_setmatch(ctx->dev, idx, curr->value - ctx->config.MATCH.tick_shift);

            ctx->config.MATCH.tick_shift = curr->value;
            break;
        }
        else
        {
            ctx->dev_match_points[idx] = curr;
            TIMER_HAL_setmatch(ctx->dev, idx, curr->value - ctx->config.MATCH.tick_shift);

            if (idx == TIMER_MR_RST)
                ctx->config.MATCH.tick_shift = curr->value;

            idx ++;
        }
    }
    return idx > 0;
}

/***************************************************************************
 *  @Interrupt callback
 ***************************************************************************/
void TIMER_HAL_intr_callback(int nb, uint8_t ch, uint32_t val)
{
    ARG_UNUSED(val);

    struct TIMER_context *ctx = &TIMER_context[nb];

    if (TIMER_MATCH == ctx->mode)
    {
        struct TIMER_match_point *match = ctx->dev_match_points[ch];
        ctx->callback(match->id, match->arg, ctx->curr_repeating);

        if (ch == TIMER_MR_RST)
        {
            if (CTX_match_load_next(ctx))
                TIMER_HAL_start(ctx->dev);
            else
                TIMER_HAL_stop(ctx->dev);
        }
        else
        {
            if (TIMER_HAL_feature.MATCH_count < ctx->config.MATCH.section_executing->match_count)
                TIMER_HAL_setmatch(ctx->dev, ch, (uint32_t)-1);
        }
    }
    else if (TIMER_PWM == ctx->mode)
    {
        ctx->callback((uint16_t)nb, &ctx->config.PWM, ctx->curr_repeating);
        ctx->curr_repeating ++;
    }
}
