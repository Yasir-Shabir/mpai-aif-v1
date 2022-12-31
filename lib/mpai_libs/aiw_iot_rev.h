/*
 * @file
 * @brief Headers of the MPAI AIW IOT-REV (Rehabilitation Exercises Validation)
 *
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef MPAI_AIW_IOT_REV_H
#define MPAI_AIW_IOT_REV_H

#include <core_aim.h>
#include <sensors_aim.h>
#include <temp_limit_aim.h>
#include <data_mic_aim.h>
#include <motion_aim.h>
#include <rehabilitation_aim.h>
#include <message_store.h>
#include <aif_controller.h>
#include <mycomp_aim.h>
#include <mycompanalysis_aim.h>

#if defined(CONFIG_MPAI_CONFIG_STORE)
    #include <config_store.h>
#endif

// TODO: generate dinamically 
static int AIW_IOT_REV = 1;

#define MPAI_LIBS_AIF_NAME "demo"
#define MPAI_LIBS_IOT_REV_AIW_NAME "IOT-REV"
#define MPAI_LIBS_IOT_REV_AIM_DATA_MIC_NAME "VolumePeaksAnalysis"
#define MPAI_LIBS_IOT_REV_AIM_SENSORS_NAME "ControlUnitSensorsReading"
#define MPAI_LIBS_IOT_REV_AIM_TEMP_LIMIT_NAME "AIM_TEMP_LIMIT"
#define MPAI_LIBS_IOT_REV_AIM_MOTION_NAME "MotionRecognitionAnalysis"
#define MPAI_LIBS_IOT_REV_AIM_REHABILITATION_NAME "MovementsWithAudioValidation"

#define MPAI_LIBS_IOT_REV_AIM_MYCOMPANALYSIS_NAME "MycompMovementsWithAudioValidation"
#define MPAI_LIBS_IOT_REV_SENSORS_DATA_CHANNEL_NAME "SensorsDataChannel"
#define MPAI_LIBS_IOT_REV_MIC_BUFFER_DATA_CHANNEL_NAME "MicBufferDataChannel"
#define MPAI_LIBS_IOT_REV_MIC_PEAK_DATA_CHANNEL_NAME "MicPeakDataChannel"
#define MPAI_LIBS_IOT_REV_MOTION_DATA_CHANNEL_NAME "MotionDataChannel"

#define MPAI_LIBS_IOT_REV_MYCOMP_DATA_CHANNEL_NAME "MycompMotionDataChannel"
#define MPAI_LIBS_IOT_REV_MYCOMP_NAME "MycompMotionAnalysis"

/* AIW global message store */
extern MPAI_AIM_MessageStore_t* message_store_test_case_aiw;
/* AIM message stores */
extern MPAI_AIM_MessageStore_t* message_store_data_mic_aim;
extern MPAI_AIM_MessageStore_t* message_store_sensors_aim;
extern MPAI_AIM_MessageStore_t* message_store_temp_limit_aim;
extern MPAI_AIM_MessageStore_t* message_store_motion_aim;
extern MPAI_AIM_MessageStore_t* message_store_rehabilitation_aim;

extern MPAI_AIM_MessageStore_t* message_store_mycompanalysis_aim;
extern MPAI_AIM_MessageStore_t* message_store_mycomm_aim;

/* AIW global channels used by message store */
extern subscriber_channel_t SENSORS_DATA_CHANNEL;
extern subscriber_channel_t MIC_BUFFER_DATA_CHANNEL;
extern subscriber_channel_t MIC_PEAK_DATA_CHANNEL;
extern subscriber_channel_t MOTION_DATA_CHANNEL;

extern subscriber_channel_t MYCOMP_DATA_CHANNEL;


/**
 * @brief Initialize AIW Test Case (IOT-REV)
 * 
 */
int MPAI_AIW_IOT_REV_Init();

/**
 * @brief Stop AIW Test Case (IOT-REV)
 * 
 */
void MPAI_AIW_IOT_REV_Stop();

/**
 * @brief Resume AIW Test Case (IOT-REV)
 */
void MPAI_AIW_IOT_REV_Resume();

/**
 * @brief Pause AIW Test Case (IOT-REV)
 * 
 */
void MPAI_AIW_IOT_REV_Pause();

/**MPAI_LIBS_IOT_REV_AIM_REHABILITATION_NAME 
 * @brief Destroy AIW Test Case (IOT-REV)
 * 
 */
void MPAI_AIW_IOT_REV_Destroy();
#endif