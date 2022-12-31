/*
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MPAI_LIBS_MYCOMP_COMMON_H
#define MPAI_LIBS_MYCOMP_COMMON_H

#include <core_common.h>

typedef enum
{ 
    MYCOMP_UNKNOWN,
	MYCOMP_STOPPED,
	MYCOMP_STARTED
}MYCOMP_TYPE;

typedef struct _mycomp_data_t{
	MYCOMP_TYPE mycomp_type;
	float mycomp_accel_total;

}mycomp_data_t;

#endif