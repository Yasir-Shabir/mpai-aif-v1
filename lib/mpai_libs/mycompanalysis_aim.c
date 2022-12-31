/*
 * @file
 * @brief Implementation of an AIM that verify if the MYCOMPanalysis exercises are doing in a correct way:
 * 1. the device has to be stationary when there is a volume peak
 * 2. the device has to be started to move when a volume peak it's ended
 * 
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include"mycompanalysis_aim.h"
#include <logging/log.h>

LOG_MODULE_REGISTER(MPAI_LIBS_MYCOMPANALYSIS_AIM, LOG_LEVEL_INF);

/*************** DEFINE ***************/

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

/* polling every XXX(ms) to check new MYCOMP messages */
#define CONFIG_MYCOMPANALYSIS_MYCOMP_TIMEOUT_MS 3000
/* polling every XXX(ms) to check new volume peak messages */
#define CONFIG_MYCOMPANALYSIS_MIC_PEAK_TIMEOUT_MS 1000

/*************** STATIC ***************/
static const struct device *led0, *led1;

static void show_movement_error()
{
	// Show error blinking leds
	int i, on = 1;
	for (i = 0; i < 6; i++)
	{
		gpio_pin_set(led0, DT_GPIO_PIN(DT_ALIAS(led0), gpios), on);
		gpio_pin_set(led1, DT_GPIO_PIN(DT_ALIAS(led1), gpios), !on);
		k_sleep(K_MSEC(100));
		on = (on == 1) ? 0 : 1;
	}
}

/**************** THREADS **********************/

static k_tid_t subscriber_mycompanalysis_thread_id;

K_THREAD_STACK_DEFINE(thread_sub_mycompanalysis_stack_area, STACKSIZE);
static struct k_thread thread_sub_mycompanalysis_sens_data;

/* SUBSCRIBER */

void th_subscribe_mycompanalysis_data(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	mpai_message_t aim_mycomp_message;
	mpai_message_t aim_peak_audio_message;
	int64_t last_event_peak_audio = 0;

	LOG_DBG("START SUBSCRIBER");

	while (1)
	{
		// poll updates from MYCOMP_DATA_CHANNEL
		int ret_mycomp = MPAI_MessageStore_poll(message_store_mycompanalysis_aim, mycompanalysis_aim_subscriber, K_MSEC(CONFIG_MYCOMPANALYSIS_MYCOMP_TIMEOUT_MS), MYCOMP_DATA_CHANNEL);

		if (ret_mycomp > 0)
		{
			// get the message from MYCOMP_DATA_CHANNEL
			MPAI_MessageStore_copy(message_store_mycompanalysis_aim, mycompanalysis_aim_subscriber, MYCOMP_DATA_CHANNEL, &aim_mycomp_message);
			LOG_DBG("Received from timestamp %lld\n", aim_mycomp_message.timestamp);

			mycomp_data_t *mycomp_data = (mycomp_data_t *)aim_mycomp_message.data;

			// discard old messages
			if (aim_mycomp_message.timestamp > last_event_peak_audio || last_event_peak_audio == 0)
			{
				if (mycomp_data->mycomp_type == MYCOMP_STOPPED)
				{
					// if the event is STOPPED, the device waits for Audio Peak for a delay (CONFIG_MYCOMPANALYSIS_MIC_PEAK_TIMEOUT_MS)
					LOG_INF("MYCOMP STOPPED: Waiting for Audio Peak");
					k_sleep(K_MSEC(10));

					int ret_mic = MPAI_MessageStore_poll(message_store_mycompanalysis_aim, mycompanalysis_aim_subscriber, K_MSEC(CONFIG_MYCOMPANALYSIS_MIC_PEAK_TIMEOUT_MS), MIC_PEAK_DATA_CHANNEL);
				
					if (ret_mic > 0)
					{
						MPAI_MessageStore_copy(message_store_mycompanalysis_aim, mycompanalysis_aim_subscriber, MIC_PEAK_DATA_CHANNEL, &aim_peak_audio_message);
						last_event_peak_audio = aim_peak_audio_message.timestamp;
						// if the Audio Peak is recognized, the movement it's done in a correct way
						LOG_INF("MOVEMENT CORRECT!");
					}
					else if (ret_mic == 0)
					{
						last_event_peak_audio = 0;
						// if the Audio Peak is not recognized, the movement it's done in a wrong way
						LOG_ERR("MOVEMENT NOT CORRECT! Audio Peak NOT FOUND");

						show_movement_error();
					}
					else
					{
						last_event_peak_audio = 0;
						printk("ERROR: error while polling: %d\n", ret_mic);
						return;
					}
				} else if (mycomp_data->mycomp_type == MYCOMP_STARTED)
				{	
					// TODO
				}
			} else 
			{
				LOG_WRN("Discard old mycomp event");
			}
		}
		else if (ret_mycomp == 0)
		{
			printk("WARNING: Did not receive new mycomp data for %d. Continuing poll.\n", CONFIG_MYCOMPANALYSIS_MYCOMP_TIMEOUT_MS);
			LOG_WRN("MOVEMENT NOT RECOGNIZED");
			show_movement_error();
		}
		else
		{
			printk("ERROR: error while polling: %d\n", ret_mycomp);
			return;
		}
	}
}

/************** EXECUTIONS ***************/
mpai_error_t* mycompanalysis_aim_subscriber()
{
	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t *mycompanalysis_aim_start()
{
	// LED
	led0 = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(led0), gpios));
	gpio_pin_configure(led0, DT_GPIO_PIN(DT_ALIAS(led0), gpios),
					   GPIO_OUTPUT_ACTIVE |
						   DT_GPIO_FLAGS(DT_ALIAS(led0), gpios));

	led1 = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(led1), gpios));
	gpio_pin_configure(led1, DT_GPIO_PIN(DT_ALIAS(led1), gpios),
					   GPIO_OUTPUT_INACTIVE |
						   DT_GPIO_FLAGS(DT_ALIAS(led1), gpios));

	// CREATE SUBSCRIBER
	subscriber_mycompanalysis_thread_id = k_thread_create(&thread_sub_mycompanalysis_sens_data, thread_sub_mycompanalysis_stack_area,
										 K_THREAD_STACK_SIZEOF(thread_sub_mycompanalysis_stack_area),
										 th_subscribe_mycompanalysis_data, NULL, NULL, NULL,
										 PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&thread_sub_mycompanalysis_sens_data, "thread_sub_mycompanalysis");

	// START THREAD
	k_thread_start(subscriber_mycompanalysis_thread_id);

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t *mycompanalysis_aim_stop()
{
	k_thread_abort(subscriber_mycompanalysis_thread_id);
	LOG_INF("Execution stopped");

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t *mycompanalysis_aim_resume()
{
	k_thread_resume(subscriber_mycompanalysis_thread_id);
	LOG_INF("Execution resumed");

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t *mycompanalysis_aim_pause()
{
	k_thread_suspend(subscriber_mycompanalysis_thread_id);
	LOG_INF("Execution paused");

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}