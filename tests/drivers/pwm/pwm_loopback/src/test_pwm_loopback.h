/*
 * Copyright (c) 2020-2021 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_PWM_LOOPBACK_H__
#define __TEST_PWM_LOOPBACK_H__

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/ztest.h>

#define PWM_LOOPBACK_OUT_IDX 0
#define PWM_LOOPBACK_IN_IDX  1

#define PWM_LOOPBACK_NODE DT_INST(0, test_pwm_loopback)

#define PWM_LOOPBACK_OUT_CTLR \
	DT_PWMS_CTLR_BY_IDX(PWM_LOOPBACK_NODE, PWM_LOOPBACK_OUT_IDX)
#define PWM_LOOPBACK_OUT_CHANNEL \
	DT_PWMS_CHANNEL_BY_IDX(PWM_LOOPBACK_NODE, PWM_LOOPBACK_OUT_IDX)
#define PWM_LOOPBACK_OUT_FLAGS \
	DT_PWMS_FLAGS_BY_IDX(PWM_LOOPBACK_NODE, PWM_LOOPBACK_OUT_IDX)

#define PWM_LOOPBACK_IN_CTLR \
	DT_PWMS_CTLR_BY_IDX(PWM_LOOPBACK_NODE, PWM_LOOPBACK_IN_IDX)
#define PWM_LOOPBACK_IN_CHANNEL \
	DT_PWMS_CHANNEL_BY_IDX(PWM_LOOPBACK_NODE, PWM_LOOPBACK_IN_IDX)
#define PWM_LOOPBACK_IN_FLAGS \
	DT_PWMS_FLAGS_BY_IDX(PWM_LOOPBACK_NODE, PWM_LOOPBACK_IN_IDX)

// stm32 with four channel capture support drop every second capture. See
// https://github.com/zephyrproject-rtos/zephyr/issues/84044
#define PWM_LOOPBACK_DROPS_FRAMES       DT_PROP(PWM_LOOPBACK_IN_CTLR, four_channel_capture_support)
// STM32 without four channel capture support will drop the first few captures.
#define PWM_LOOPBACK_SKIP_FIRST_CAPTURE CONFIG_PWM_STM32

struct test_pwm {
	const struct device *dev;
	uint32_t pwm;
	pwm_flags_t flags;
};

struct test_pwm_callback_data {
	uint32_t *buffer;
	size_t buffer_len;
	size_t count;
	int status;
	struct k_sem sem;
	bool pulse_capture;
};

struct test_pwm_pattern_data {
	uint32_t *periods;
	uint32_t *pulses;
	size_t buffer_len;
	size_t count;
	bool discard_capture;
};

void get_test_pwms(struct test_pwm *out, struct test_pwm *in);

#endif /* __TEST_PWM_LOOPBACK_H__ */
