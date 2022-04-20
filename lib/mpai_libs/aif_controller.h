/*
 * @file
 * @brief Headers of a MPAI AIF Controller, according to the specs V1
 * 
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MPAI_LIBS_AIF_CONTROLLER_H
#define MPAI_LIBS_AIF_CONTROLLER_H

#include <drivers/gpio.h>
#include <drivers/led.h>
#include <drivers/i2c.h>
#include <drivers/spi.h>
#include <drivers/sensor.h>
#include <usb/usb_device.h>
#include <drivers/uart.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <logging/log.h>

#include <errno.h>
#include <sys/byteorder.h>

#ifdef CONFIG_APP_TEST_WRITE_TO_FLASH
	#include <flash_store.h>
#endif
#include <parson.h>

#include <aiw_cae_rev.h>

#ifdef CONFIG_COAP_SERVER
	#include <coap_connect.h>
#endif
#ifdef CONFIG_MPAI_CONFIG_STORE	
	#include <config_store.h>
#endif

#define WHOAMI_REG 0x0F
#define WHOAMI_ALT_REG 0x4F

/**
 * @brief Initialize the MPAI Libs AIF Controller
 * 
 */
mpai_error_t MPAI_AIFU_Controller_Initialize();

/**
 * @brief Destroy the MPAI Libs AIF Controller
 * 
 * @return mpai_error_t 
 */
mpai_error_t MPAI_AIFU_Controller_Destroy();

/**
 * @brief Start specified MPAI AIW
 * 
 * @param name name of the AIW
 * @param AIW_ID AIW_ID generated
 * @return error_t 
 */
mpai_error_t MPAI_AIFU_AIW_Start(const char* name, int* AIW_ID);

/**
 * @brief Start specified MPAI AIW
 * 
 * @param name name of the AIW
 * @param AIW_ID AIW_ID generated
 * @return error_t 
 */
mpai_error_t MPAI_AIFU_AIW_Pause(int AIW_ID);

/**
 * @brief Resume specified MPAI AIW
 * 
 * @param AIW_ID 
 * @return mpai_error_t 
 */
mpai_error_t MPAI_AIFU_AIW_Resume(int AIW_ID);

/**
 * @brief Stop specified MPAI AIW
 * 
 * @param AIW_ID 
 * @return mpai_error_t 
 */
mpai_error_t MPAI_AIFU_AIW_Stop(int AIW_ID);

/**
 * @brief Get AIM Status of an AIW
 * 
 * @param AIW_ID 
 * @param name 
 * @param status 
 * @return error_t 
 */
mpai_error_t MPAI_AIFU_AIM_GetStatus(int AIW_ID, const char* name, int* status);

#endif