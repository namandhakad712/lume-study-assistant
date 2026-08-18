/**
 * @file app_knock.c
 * @brief Desk double-knock wake detection for the Study AI Assistant.
 *
 * The microphone stream is captured continuously by the AI audio component, and
 * every PCM slice is delivered to the app via AI_USER_EVT_MIC_DATA. This module
 * computes a cheap energy level per slice and runs a small state machine that
 * recognizes a double knock (two short loud bursts 100-800 ms apart). When a
 * double knock is confirmed and the chat is idle, a worker thread fires the same
 * key event as the physical button: ai_mode_handle_key(TDL_BUTTON_PRESS_SINGLE_CLICK).
 *
 * No registers are consumed - everything happens on the existing codec stream.
 */

#include "tal_api.h"

#include "ai_user_event.h"
#include "ai_manage_mode.h"

#include "app_knock.h"

/***********************************************************
************************macro define************************
***********************************************************/
/* Adaptive noise floor: window over which the quiet-energy minimum is tracked. */
#define KNOCK_FLOOR_WINDOW_MS   (1000)

/* Hit threshold = max(noise_floor * gain, hard minimum). Tune on hardware. */
#define KNOCK_THRESH_GAIN       (8)
#define KNOCK_THRESH_MIN        (300)

/* A knock is a short burst: longer loud stretches (speech/music) are ignored. */
#define KNOCK_MAX_BURST_MS      (200)

/* Double knock timing: 2nd burst must start 100..800 ms after the 1st. */
#define KNOCK_MIN_GAP_MS        (100)
#define KNOCK_MAX_GAP_MS        (800)

/* Cooldown after a confirmed knock to avoid re-triggering on the wake alert. */
#define KNOCK_ARM_COOLDOWN_MS   (1500)

/* Worker thread poll period. */
#define KNOCK_WORKER_POLL_MS    (20)

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef enum {
    KNOCK_STATE_IDLE,
    KNOCK_STATE_ARMED,
    KNOCK_STATE_COOLDOWN,
} KNOCK_STATE_E;

/***********************************************************
***********************variable define**********************
***********************************************************/
static volatile bool     sg_knock_pending = false;
static KNOCK_STATE_E     sg_state = KNOCK_STATE_IDLE;
static uint32_t          sg_floor = 0;
static uint32_t          sg_cur_min = 0;
static uint32_t          sg_floor_win_start = 0;
static bool              sg_in_burst = false;
static uint32_t          sg_burst_start = 0;
static uint32_t          sg_last_knock_at = 0;
static THREAD_HANDLE     sg_worker = NULL;

/***********************************************************
***********************function define**********************
***********************************************************/
static void __app_knock_fire(void)
{
#if defined(ENABLE_BUTTON) && (ENABLE_BUTTON == 1)
    PR_NOTICE("app_knock: double knock detected -> wake AI chat");
    ai_mode_handle_key(TDL_BUTTON_PRESS_SINGLE_CLICK, NULL);
#endif
}

/**
 * @brief Worker thread: fires the wake trigger outside of the audio callback
 *        context.
 */
static void __app_knock_worker(void *arg)
{
    while (tal_thread_get_state(sg_worker) == THREAD_STATE_RUNNING) {
        if (sg_knock_pending) {
            sg_knock_pending = false;

            /* Only wake from idle - never interrupt listening/thinking/speaking. */
            if (ai_mode_get_state() == AI_MODE_STATE_IDLE) {
                __app_knock_fire();
            }
        }
        tal_system_sleep(KNOCK_WORKER_POLL_MS);
    }
}

static void __app_knock_reset_burst(void)
{
    sg_in_burst = false;
    sg_burst_start = 0;
}

OPERATE_RET app_knock_feed_mic(uint8_t *data, uint32_t data_len)
{
    uint32_t now = tal_system_get_millisecond();

    if (data == NULL || data_len == 0) {
        __app_knock_reset_burst();
        return OPRT_OK;
    }

    /* A knock only makes sense while the chat is idle. Reset the recognizer on
     * any non-idle state so a burst during listening can't straddle the return
     * to idle and fire a stale trigger. */
    if (ai_mode_get_state() != AI_MODE_STATE_IDLE) {
        if (sg_state == KNOCK_STATE_ARMED) {
            sg_state = KNOCK_STATE_IDLE;
        }
        /* Keep updating the noise floor even while speaking, so the threshold is
         * right when we return to idle. */
    }

    /* Cheap energy = mean of |int16 sample| over the slice (16-bit mono PCM). */
    uint32_t n = data_len / 2;
    if (n == 0) {
        return OPRT_OK;
    }
    uint64_t sum = 0;
    int16_t *pcm = (int16_t *)data;
    for (uint32_t i = 0; i < n; i++) {
        int32_t v = pcm[i];
        sum += v < 0 ? (uint32_t)(-v) : (uint32_t)v;
    }
    uint32_t energy = (uint32_t)(sum / n);

    /* Adaptive noise floor: track the quiet-energy minimum over a rolling window. */
    if (now - sg_floor_win_start >= KNOCK_FLOOR_WINDOW_MS) {
        sg_floor_win_start = now;
        if (sg_cur_min > 0) {
            sg_floor = sg_cur_min;
        }
        sg_cur_min = (uint32_t)-1;
    }
    if (energy < sg_cur_min) {
        sg_cur_min = energy;
    }

    uint32_t threshold = sg_floor * KNOCK_THRESH_GAIN;
    if (threshold < KNOCK_THRESH_MIN) {
        threshold = KNOCK_THRESH_MIN;
    }

    if (sg_state == KNOCK_STATE_COOLDOWN) {
        if (now - sg_last_knock_at >= KNOCK_ARM_COOLDOWN_MS) {
            sg_state = KNOCK_STATE_IDLE;
        }
        return OPRT_OK;
    }

    bool loud = (energy > threshold);

    if (!loud) {
        if (sg_in_burst) {
            /* Burst ended: evaluate it. */
            uint32_t burst_ms = now - sg_burst_start;
            __app_knock_reset_burst();

            if (burst_ms <= KNOCK_MAX_BURST_MS) {
                if (sg_state == KNOCK_STATE_ARMED &&
                    (sg_burst_start - sg_last_knock_at) >= KNOCK_MIN_GAP_MS) {
                    /* Second knock within the window: fire. */
                    sg_knock_pending = true;
                    sg_state = KNOCK_STATE_COOLDOWN;
                    sg_last_knock_at = now;
                } else {
                    /* First knock (or too-late second knock): re-arm. */
                    sg_state = KNOCK_STATE_ARMED;
                    sg_last_knock_at = sg_burst_start;
                }
            }
        }
    } else {
        if (!sg_in_burst) {
            sg_in_burst = true;
            sg_burst_start = now;

            /* If the burst starts too long after the last knock, it can't be a
             * double knock - re-arm. */
            if (sg_state == KNOCK_STATE_ARMED &&
                (now - sg_last_knock_at) > KNOCK_MAX_GAP_MS) {
                sg_state = KNOCK_STATE_IDLE;
            }
        }
    }

    return OPRT_OK;
}

OPERATE_RET app_knock_init(void)
{
    OPERATE_RET rt = OPRT_OK;

    THREAD_CFG_T thrd_cfg = {
        .priority = THREAD_PRIO_5,
        .stackDepth = 2 * 1024,
        .thrdname = "knock_worker",
        .psram_mode = 1,
    };
    rt = tal_thread_create_and_start(&sg_worker, NULL, NULL, __app_knock_worker, NULL, &thrd_cfg);
    if (rt != OPRT_OK) {
        PR_ERR("app_knock: worker thread create failed rt=%d", rt);
        return rt;
    }

    sg_floor = 0;
    sg_cur_min = (uint32_t)-1;
    sg_floor_win_start = tal_system_get_millisecond();

    PR_NOTICE("app_knock: init ok, threshold gain=%d min=%d", KNOCK_THRESH_GAIN, KNOCK_THRESH_MIN);

    return OPRT_OK;
}