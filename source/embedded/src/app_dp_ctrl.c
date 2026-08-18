/**
 * @file app_dp_ctrl.c
 * @brief Study AI Assistant DP control.
 *
 * Wires the product DPs to the SDK:
 *   DP 2  status              (ro)  - reported from AI mode state
 *   DP 6  volume_set          (rw)  - ai_chat_set_volume (see tuya_app_main.c)
 *   DP 9  conversational_mode (rw)  - ai_mode_switch (hold/key/weakup/free)
 *   DP 101 mute               (rw)  - ai_audio_input_stop/start
 *   DP 102 led_switch         (rw)  - LED override (AI modes also drive it)
 *   DP 103 study_mode         (rw)  - persisted subject context for the agent
 *   DP 104 focus_timer        (rw)  - countdown, chime + DP report at 0
 */

#include "tal_api.h"

#include "ai_chat_main.h"
#include "ai_audio_input.h"
#include "ai_audio_player.h"
#include "ai_manage_mode.h"
#include "tdl_led_manage.h"
#include "tuya_iot.h"
#include "tuya_iot_dp.h"

#include "app_dp_ctrl.h"
#include "tuya_dp_profile.h"

/***********************************************************
************************macro define************************
***********************************************************/
/* DP 9 conversational_mode enum values (cloud order == SDK order) */
#define DP_MODE_HOLD     0
#define DP_MODE_KEY      1
#define DP_MODE_WEAKUP   2
#define DP_MODE_FREE     3

/* DP 103 study_mode enum values (informational, stored raw) */
#define STUDY_MODE_GENERAL      0

/* Focus timer tick period. */
#define FOCUS_TICK_MS           (1000)

/* KV key for study mode persistence. */
#define KV_KEY_STUDY_MODE       "study_mode"

/***********************************************************
***********************typedef define***********************
***********************************************************/

/***********************************************************
***********************variable define**********************
***********************************************************/
static TDL_LED_HANDLE_T sg_led_hdl = NULL;
static bool             sg_led_override_off = false; /* user forced LED off */
static bool             sg_muted = false;
static uint8_t          sg_study_mode = STUDY_MODE_GENERAL;

static TIMER_ID         sg_focus_timer = NULL;
static uint32_t         sg_focus_remaining_s = 0; /* 0 = timer inactive */

/***********************************************************
***********************function define**********************
***********************************************************/
static OPERATE_RET __app_dp_report_value(uint8_t dpid, uint8_t type, int value)
{
    tuya_iot_client_t *client = tuya_iot_client_get();
    dp_obj_t           dp_obj = {0};

    dp_obj.id             = dpid;
    dp_obj.type           = type;
    dp_obj.value.dp_value = value;

    return tuya_iot_dp_obj_report(client, client->activate.devid, &dp_obj, 1, 0);
}

static void __app_dp_apply_led_override(void)
{
    if (sg_led_hdl == NULL) {
        return;
    }
    if (sg_led_override_off) {
        tdl_led_set_status(sg_led_hdl, TDL_LED_OFF);
    }
}

static void __app_dp_focus_tick(TIMER_ID timer_id, void *arg)
{
    if (sg_focus_remaining_s == 0) {
        return;
    }

    sg_focus_remaining_s--;

    if (sg_focus_remaining_s > 0) {
        return;
    }

    /* Countdown finished: chime + report 0 + stop ticking. */
    PR_NOTICE("app_dp: focus timer finished");
    ai_audio_player_alert(AI_AUDIO_ALERT_WAKEUP);
    __app_dp_report_value(DPID_FOCUS_TIMER, PROP_VALUE, 0);
    tal_sw_timer_stop(sg_focus_timer);
}

OPERATE_RET app_dp_set_chat_mode(uint8_t mode_idx)
{
    AI_CHAT_MODE_E mode;

    switch (mode_idx) {
    case DP_MODE_HOLD:
        mode = AI_CHAT_MODE_HOLD;
        break;
    case DP_MODE_KEY:
        mode = AI_CHAT_MODE_ONE_SHOT;
        break;
    case DP_MODE_WEAKUP:
        mode = AI_CHAT_MODE_WAKEUP;
        break;
    case DP_MODE_FREE:
        mode = AI_CHAT_MODE_FREE;
        break;
    default:
        PR_WARN("app_dp: unknown conversational_mode %d", mode_idx);
        return OPRT_INVALID_PARM;
    }

    PR_NOTICE("app_dp: switch chat mode -> %d", mode);
    return ai_mode_switch(mode);
}

OPERATE_RET app_dp_set_mute(bool mute)
{
    OPERATE_RET rt = OPRT_OK;

    if (sg_muted == mute) {
        return OPRT_OK;
    }
    sg_muted = mute;

    if (mute) {
        PR_NOTICE("app_dp: mute ON (stop mic)");
        rt = ai_audio_input_stop();
    } else {
        PR_NOTICE("app_dp: mute OFF (start mic)");
        rt = ai_audio_input_start();
    }

    /* Mic is off while muted, so the knock detector gets no stream either. */
    return rt;
}

OPERATE_RET app_dp_set_led_override(bool on)
{
    PR_NOTICE("app_dp: led_switch %s", on ? "on (AI drives)" : "off (forced)");
    sg_led_override_off = !on;
    __app_dp_apply_led_override();
    return OPRT_OK;
}

OPERATE_RET app_dp_set_study_mode(uint8_t mode_idx)
{
    sg_study_mode = mode_idx;

    tal_kv_set(KV_KEY_STUDY_MODE, (const uint8_t *)&sg_study_mode, sizeof(sg_study_mode));

    PR_NOTICE("app_dp: study_mode -> %d", sg_study_mode);

    /* Echo so the cloud/agent sees the confirmed value. */
    return __app_dp_report_value(DPID_STUDY_MODE, PROP_ENUM, sg_study_mode);
}

OPERATE_RET app_dp_set_focus_timer(uint32_t minutes)
{
    if (minutes == 0) {
        /* Cancel. */
        sg_focus_remaining_s = 0;
        tal_sw_timer_stop(sg_focus_timer);
        PR_NOTICE("app_dp: focus timer cancelled");
        return OPRT_OK;
    }

    sg_focus_remaining_s = minutes * 60;
    PR_NOTICE("app_dp: focus timer set -> %u min (%u s)", minutes, sg_focus_remaining_s);
    tal_sw_timer_start(sg_focus_timer, FOCUS_TICK_MS, TAL_TIMER_CYCLE);
    return OPRT_OK;
}

void app_dp_on_mode_state(AI_MODE_STATE_E state)
{
    uint8_t status = DP_STATUS_STANDBY;

    switch (state) {
    case AI_MODE_STATE_SPEAK:
        status = DP_STATUS_PLAYING;
        break;
    case AI_MODE_STATE_LISTEN:
    case AI_MODE_STATE_UPLOAD:
    case AI_MODE_STATE_THINK:
        status = DP_STATUS_ON;
        break;
    case AI_MODE_STATE_IDLE:
    default:
        status = DP_STATUS_STANDBY;
        break;
    }

    PR_DEBUG("app_dp: mode state %d -> status %d", state, status);
    app_dp_report_status(status);

    /* Re-assert the user's LED override after the mode layer drives the LED. */
    __app_dp_apply_led_override();
}

OPERATE_RET app_dp_report_status(uint8_t status)
{
    return __app_dp_report_value(DPID_STATUS, PROP_ENUM, status);
}

OPERATE_RET app_dp_ctrl_init(void)
{
    OPERATE_RET rt = OPRT_OK;

#if defined(ENABLE_LED) && (ENABLE_LED == 1)
    sg_led_hdl = tdl_led_find_dev(LED_NAME);
    if (sg_led_hdl == NULL) {
        PR_WARN("app_dp: LED \"%s\" not registered", LED_NAME);
    }
#endif

    /* Restore study mode from KV. */
    uint8_t *buf = NULL;
    size_t   len = 0;
    if (tal_kv_get(KV_KEY_STUDY_MODE, &buf, &len) == OPRT_OK && buf != NULL && len == sizeof(sg_study_mode)) {
        sg_study_mode = *buf;
        tal_kv_free(buf);
        PR_NOTICE("app_dp: study_mode restored -> %d", sg_study_mode);
    }

    rt = tal_sw_timer_create(__app_dp_focus_tick, NULL, &sg_focus_timer);
    if (rt != OPRT_OK) {
        PR_ERR("app_dp: focus timer create failed rt=%d", rt);
        return rt;
    }

    PR_NOTICE("app_dp: init ok (mute=%d led_override_off=%d study_mode=%d)",
              sg_muted, sg_led_override_off, sg_study_mode);
    return OPRT_OK;
}