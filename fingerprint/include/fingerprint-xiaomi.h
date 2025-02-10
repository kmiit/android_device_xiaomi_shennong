/**
 * Copyright (C) 2025 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <hardware/hardware.h>
#include <hardware/hw_auth_token.h>

#define COMMAND_NIT 10
#define PARAM_NIT_FOD 1
#define PARAM_NIT_NONE 0

#define COMMAND_FOD_PRESS_STATUS 1
#define COMMAND_FOD_PRESS_X 2
#define COMMAND_FOD_PRESS_Y 3
#define PARAM_FOD_PRESSED 1
#define PARAM_FOD_RELEASED 0

#define FOD_STATUS_PATH "/sys/class/touch/touch_dev/fod_press_status"
#define FOD_STATUS_OFF 0
#define FOD_STATUS_ON 1

#define DISP_PARAM_PATH "/sys/devices/virtual/mi_display/disp_feature/disp-DSI-0/disp_param"
#define DISP_PARAM_LOCAL_HBM_MODE "9"
#define DISP_PARAM_LOCAL_HBM_OFF "0"
#define DISP_PARAM_LOCAL_HBM_ON "1"

#define FINGERPRINT_ACQUIRED_VENDOR 7

typedef struct fingerprint_hal {
    const char* class_name;
} fingerprint_hal_t;

static const fingerprint_hal_t kModules[] = {
        {"fortsense"},  {"fpc"},         {"fpc_fod"}, {"goodix"}, {"goodix:gf_fingerprint"},
        {"goodix_fod"}, {"goodix_fod6"}, {"silead"},  {"syna"}, {"goodix_us"},
};

#define FINGERPRINT_MODULE_API_VERSION_2_1 HARDWARE_MODULE_API_VERSION(2, 1)
#define FINGERPRINT_HARDWARE_MODULE_ID "fingerprint"

typedef enum fingerprint_msg_type {
    FINGERPRINT_ERROR = -1,
    FINGERPRINT_ACQUIRED = 1,
    FINGERPRINT_TEMPLATE_ENROLLING = 3,
    FINGERPRINT_TEMPLATE_REMOVED = 4,
    FINGERPRINT_AUTHENTICATED = 5,
    FINGERPRINT_TEMPLATE_ENUMERATING = 6,
    FINGERPRINT_CHALLENGE_GENERATED = 7,
    FINGERPRINT_CHALLENGE_REVOKED = 8,
    FINGERPRINT_AUTHENTICATOR_ID_RETRIEVED = 9,
    FINGERPRINT_AUTHENTICATOR_ID_INVALIDATED = 10,
    FINGERPRINT_RESET_LOCKOUT = 11,
} fingerprint_msg_type_t;

/**
 * Fingerprint errors are meant to tell the framework to terminate the current operation and ask
 * for the user to correct the situation. These will almost always result in messaging and user
 * interaction to correct the problem.
 *
 * For example, FINGERPRINT_ERROR_CANCELED should follow any acquisition message that results in
 * a situation where the current operation can't continue without user interaction. For example,
 * if the sensor is dirty during enrollment and no further enrollment progress can be made,
 * send FINGERPRINT_ACQUIRED_IMAGER_DIRTY followed by FINGERPRINT_ERROR_CANCELED.
 */
typedef enum fingerprint_error {
    FINGERPRINT_ERROR_HW_UNAVAILABLE = 1, /* The hardware has an error that can't be resolved. */
    FINGERPRINT_ERROR_UNABLE_TO_PROCESS = 2, /* Bad data; operation can't continue */
    FINGERPRINT_ERROR_TIMEOUT = 3, /* The operation has timed out waiting for user input. */
    FINGERPRINT_ERROR_NO_SPACE = 4, /* No space available to store a template */
    FINGERPRINT_ERROR_CANCELED = 5, /* The current operation can't proceed. See above. */
    FINGERPRINT_ERROR_UNABLE_TO_REMOVE = 6, /* fingerprint with given id can't be removed */
    FINGERPRINT_ERROR_LOCKOUT = 7, /* the fingerprint hardware is in lockout due to too many attempts */
    FINGERPRINT_ERROR_VENDOR_BASE = 1000 /* vendor-specific error messages start here */
} fingerprint_error_t;

/**
 * Fingerprint acquisition info is meant as feedback for the current operation.  Anything but
 * FINGERPRINT_ACQUIRED_GOOD will be shown to the user as feedback on how to take action on the
 * current operation. For example, FINGERPRINT_ACQUIRED_IMAGER_DIRTY can be used to tell the user
 * to clean the sensor.  If this will cause the current operation to fail, an additional
 * FINGERPRINT_ERROR_CANCELED can be sent to stop the operation in progress (e.g. enrollment).
 * In general, these messages will result in a "Try again" message.
 */
typedef enum fingerprint_acquired_info {
    FINGERPRINT_ACQUIRED_GOOD = 0,
    FINGERPRINT_ACQUIRED_PARTIAL = 1, /* sensor needs more data, i.e. longer swipe. */
    FINGERPRINT_ACQUIRED_INSUFFICIENT = 2, /* image doesn't contain enough detail for recognition*/
    FINGERPRINT_ACQUIRED_IMAGER_DIRTY = 3, /* sensor needs to be cleaned */
    FINGERPRINT_ACQUIRED_TOO_SLOW = 4, /* mostly swipe-type sensors; not enough data collected */
    FINGERPRINT_ACQUIRED_TOO_FAST = 5, /* for swipe and area sensors; tell user to slow down*/
    FINGERPRINT_ACQUIRED_DETECTED = 6, /* when the finger is first detected. Used to optimize wakeup.
                                          Should be followed by one of the above messages */
    FINGERPRINT_ACQUIRED_VENDOR_BASE = 1000 /* vendor-specific acquisition messages start here */
} fingerprint_acquired_info_t;

typedef struct fingerprint_finger_id {
    uint32_t fid;
} fingerprint_finger_id_t;

typedef struct fingerprint_enroll {
    uint32_t fid;
    /* samples_remaining goes from N (no data collected, but N scans needed)
     * to 0 (no more data is needed to build a template). */
    uint32_t samples_remaining;
    uint64_t msg; /* Vendor specific message. Used for user guidance */
} fingerprint_enroll_t;

typedef struct fingerprint_iterator {
    uint32_t fid;
    uint32_t remaining_templates;
} fingerprint_iterator_t;

typedef fingerprint_iterator_t fingerprint_enumerated_t;
typedef fingerprint_iterator_t fingerprint_removed_t;

typedef struct fingerprint_acquired {
    fingerprint_acquired_info_t acquired_info; /* information about the image */
} fingerprint_acquired_t;

typedef struct fingerprint_authenticated {
    fingerprint_finger_id_t finger;
    hw_auth_token_t hat;
} fingerprint_authenticated_t;

typedef struct fingerprint_vendor_extend {
    int64_t data;
} fingerprint_vendor_extend_t;

typedef struct fingerprint_msg {
    fingerprint_msg_type_t type;
    union {
        fingerprint_error_t error;
        fingerprint_enroll_t enroll;
        fingerprint_enumerated_t enumerated;
        fingerprint_removed_t removed;
        fingerprint_acquired_t acquired;
        fingerprint_authenticated_t authenticated;
        fingerprint_vendor_extend_t extend;
    } data;
} fingerprint_msg_t;

/* Callback function type */
typedef void (*fingerprint_notify_t)(const fingerprint_msg_t *msg);

/* Synchronous operation */
typedef struct fingerprint_device {
    /**
     * Common methods of the fingerprint device. This *must* be the first member
     * of fingerprint_device as users of this structure will cast a hw_device_t
     * to fingerprint_device pointer in contexts where it's known
     * the hw_device_t references a fingerprint_device.
     */
    struct hw_device_t common;

    /**
     * Client provided callback function to receive notifications.
     * Do not set by hand, use the function above instead.
     */
    fingerprint_notify_t notify;

    /**
     * Set notification callback:
     * Registers a user function that would receive notifications from the HAL
     * The call will block if the HAL state machine is in busy state until HAL
     * leaves the busy state.
     *
     * Function return: 0 if callback function is successfuly registered
     *                  or a negative number in case of error, generally from the errno.h set.
     */
    int (*set_notify)(struct fingerprint_device *dev, fingerprint_notify_t notify);

    /**
     * generateChallenge:
     *
     * Begins a secure transaction request. Note that the challenge by itself is not useful. It only
     * becomes useful when wrapped in a verifiable message such as a HardwareAuthToken.
     *
     * Notify with:
     *  message type: FINGERPRINT_CHALLENGE_GENERATED(7)
     *          data: uint64_t challenge
     * 
     * Callbacks that signify the end of this operation's lifecycle:
     *   - ISessionCallback#onChallengeGenerated
     */
    uint64_t (*generateChallenge)(struct fingerprint_device *dev);

    /**
     * revokeChallenge:
     *
     * Revokes a challenge that was previously generated. Note that if a non-existent challenge is
     * provided, the HAL must still notify the framework using ISessionCallback#onChallengeRevoked.
     *
     * Notify with:
     *  message type: FINGERPRINT_CHALLENGE_REVOKED(8)
     *          data: uint64_t challenge
     *
     * Callbacks that signify the end of this operation's lifecycle:
     *   - ISessionCallback#onChallengeRevoked
     *
     * @param challenge Challenge that should be revoked.
     */
    uint32_t (*revokeChallenge)(struct fingerprint_device *dev, uint64_t challenge);

    /**
     * enroll:
     *
     * A request to add a fingerprint enrollment.
     * 
     * Notify with:
     *  message type: FINGERPRINT_TEMPLATE_ENROLLING(3)
     *          data: long challenge
     * 
     * Callbacks that signify the end of this operation's lifecycle:
     *   - ISessionCallback#onError
     *   - ISessionCallback#onEnrollmentProgress(enrollmentId, remaining=0)
     * 
     * @param hat Containing the challenge in the "challenge" field.
     */
    uint32_t (*enroll)(struct fingerprint_device *dev, const hw_auth_token_t *hat);

    /**
     * getAuthenticatorId:
     *
     * MUST return 0 via ISessionCallback#onAuthenticatorIdRetrieved for sensors that are configured
     * as SensorStrength::WEAK or SensorStrength::CONVENIENCE.
     * 
     * Notify with:
     *  message type: FINGERPRINT_AUTHENTICATOR_ID_RETRIEVED(9)
     *         data: uint64_t authenticatorId
     * 
     * Callbacks that signify the end of this operation's lifecycle:
     *   - ISessionCallback#onAuthenticatorIdRetrieved
     */
    uint64_t (*getAuthenticatorId)(struct fingerprint_device *dev);

    /**
     * invalidateAuthenticatorId:
     *
     * This operation only applies to sensors that are configured as SensorStrength::STRONG. If
     * invoked by the framework for sensors of other strengths, the HAL should immediately invoke
     * ISessionCallback#onAuthenticatorIdInvalidated.
     * 
     * Notify with:
     *  message type: FINGERPRINT_AUTHENTICATOR_ID_INVALIDATED(10)
     *          data: long newAuthenticatorId
     * 
     * Callbacks that signify the end of this operation's lifecycle:
     *   - ISessionCallback#onAuthenticatorIdInvalidated
     */
    uint64_t (*invalidateAuthenticatorId)(struct fingerprint_device *dev);

    /**
     * cancel:
     * 
     * Cancel an progressing enroll
     * 
     * Note that cancellation (see common::ICancellationSignal) must be followed with an
     * Error::CANCELED message.
     */
    uint32_t (*cancel)(struct fingerprint_device *dev);

    /**
     * enumerate:
     *
     * A request to enumerate (list) the enrollments for this (sensorId, userId) pair. The framework
     * typically uses this to ensure that its cache is in sync with the HAL.
     * 
     * Notify with:
     *  message type: FINGERPRINT_TEMPLATE_ENUMERATING(6)
     *          data: { uint32_t enrollemnt, 
     *                  uint32_t remaining_templates}
     * 
     * Callbacks that signify the end of this operation's lifecycle:
     *   - ISessionCallback#onEnrollmentsEnumerated
     */
    uint32_t (*enumerate)(struct fingerprint_device *dev);

    /**
     * remove:
     * 
     * A request to remove the enrollments for this (sensorId, userId) pair.
     * 
     * Notify with:
     *  message type: FINGERPRINT_TEMPLATE_REMOVED(4)
     *          data: { int enrollment,
     *                  int remaining_templates}
     * 
     * Callbacks that signify the end of this operation's lifecycle:
     *   - ISessionCallback#onEnrollmentsRemoved
     * 
     * @param enrollmentIds a list of enrollments that should be removed.
     * @param count the count need to be removed, usually is the enrollmentIds.size()
     */
    uint64_t (*remove)(struct fingerprint_device *dev, const int32_t *enrollmentIds, uint32_t count);

    /**
     * setActiveGroup：
     * 
     * Setup the path of current user's fingerprint data.
     * 
     * @param userid id of current user.
     * @param store_path the path to store fingerprint data.
     */
    uint32_t (*setActiveGroup)(struct fingerprint_device *dev, uint32_t userid,
                            const char *store_path);
    
    /**
     * authenticate:
     *
     * A request to start looking for fingerprints to authenticate.
     * 
     * Notify with:
     *  message type: FINGERPRINT_AUTHENTICATED(5)
     *          data: { uint32_t fingerprintid,
     *                  hw_auth_token_t hat}
     * 
     * Callbacks that signify the end of this operation's lifecycle:
     *   - ISessionCallback#onError
     *   - ISessionCallback#onAuthenticationSucceeded
     * 
     *@param operationId For sensors configured as SensorStrength::STRONG, this must be used ONLY
     *                    upon successful authentication and wrapped in the HardwareAuthToken's
     *                    "challenge" field and sent to the framework via
     *                    ISessionCallback#onAuthenticationSucceeded. The operationId is an opaque
     *                    identifier created from a separate secure subsystem such as, but not
     *                    limited to KeyStore/KeyMaster. The HardwareAuthToken can then be used as
     *                    an attestation for the provided operation. For example, this is used to
     *                    unlock biometric-bound auth-per-use keys (see
     *                    setUserAuthenticationParameters in KeyGenParameterSpec.Builder and
     *                    KeyProtection.Builder).
     */
    uint32_t (*authenticate)(struct fingerprint_device *dev, uint64_t operation_id);

    /**
     * resetLockout:
     *
     * Requests the HAL to clear the lockout counter. Upon receiving this request, the HAL must
     * perform the following:
     *   1) Verify the authenticity and integrity of the provided HAT
     *   2) Verify that the timestamp provided within the HAT is relatively recent (e.g. on the
     *      order of minutes, not hours).
     * If either of the checks fail, the HAL must invoke ISessionCallback#onError with
     * Error::UNABLE_TO_PROCESS.
     * 
     * Lockout may be cleared in the following ways:
     *   1) ISession#resetLockout
     *   2) After a period of time, according to a rate-limiter.
     * 
     * Notify with:
     *  message type: FINGERPRINT_RESET_LOCKOUT(11)
     *          data: None
     * 
     * Callbacks that signify the end of this operation's lifecycle:
     *   - ISessionCallback#onLockoutCleared
     */
    uint32_t (*resetLockout)(struct fingerprint_device* dev, const hw_auth_token_t* hat);

    /**
     * onPointerDown:
     *
     * This operation only applies to sensors that are configured as
     * FingerprintSensorType::UNDER_DISPLAY_*. If invoked erroneously by the framework for sensors
     * of other types, the HAL must treat this as a no-op and return immediately.
     * 
     * @param pointerId See android.view.MotionEvent#getPointerId
     * @param x The distance in pixels from the left edge of the display.
     * @param y The distance in pixels from the top edge of the display.
     * @param minor See android.view.MotionEvent#getTouchMinor
     * @param major See android.view.MotionEvent#getTouchMajor
     * 
     * @deprecated use onPointerDownWithContext instead.
     * Also empty in Xiaomi's fingerprint module
     */
    void (*onPointerDown)(struct fingerprint_device *dev, int32_t pointerId, int32_t x, int32_t y, float minor, float major);

    /**
     * onPointerUp:
     *
     * This operation only applies to sensors that are configured as
     * FingerprintSensorType::UNDER_DISPLAY_*. If invoked for sensors of other types, the HAL must
     * treat this as a no-op and return immediately.
     * 
     * @param pointerId See android.view.MotionEvent#getPointerId
     *
     * @deprecated use onPointerUpWithContext instead.
     * Also empty in Xiaomi's fingerprint module
     */
    void (*onPointerUp)(struct fingerprint_device *dev, int32_t pointerId);

    /**
     * goodixExtCmd：
     * 
     * Xiaomi's vendor function to send extra command to fingerprint module.
     */
    uint64_t (*goodixExtCmd)(struct fingerprint_device *dev, int32_t cmd, int32_t param);

    /* Reserved for backward binary compatibility */
    void *reserved[2];
} fingerprint_device_t;

typedef struct fingerprint_module {
    /**
     * Common methods of the fingerprint module. This *must* be the first member
     * of fingerprint_module as users of this structure will cast a hw_module_t
     * to fingerprint_module pointer in contexts where it's known
     * the hw_module_t references a fingerprint_module.
     */
    struct hw_module_t common;
} fingerprint_module_t;
