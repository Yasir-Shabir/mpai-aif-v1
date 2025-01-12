/*
 * @file
 * @brief Implementation of an AIM that recognizes the motion of device (Start, Stop, ecc...)
 * 
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mycomp_aim.h"
#include <logging/log.h>

LOG_MODULE_REGISTER(MPAI_LIBS_MYCOMPMOTION_AIM, LOG_LEVEL_INF);

/*************** DEFINE ***************/

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

/* min delay used to detect when mcu is stopped */
#define MCU_MIN_DETECTED_STOP_DELAY_MS 100

/* delay in polling from sensors*/
#define SENSORS_DATA_POLLING_MS 1000

/* parameters to identify correct motion events, like start and stop: at the moment, we have find them doing some tests 
 * because the total acceleration is not "9.81" perfectly in our test device
 */
#define ACCEL_TOT_THRESHOLD_MIN 9.5
#define ACCEL_TOT_THRESHOLD_MAX 10.5

/*************** STATIC ***************/
/* last time (in ms) that mcu has stopped */
static int64_t mcu_has_stopped_ts = 0.0;
/* motion data to send to message store */
static mycomp_data_t mycomp_motion_data = {.mycomp_accel_total = -1.0, .mycomp_type = MYCOMP_UNKNOWN};

static void publish_mycomp_to_message_store(MYCOMP_TYPE mycomp_motion_type, float mycomp_accel_total)
{
	//LOG_INF("Start of publish_MOTION_to_message_store");
	mycomp_motion_data.mycomp_type = mycomp_motion_type;
	mycomp_motion_data.mycomp_accel_total = mycomp_accel_total;
	
	// Publish sensor message 
	mpai_message_t msg = {.data = &mycomp_motion_data,.timestamp = k_uptime_get()};

	MPAI_MessageStore_publish(message_store_mycomp_aim, &msg, MYCOMP_DATA_CHANNEL);

	LOG_DBG("Message mycomp motion published");
}

/**************** THREADS **********************/

static k_tid_t subscriber_mycomp_thread_id;

K_THREAD_STACK_DEFINE(thread_sub_mycomp_stack_area, STACKSIZE);
static struct k_thread thread_sub_mycomp_sens_data;

void th_subscribe_mycomp_data(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	mpai_message_t aim_message;

	LOG_DBG("START SUBSCRIBER");

	while (1)
	{
		/* this function will return once new data has arrived, or upon timeout (1000ms in this case). */
		int ret = MPAI_MessageStore_poll(message_store_mycomp_aim, mycomp_aim_subscriber, K_MSEC(SENSORS_DATA_POLLING_MS), SENSORS_DATA_CHANNEL);

		/* ret returns:
		 * a positive value if new data was successfully returned
		 * 0 if the poll timed out
		 * negative if an error occured while polling
		 */
		if (ret > 0)
		{
			MPAI_MessageStore_copy(message_store_mycomp_aim, mycomp_aim_subscriber, SENSORS_DATA_CHANNEL, &aim_message);
			LOG_DBG("Received from timestamp of mycomp motion%lld\n", aim_message.timestamp);

			sensor_result_t *sensor_data = (sensor_result_t *)aim_message.data;

			#ifdef CONFIG_LSM6DSL
				float accel_x = sensor_value_to_double(&(sensor_data->lsm6dsl_accel[0]));
				float accel_y = sensor_value_to_double(&(sensor_data->lsm6dsl_accel[1]));
				float accel_z = sensor_value_to_double(&(sensor_data->lsm6dsl_accel[2]));
				// compute vectorial product to get the total acceleration
				float accel_tot = sqrt(accel_x*accel_x + accel_y*accel_y + accel_z*accel_z);

				// algorithm to check if mcu is stopped or not
				// 1. check if the total acceleration is between the MIN and the MAX threshold
				if (accel_tot >= ACCEL_TOT_THRESHOLD_MIN && accel_tot <= ACCEL_TOT_THRESHOLD_MAX)
				{
					if (mcu_has_stopped_ts != 0 && aim_message.timestamp - mcu_has_stopped_ts >=  MCU_MIN_DETECTED_STOP_DELAY_MS)
					{
						// MCU is stopped but it doesn't publish event, because it was already stopped
					}
					// 2. check if mcu was moving previously
					else if (mcu_has_stopped_ts == 0)
					{
						mcu_has_stopped_ts = aim_message.timestamp;	

						printk("MCU Motion stopped: Accel (m.s-2): tot: %.5f\n", accel_tot);

						publish_mycomp_to_message_store(MYCOMP_STOPPED, accel_tot);
					}
				} else 
				{	
					if (mcu_has_stopped_ts > 0) 
					{
						printk("MCU Motion OF MYCOMP started: Accel (m.s-2): tot: %.5f\n", accel_tot);

						// at the moment is commented
						// publish_motion_to_message_store(STARTED, accel_tot);
					}
					// reset last time that mcu is stopped
					mcu_has_stopped_ts = 0;
				}
			#endif
		}
		else if (ret == 0)
		{
			printk("WARNING: Did not receive new data for %dms. Continuing poll.\n", SENSORS_DATA_POLLING_MS);
		}
		else
		{
			printk("ERROR: error while polling: %d\n", ret);
			return;
		}
	}
}

/************** EXECUTIONS ***************/
mpai_error_t* mycomp_aim_subscriber()
{
	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t *mycomp_aim_start()
{
	mcu_has_stopped_ts = k_uptime_get();

	// CREATE SUBSCRIBER
	subscriber_mycomp_thread_id = k_thread_create(&thread_sub_mycomp_sens_data, thread_sub_mycomp_stack_area,
										 K_THREAD_STACK_SIZEOF(thread_sub_mycomp_stack_area),
										 th_subscribe_mycomp_data, NULL, NULL, NULL,
										 PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&thread_sub_mycomp_sens_data, "thread_sub_motion");

	// START THREAD
	k_thread_start(subscriber_mycomp_thread_id);

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t *mycomp_aim_stop()
{
	k_thread_abort(subscriber_mycomp_thread_id);
	LOG_INF("Execution stopped");

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t *mycomp_aim_resume()
{
	k_thread_resume(subscriber_mycomp_thread_id);
	LOG_INF("Execution resumed");

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t *mycomp_aim_pause()
{
	k_thread_suspend(subscriber_mycomp_thread_id);
	LOG_INF("Execution paused");

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}