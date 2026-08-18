/**
 * @file app_knock.h
 * @brief Desk double-knock wake detection for the Study AI Assistant.
 *
 * Listens to the always-on microphone stream (AI_USER_EVT_MIC_DATA) instead of a
 * dedicated vibration sensor. A double fist-tap on the desk produces two sharp,
 * closely-spaced acoustic bursts on the onboard mic; when recognized while the AI
 * chat is idle, it triggers the same wake path as the physical button
 * (ai_mode_handle_key(TDL_BUTTON_PRESS_SINGLE_CLICK)).
 */

#ifndef __APP_KNOCK_H__
#define __APP_KNOCK_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
********************function declaration********************
***********************************************************/
/**
 * @brief Initialize the knock detector (worker thread).
 * @return OPRT_OK on success.
 */
OPERATE_RET app_knock_init(void);

/**
 * @brief Feed one mic PCM slice into the detector.
 * @param data     PCM16LE audio slice (may be NULL when the stream stops).
 * @param data_len byte length of the slice.
 * @return OPRT_OK always.
 */
OPERATE_RET app_knock_feed_mic(uint8_t *data, uint32_t data_len);

#ifdef __cplusplus
}
#endif

#endif /* __APP_KNOCK_H__ */