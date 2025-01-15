/*
 * Copyright (c) 2020-2021 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/ztest.h>

#include "test_pwm_loopback.h"

#define TEST_PWM_PERIOD_NSEC 100000000
#define TEST_PWM_PULSE_NSEC   15000000
#define TEST_PWM_PERIOD_USEC    100000
#define TEST_PWM_PULSE_USEC      75000

enum test_pwm_unit {
	TEST_PWM_UNIT_NSEC,
	TEST_PWM_UNIT_USEC,
};

void get_test_pwms(struct test_pwm *out, struct test_pwm *in)
{
	/* PWM generator device */
	out->dev = DEVICE_DT_GET(PWM_LOOPBACK_OUT_CTLR);
	out->pwm = PWM_LOOPBACK_OUT_CHANNEL;
	out->flags = PWM_LOOPBACK_OUT_FLAGS;
	zassert_true(device_is_ready(out->dev), "pwm loopback output device is not ready");

	/* PWM capture device */
	in->dev = DEVICE_DT_GET(PWM_LOOPBACK_IN_CTLR);
	in->pwm = PWM_LOOPBACK_IN_CHANNEL;
	in->flags = PWM_LOOPBACK_IN_FLAGS;
	zassert_true(device_is_ready(in->dev), "pwm loopback input device is not ready");
}

static void test_capture(uint32_t period, uint32_t pulse, enum test_pwm_unit unit,
		  pwm_flags_t flags)
{
	struct test_pwm in;
	struct test_pwm out;
	uint64_t period_capture = 0;
	uint64_t pulse_capture = 0;
	int err = 0;

	get_test_pwms(&out, &in);

	switch (unit) {
	case TEST_PWM_UNIT_NSEC:
		TC_PRINT("Testing PWM capture @ %u/%u nsec\n",
			 pulse, period);
		err = pwm_set(out.dev, out.pwm, period, pulse, out.flags ^=
			      (flags & PWM_POLARITY_MASK));
		break;

	case TEST_PWM_UNIT_USEC:
		TC_PRINT("Testing PWM capture @ %u/%u usec\n",
			 pulse, period);
		err = pwm_set(out.dev, out.pwm, PWM_USEC(period),
			      PWM_USEC(pulse), out.flags ^=
			      (flags & PWM_POLARITY_MASK));
		break;

	default:
		TC_PRINT("Unsupported test unit");
		ztest_test_fail();
	}

	zassert_equal(err, 0, "failed to set pwm output (err %d)", err);

	switch (unit) {
	case TEST_PWM_UNIT_NSEC:
		err = pwm_capture_nsec(in.dev, in.pwm, flags, &period_capture,
				       &pulse_capture, K_NSEC(period * 10));
		break;

	case TEST_PWM_UNIT_USEC:
		err = pwm_capture_usec(in.dev, in.pwm, flags, &period_capture,
				       &pulse_capture, K_USEC(period * 10));
		break;

	default:
		TC_PRINT("Unsupported test unit");
		ztest_test_fail();
	}

	pwm_disable_capture(in.dev, in.pwm);

	if (err == -ENOTSUP) {
		TC_PRINT("capture type not supported\n");
		ztest_test_skip();
	}

	zassert_equal(err, 0, "failed to capture pwm (err %d)", err);

	if (flags & PWM_CAPTURE_TYPE_PERIOD) {
		zassert_within(period_capture, period, period / 100,
			       "period capture off by more than 1%");
	}

	if (flags & PWM_CAPTURE_TYPE_PULSE) {
		zassert_within(pulse_capture, pulse, pulse / 100,
			       "pulse capture off by more than 1%");
	}
}

ZTEST_USER(pwm_loopback, test_pulse_capture)
{
	test_capture(TEST_PWM_PERIOD_NSEC, TEST_PWM_PULSE_NSEC,
		     TEST_PWM_UNIT_NSEC,
		     PWM_CAPTURE_TYPE_PULSE | PWM_POLARITY_NORMAL);
	test_capture(TEST_PWM_PERIOD_USEC, TEST_PWM_PULSE_USEC,
		     TEST_PWM_UNIT_USEC,
		     PWM_CAPTURE_TYPE_PULSE | PWM_POLARITY_NORMAL);
}

ZTEST_USER(pwm_loopback, test_pulse_capture_inverted)
{
	test_capture(TEST_PWM_PERIOD_NSEC, TEST_PWM_PULSE_NSEC,
		     TEST_PWM_UNIT_NSEC,
		     PWM_CAPTURE_TYPE_PULSE | PWM_POLARITY_INVERTED);
	test_capture(TEST_PWM_PERIOD_USEC, TEST_PWM_PULSE_USEC,
		     TEST_PWM_UNIT_USEC,
		     PWM_CAPTURE_TYPE_PULSE | PWM_POLARITY_INVERTED);
}

ZTEST_USER(pwm_loopback, test_period_capture)
{
	test_capture(TEST_PWM_PERIOD_NSEC, TEST_PWM_PULSE_NSEC,
		     TEST_PWM_UNIT_NSEC,
		     PWM_CAPTURE_TYPE_PERIOD | PWM_POLARITY_NORMAL);
	test_capture(TEST_PWM_PERIOD_USEC, TEST_PWM_PULSE_USEC,
		     TEST_PWM_UNIT_USEC,
		     PWM_CAPTURE_TYPE_PERIOD | PWM_POLARITY_NORMAL);
}

ZTEST_USER(pwm_loopback, test_period_capture_inverted)
{
	test_capture(TEST_PWM_PERIOD_NSEC, TEST_PWM_PULSE_NSEC,
		     TEST_PWM_UNIT_NSEC,
		     PWM_CAPTURE_TYPE_PERIOD | PWM_POLARITY_INVERTED);
	test_capture(TEST_PWM_PERIOD_USEC, TEST_PWM_PULSE_USEC,
		     TEST_PWM_UNIT_USEC,
		     PWM_CAPTURE_TYPE_PERIOD | PWM_POLARITY_INVERTED);
}

ZTEST_USER(pwm_loopback, test_pulse_and_period_capture)
{
	test_capture(TEST_PWM_PERIOD_NSEC, TEST_PWM_PULSE_NSEC,
		     TEST_PWM_UNIT_NSEC,
		     PWM_CAPTURE_TYPE_BOTH | PWM_POLARITY_NORMAL);
	test_capture(TEST_PWM_PERIOD_USEC, TEST_PWM_PULSE_USEC,
		     TEST_PWM_UNIT_USEC,
		     PWM_CAPTURE_TYPE_BOTH | PWM_POLARITY_NORMAL);
}

ZTEST_USER(pwm_loopback, test_capture_timeout)
{
	struct test_pwm in;
	struct test_pwm out;
	uint32_t period;
	uint32_t pulse;
	int err;

	get_test_pwms(&out, &in);

	err = pwm_set_cycles(out.dev, out.pwm, 100, 0, out.flags);
	zassert_equal(err, 0, "failed to set pwm output (err %d)", err);

	err = pwm_capture_cycles(in.dev, in.pwm, PWM_CAPTURE_TYPE_PULSE,
				 &period, &pulse, K_MSEC(1000));
	if (err == -ENOTSUP) {
		TC_PRINT("Pulse capture not supported, "
			 "trying period capture\n");
		err = pwm_capture_cycles(in.dev, in.pwm,
					 PWM_CAPTURE_TYPE_PERIOD, &period,
					 &pulse, K_MSEC(1000));
	}

	zassert_equal(err, -EAGAIN, "pwm capture did not timeout (err %d)",
		      err);
}

static void continuous_capture_callback(const struct device *dev,
					uint32_t pwm,
					uint32_t period_cycles,
					uint32_t pulse_cycles,
					int status,
					void *user_data)
{
	struct test_pwm_callback_data *data = user_data;

	if (data->count > data->buffer_len) {
		/* Safe guard in case capture is not disabled */
		return;
	}

	if (status != 0) {
		/* Error occurred */
		data->status = status;
		k_sem_give(&data->sem);
	}

	if (data->pulse_capture) {
		data->buffer[data->count++] = pulse_cycles;
	} else {
		data->buffer[data->count++] = period_cycles;
	}

	if (data->count > data->buffer_len) {
		data->status = 0;
		k_sem_give(&data->sem);
	}
}

ZTEST(pwm_loopback, test_continuous_capture)
{
	struct test_pwm in;
	struct test_pwm out;
	uint32_t buffer[10];
	struct test_pwm_callback_data data = {
		.buffer = buffer,
		.buffer_len = ARRAY_SIZE(buffer),
		.count = 0,
		.pulse_capture = true,
	};
	uint64_t usec = 0;
	int err;
	int i;

	get_test_pwms(&out, &in);

	memset(buffer, 0, sizeof(buffer));
	k_sem_init(&data.sem, 0, 1);

	err = pwm_set(out.dev, out.pwm, PWM_USEC(TEST_PWM_PERIOD_USEC),
		      PWM_USEC(TEST_PWM_PULSE_USEC), out.flags);
	zassert_equal(err, 0, "failed to set pwm output (err %d)", err);

	err = pwm_configure_capture(in.dev, in.pwm,
				    in.flags |
				    PWM_CAPTURE_MODE_CONTINUOUS |
				    PWM_CAPTURE_TYPE_PULSE,
				    continuous_capture_callback, &data);
	if (err == -ENOTSUP) {
		TC_PRINT("Pulse capture not supported, "
			 "trying period capture\n");
		err = pwm_configure_capture(in.dev, in.pwm,
					    in.flags |
					    PWM_CAPTURE_MODE_CONTINUOUS |
					    PWM_CAPTURE_TYPE_PERIOD,
					    continuous_capture_callback, &data);
		zassert_equal(err, 0, "failed to configure pwm input (err %d)",
			      err);
		data.pulse_capture = false;
	}

	err = pwm_enable_capture(in.dev, in.pwm);
	zassert_equal(err, 0, "failed to enable pwm capture (err %d)", err);

	err = k_sem_take(&data.sem, K_USEC(TEST_PWM_PERIOD_USEC * data.buffer_len * 10));
	zassert_equal(err, 0, "pwm capture timed out (err %d)", err);
	zassert_equal(data.status, 0, "pwm capture failed (err %d)", err);

	err = pwm_disable_capture(in.dev, in.pwm);
	zassert_equal(err, 0, "failed to disable pwm capture (err %d)", err);

	for (i = 0; i < data.buffer_len; i++) {
		err = pwm_cycles_to_usec(in.dev, in.pwm, buffer[i], &usec);
		zassert_equal(err, 0, "failed to calculate usec (err %d)", err);

		if (data.pulse_capture) {
			zassert_within(usec, TEST_PWM_PULSE_USEC, TEST_PWM_PULSE_USEC / 100,
				       "pulse capture off by more than 1%");
		} else {
			zassert_within(usec, TEST_PWM_PERIOD_USEC, TEST_PWM_PERIOD_USEC / 100,
				       "period capture off by more than 1%");
		}
	}
}

ZTEST(pwm_loopback, test_capture_busy)
{
	struct test_pwm in;
	struct test_pwm out;
	uint32_t buffer[10];
	struct test_pwm_callback_data data = {
		.buffer = buffer,
		.buffer_len = ARRAY_SIZE(buffer),
		.count = 0,
		.pulse_capture = true,
	};
	pwm_flags_t flags = PWM_CAPTURE_MODE_SINGLE |
		PWM_CAPTURE_TYPE_PULSE;
	int err;

	get_test_pwms(&out, &in);

	memset(buffer, 0, sizeof(buffer));
	k_sem_init(&data.sem, 0, 1);

	err = pwm_set_cycles(out.dev, out.pwm, 100, 0, out.flags);
	zassert_equal(err, 0, "failed to set pwm output (err %d)", err);

	err = pwm_configure_capture(in.dev, in.pwm, in.flags | flags,
				    continuous_capture_callback, &data);
	if (err == -ENOTSUP) {
		TC_PRINT("Pulse capture not supported, "
			 "trying period capture\n");
		flags = PWM_CAPTURE_MODE_SINGLE | PWM_CAPTURE_TYPE_PERIOD;
		err = pwm_configure_capture(in.dev, in.pwm, in.flags | flags,
					    continuous_capture_callback, &data);
		zassert_equal(err, 0, "failed to configure pwm input (err %d)",
			      err);
		data.pulse_capture = false;
	}

	err = pwm_enable_capture(in.dev, in.pwm);
	zassert_equal(err, 0, "failed to enable pwm capture (err %d)", err);

	err = pwm_configure_capture(in.dev, in.pwm, in.flags | flags,
				    continuous_capture_callback, &data);
	zassert_equal(err, -EBUSY, "pwm capture not busy (err %d)", err);

	err = pwm_enable_capture(in.dev, in.pwm);
	zassert_equal(err, -EBUSY, "pwm capture not busy (err %d)", err);

	err = pwm_disable_capture(in.dev, in.pwm);
	zassert_equal(err, 0, "failed to disable pwm capture (err %d)", err);
}

#if !PWM_LOOPBACK_DROPS_FRAMES
static void pattern_capture_callback(const struct device *dev, uint32_t pwm,
				     uint32_t period_cycles,
				     uint32_t pulse_cycles, int status,
				     void *user_data)
{
	struct test_pwm_pattern_data *data = user_data;

	if (data->count >= data->buffer_len) {
		/* Safe guard in case capture is not disabled */
		return;
	}
	if (status) {
		TC_PRINT("Capture_callback error: %d", status);
		return;
	}
	if (data->discard_capture) {
		data->discard_capture = false;
		return;
	}

	data->pulses[data->count] = pulse_cycles;
	data->periods[data->count] = period_cycles;
	data->count++;
}

static void pattern_complete_callback(const struct device *dev, void *user_data)
{
	struct k_sem *sem = user_data;
	k_sem_give(sem);
}

#define NUM_SAMPLES 10

ZTEST(pwm_loopback, test_set_pattern)
{
	struct test_pwm in;
	struct test_pwm out;
	uint32_t periods_in[NUM_SAMPLES], pulses_in[NUM_SAMPLES],
		periods_out[NUM_SAMPLES], pulses_out[NUM_SAMPLES];
	uint64_t periods_usec[NUM_SAMPLES], pulses_usec[NUM_SAMPLES];
	struct k_sem sem;
	struct test_pwm_pattern_data data_in = {
		.periods = periods_in,
		.pulses = pulses_in,
		.buffer_len = NUM_SAMPLES,
		.count = 0,
	};
	uint64_t usec = 0;
	uint64_t total_usec;
	uint64_t cycles_per_sec;
	int err;
	int i;

	get_test_pwms(&out, &in);

	memset(periods_in, 0, sizeof(periods_in));
	memset(pulses_in, 0, sizeof(pulses_in));

	k_sem_init(&sem, 0, 1);

	/* disable pwm. */
	err = pwm_set(out.dev, out.pwm, 0, 0, out.flags);
	zassert_equal(err, 0, "failed to set pwm output (err %d)", err);

	err = pwm_configure_capture(in.dev, in.pwm,
				    in.flags | PWM_CAPTURE_MODE_CONTINUOUS |
					    PWM_CAPTURE_TYPE_BOTH,
				    pattern_capture_callback, &data_in);
	if (err == -ENOTSUP) {
		TC_PRINT("Capture of pulse and period at the same time not "
			 "supported. Skipping test\n");
		return;
	}

	err = pwm_enable_capture(in.dev, in.pwm);
	zassert_equal(err, 0, "failed to enable pwm capture (err %d)", err);

	/* set some changing values for period and pulse. */
	for (i = 0; i < NUM_SAMPLES; i++) {
		periods_usec[i] = (i + 1) * 1200;
		pulses_usec[i] = (i + 1) * 400;
	}
	/* set the last sample to 0, so that we have a clean ending.*/
	periods_usec[NUM_SAMPLES - 1] = 0;
	pulses_usec[NUM_SAMPLES - 1] = 0;

	/* convert from usec to cycles. */
	pwm_get_cycles_per_sec(out.dev, out.pwm, &cycles_per_sec);
	total_usec = 0;
	for (i = 0; i < NUM_SAMPLES; i++) {
		periods_out[i] = (periods_usec[i] * cycles_per_sec /
				  (uint64_t)USEC_PER_SEC);
		pulses_out[i] = (pulses_usec[i] * cycles_per_sec /
				 (uint64_t)USEC_PER_SEC);

		total_usec += periods_usec[i];
	}

#if PWM_LOOPBACK_SKIP_FIRST_CAPTURE
	/* start the pattern. */
	err = pwm_set_pattern(out.dev, out.pwm, periods_out, pulses_out,
			      NUM_SAMPLES, out.flags, pattern_complete_callback,
			      &sem);
	if (err == -ENOSYS) {
		TC_PRINT("Pattern not supported. Skipping test\n");
		pwm_disable_capture(in.dev, in.pwm);
		return;
	}
	zassert_equal(err, 0, "failed to set pattern (err %d)", err);
	err = k_sem_take(&sem, K_USEC(total_usec * 10));

	/* We discard the first pattern iteration, because some drivers discard
	 * the first few samples in their capture and compare logic.*/
	data_in.count = 0;
	/* ignore the first capture now, as it it the long trail from the last
	 * pattern.*/
	data_in.discard_capture = true;
#endif

	/* start the pattern again */
	err = pwm_set_pattern(out.dev, out.pwm, periods_out, pulses_out,
			      NUM_SAMPLES, out.flags, pattern_complete_callback,
			      &sem);
	zassert_equal(err, 0, "failed to set pattern (err %d)", err);

	err = k_sem_take(&sem, K_USEC(total_usec * 10));
	zassert_equal(err, 0, "pwm capture timed out (err %d)", err);

	err = pwm_disable_capture(in.dev, in.pwm);
	zassert_equal(err, 0, "failed to disable pwm capture (err %d)", err);

	zassert_equal(data_in.count, NUM_SAMPLES - 2, "Did not capture enough samples!");

	for (i = 0; i < data_in.count; i++) {
		/* convert everything to usec, as the counters for capture and
		 * send could be different. */
		err = pwm_cycles_to_usec(in.dev, in.pwm, periods_in[i], &usec);
		zassert_equal(err, 0, "failed to calculate usec (err %d)", err);
		zassert_within(usec, periods_usec[i], periods_usec[i] / 100,
			       "period %d capture off by more than 1 percent "
			       "%lld vs %lld",
			       i, periods_usec[i], usec);

		err = pwm_cycles_to_usec(in.dev, in.pwm, pulses_in[i], &usec);
		zassert_equal(err, 0, "failed to calculate usec (err %d)", err);
		zassert_within(usec, pulses_usec[i], pulses_usec[i] / 100,
			       "pulse %d capture off by more than 1 percent "
			       "%lld vs %lld",
			       i, pulses_usec[i], usec);
	}
}
#endif // !PWM_LOOPBACK_DROPS_FRAMES
