/**
 * @file app_dp_ctrl.h
 * @brief Study AI Assistant DP control: conversational mode, status, mute,
 *        LED override, study mode and focus timer.
 */

#ifndef __APP_DP_CTRL_H__
#define __APP_DP_CTRL_H__

#include "tuya_cloud_types.h"
#include "ai_manage_mode.h"

#ifdef __cplusplus
extern "C" {
#endif

/* DP 2 "status" enum values (from cloud schema) */
#define DP_STATUS_STANDBY    0
#define DP_STATUS_PLAYING    3
#define DP_STATUS_ON         8
#define DP_STATUS_OFF        9

/**
 * @brief Initialize DP control (open LED handle, restore study mode from KV).
 * @return OPRT_OK on success.
 */
OPERATE_RET app_dp_ctrl_init(void);

/**
 * @brief Apply chat-mode DP 9 (hold/key/weakup/free -> AI mode).
 * @param mode_idx enum value from the cloud DP.
 * @return OPRT_OK on success.
 */
OPERATE_RET app_dp_set_chat_mode(uint8_t mode_idx);

/**
 * @brief Apply mute DP 101 (stop/start the mic pipeline).
 * @param mute true = muted.
 * @return OPRT_OK on success.
 */
OPERATE_RET app_dp_set_mute(bool mute);

/**
 * @brief Apply LED override DP 102.
 * @param on true = LED follows AI state, false = force off.
 * @return OPRT_OK on success.
 */
OPERATE_RET app_dp_set_led_override(bool on);

/**
 * @brief Apply study mode DP 103 (persist to KV, echo).
 * @param mode_idx enum value from the cloud DP.
 * @return OPRT_OK on success.
 */
OPERATE_RET app_dp_set_study_mode(uint8_t mode_idx);

/**
 * @brief Apply focus timer DP 104 (start countdown / cancel).
 * @param minutes 0 = cancel, >0 = start N-minute timer.
 * @return OPRT_OK on success.
 */
OPERATE_RET app_dp_set_focus_timer(uint32_t minutes);

/**
 * @brief Handle an AI mode state change: report DP 2 and re-apply any
 *        active LED override.
 * @param state new AI mode state.
 */
void app_dp_on_mode_state(AI_MODE_STATE_E state);

/**
 * @brief Report DP 2 status to the cloud.
 * @param status one of the DP_STATUS_* values.
 * @return OPRT_OK on success.
 */
OPERATE_RET app_dp_report_status(uint8_t status);

#ifdef __cplusplus
}
#endif

#endif /* __APP_DP_CTRL_H__ */