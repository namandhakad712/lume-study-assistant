/**
 * @file app_chat_bot.c
 * @brief app_chat_bot module is used to
 * @version 0.1
 * @date 2025-03-25
 */

#include "tal_api.h"

#include "netmgr.h"

#include "ai_chat_main.h"
#include "app_chat_bot.h"
#include "app_dp_ctrl.h"
#include "app_knock.h"

/***********************************************************
************************macro define************************
***********************************************************/
#define PRINTF_FREE_HEAP_TTIME (10 * 1000)

/***********************************************************
***********************typedef define***********************
***********************************************************/

/***********************************************************
***********************const declaration********************
***********************************************************/

/***********************************************************
***********************variable define**********************
***********************************************************/
static TIMER_ID sg_printf_heap_tm;

/***********************************************************
***********************function define**********************
***********************************************************/
static void __printf_free_heap_tm_cb(TIMER_ID timer_id, void *arg)
{
#if defined(ENABLE_EXT_RAM) && (ENABLE_EXT_RAM == 1)
    uint32_t free_heap       = tal_system_get_free_heap_size();
    uint32_t free_psram_heap = tal_psram_get_free_heap_size();
    PR_INFO("Free heap size:%d, Free psram heap size:%d", free_heap, free_psram_heap);
#else
    uint32_t free_heap = tal_system_get_free_heap_size();
    PR_INFO("Free heap size:%d", free_heap);
#endif
}

static void __ai_chat_handle_event(AI_NOTIFY_EVENT_T *event)
{
    switch(event->type) {
        case AI_USER_EVT_MIC_DATA: {
            AI_NOTIFY_MIC_DATA_T *mic = (AI_NOTIFY_MIC_DATA_T *)event->data;
            if (mic) {
                app_knock_feed_mic(mic->data, mic->data_len);
            }
        }
        break;
        case AI_USER_EVT_MODE_STATE_UPDATE: {
            AI_MODE_STATE_E state = (AI_MODE_STATE_E)(intptr_t)event->data;
            app_dp_on_mode_state(state);
        }
        break;
        default:
        break;
    }
}

OPERATE_RET app_chat_bot_init(void)
{
    OPERATE_RET rt = OPRT_OK;

    AI_CHAT_MODE_CFG_T ai_chat_cfg = {
        .default_mode = AI_CHAT_MODE_ONE_SHOT, // turn-based: talk -> reply -> wait for next trigger
        .default_vol  = 70,
        .evt_cb       = __ai_chat_handle_event,
    };
    TUYA_CALL_ERR_RETURN(ai_chat_init(&ai_chat_cfg));

    // Desk double-knock wake detection (Study AI Assistant feature)
    TUYA_CALL_ERR_RETURN(app_knock_init());

    // Free heap size
    tal_sw_timer_create(__printf_free_heap_tm_cb, NULL, &sg_printf_heap_tm);
    tal_sw_timer_start(sg_printf_heap_tm, PRINTF_FREE_HEAP_TTIME, TAL_TIMER_CYCLE);

    return OPRT_OK;
}