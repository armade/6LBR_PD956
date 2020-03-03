/*
 * Copyright � 2019, Peter Mikkelsen
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include "contiki-conf.h"
#include "lib/sensors.h"
#include "bmp-280-sensor.h"
#include "sys/ctimer.h"
#include "lib/sensors.h"
#include "i2csoft.h"
#include "board-peripherals.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#ifdef NODE_BMP280
/*---------------------------------------------------------------------------*/
#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
/*---------------------------------------------------------------------------*/
#define BMP280_I2C_ADDRESS                  0x76
/*---------------------------------------------------------------------------*/
/* Registers */
#define ADDR_CALIB                          0x88
#define ADDR_PROD_ID                        0xD0
#define ADDR_RESET                          0xE0
#define ADDR_STATUS                         0xF3
#define ADDR_CTRL_MEAS                      0xF4
#define ADDR_CONFIG                         0xF5
#define ADDR_PRESS_MSB                      0xF7
#define ADDR_PRESS_LSB                      0xF8
#define ADDR_PRESS_XLSB                     0xF9
#define ADDR_TEMP_MSB                       0xFA
#define ADDR_TEMP_LSB                       0xFB
#define ADDR_TEMP_XLSB                      0xFC
/*---------------------------------------------------------------------------*/
/* Reset values */
#define VAL_PROD_ID                         0x58
#define VAL_RESET                           0x00
#define VAL_STATUS                          0x00
#define VAL_CTRL_MEAS                       0x00
#define VAL_CONFIG                          0x00
#define VAL_PRESS_MSB                       0x80
#define VAL_PRESS_LSB                       0x00
#define VAL_TEMP_MSB                        0x80
#define VAL_TEMP_LSB                        0x00
/*---------------------------------------------------------------------------*/
/* Test values */
#define VAL_RESET_EXECUTE                   0xB6
#define VAL_CTRL_MEAS_TEST                  0x55
/*---------------------------------------------------------------------------*/
/* Misc. */
#define MEAS_DATA_SIZE                      6
/*---------------------------------------------------------------------------*/
#define RES_OFF                             0
#define RES_ULTRA_LOW_POWER                 1
#define RES_LOW_POWER                       2
#define RES_STANDARD                        3
#define RES_HIGH                            5
#define RES_ULTRA_HIGH                      6
/*---------------------------------------------------------------------------*/
/* Bit fields in CTRL_MEAS register */
#define PM_OFF                              0
#define PM_FORCED                           1
#define PM_NORMAL                           3
/*---------------------------------------------------------------------------*/
#define OSRST(v)                            ((v) << 5)
#define OSRSP(v)                            ((v) << 2)
/*---------------------------------------------------------------------------*/
typedef struct {
	uint16_t 	dig_t1;
	int16_t 	dig_t2;
	int16_t 	dig_t3;
	uint16_t 	dig_p1;
	int16_t 	dig_p2;
	int16_t 	dig_p3;
	int16_t 	dig_p4;
	int16_t 	dig_p5;
	int16_t 	dig_p6;
	int16_t 	dig_p7;
	int16_t 	dig_p8;
	int16_t 	dig_p9;
	int32_t 	t_fine;
} bmp_280_calibration_t;
/*---------------------------------------------------------------------------*/
#define CALIB_DATA_SIZE (sizeof(bmp_280_calibration_t))
static uint8_t calibration_data[CALIB_DATA_SIZE];
/*---------------------------------------------------------------------------*/
static int sensor_status = SENSOR_STATUS_DISABLED;
/*---------------------------------------------------------------------------*/
/* A buffer for the raw reading from the sensor */
#define SENSOR_DATA_BUF_SIZE   MEAS_DATA_SIZE

static uint8_t sensor_value[SENSOR_DATA_BUF_SIZE];
/*---------------------------------------------------------------------------*/
/* Wait SENSOR_STARTUP_DELAY clock ticks for the sensor to be ready - ~80ms */
#define SENSOR_STARTUP_DELAY (80 * 1000) / CLOCK_SECOND

static struct ctimer startup_timer;



static int32_t temp = 0;
static uint32_t pres = 0;

static void bmp280_convert(uint8_t *data, int32_t *temp, uint32_t *press);
static bool bmp280_read_data(uint8_t *data);
/*---------------------------------------------------------------------------*/
static inline
uint16_t SoftI2Cread_register(uint8_t Addr, uint8_t reg, uint8_t *Data, uint8_t len)
{
	uint8_t i;
    SoftI2CStart();
	SoftI2CWriteByte((Addr<<1)&0xFE); //clr LSB for write
	SoftI2CWriteByte(reg);

	SoftI2CStart();
	SoftI2CWriteByte((Addr<<1)|1); //set LSB for read

	for(i=0;i<(len-1);i++)
		*Data++=SoftI2CReadByte(1);

	*Data++=SoftI2CReadByte(0);

	SoftI2CStop();

	return 1;
}
static inline
uint8_t SoftI2Cread_char_register(uint8_t Addr, uint8_t reg)
{
	uint8_t lsb;

    SoftI2CStart();
	SoftI2CWriteByte((Addr<<1)&0xFE); //clr LSB for write
	SoftI2CWriteByte(reg);

	SoftI2CStart();
	SoftI2CWriteByte((Addr<<1)|1); //set LSB for read

	lsb=SoftI2CReadByte(0);

	SoftI2CStop();

	return lsb;
}
static
void SoftI2Cwrite_char_register(uint8_t Addr, uint8_t reg, uint8_t data)
{
    SoftI2CStart();
	SoftI2CWriteByte((Addr<<1)&0xFE); //clr LSB for write
	SoftI2CWriteByte(reg);
	SoftI2CWriteByte(data);

	SoftI2CStop();

	return;
}
/*---------------------------------------------------------------------------*/

static void
notify_ready(void *not_used)
{
	int rv;
	memset(sensor_value, 0, SENSOR_DATA_BUF_SIZE);

	rv = bmp280_read_data(sensor_value);

	if(rv == 0) {
		temp = SENSOR_ERROR;
		pres = SENSOR_ERROR;

		sensors_changed(&bmp_280_sensor);
	}

	PRINTF("val: %02x%02x%02x %02x%02x%02x\n",
		   sensor_value[0], sensor_value[1], sensor_value[2],
		   sensor_value[3], sensor_value[4], sensor_value[5]);

	bmp280_convert(sensor_value, &temp, &pres);
	sensor_status = SENSOR_STATUS_READY;
	sensors_changed(&bmp_280_sensor);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Initalise the sensor
 */
static void
bmp280_init(void)
{
	uint8_t val;

	/* Read and store calibration data */
	SoftI2Cread_register(BMP280_I2C_ADDRESS, ADDR_CALIB, calibration_data, CALIB_DATA_SIZE);

	/* Reset the sensor */
	val = VAL_RESET_EXECUTE;
	SoftI2Cwrite_char_register(BMP280_I2C_ADDRESS, ADDR_RESET, val);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Enable/disable measurements
 * \param enable 0: disable, enable otherwise
 *
 * @return      none
 */
static void
bmp280_enable_sensor(bool enable)
{
	uint8_t val;

	if(enable) {
		/* Enable forced mode */
		val = PM_FORCED | OSRSP(1) | OSRST(1);
	} else {
		val = PM_OFF;
	}
	//sensor_common_write_reg(ADDR_CTRL_MEAS, &val, sizeof(val));
	SoftI2Cwrite_char_register(BMP280_I2C_ADDRESS, ADDR_CTRL_MEAS, val);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Read temperature and pressure data
 * \param data Pointer to a buffer where temperature and pressure will be
 *             written (6 bytes)
 * \return True if valid data could be retrieved
 */
static bool
bmp280_read_data(uint8_t *data)
{
	SoftI2Cread_register(BMP280_I2C_ADDRESS, ADDR_PRESS_MSB, data, MEAS_DATA_SIZE);

	return 1;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Convert raw data to values in degrees C (temp) and Pascal (pressure)
 * \param data Pointer to a buffer that holds raw sensor data
 * \param temp Pointer to a variable where the converted temperature will be
 *             written
 * \param press Pointer to a variable where the converted pressure will be
 *              written
 */
static void
bmp280_convert(uint8_t *data, int32_t *temp, uint32_t *press)
{
	int32_t utemp, upress;
	bmp_280_calibration_t *p = (bmp_280_calibration_t *)calibration_data;
	int32_t v_x1_u32r;
	int32_t v_x2_u32r;
	int32_t temperature;
	uint32_t pressure;

	/* Pressure */
	upress = (int32_t)((((uint32_t)(data[0])) << 12)
					 | (((uint32_t)(data[1])) << 4) | ((uint32_t)data[2] >> 4));

	/* Temperature */
	utemp = (int32_t)((((uint32_t)(data[3])) << 12) | (((uint32_t)(data[4])) << 4)
					| ((uint32_t)data[5] >> 4));

	if(utemp != 0x80000){
		/* Compensate temperature */
		v_x1_u32r = ((((utemp >> 3) - ((int32_t)p->dig_t1 << 1)))
				   * ((int32_t)p->dig_t2)) >> 11;
		v_x2_u32r = (((((utemp >> 4) - ((int32_t)p->dig_t1))
				   * ((utemp >> 4) - ((int32_t)p->dig_t1))) >> 12)
				   * ((int32_t)p->dig_t3)) >> 14;

		p->t_fine = v_x1_u32r + v_x2_u32r;
		temperature = (p->t_fine * 5 + 128) >> 8;
		*temp = temperature;
	}
	else
	{
		*temp = SENSOR_ERROR;
	}

	if(upress != 0x80000){
		/* Compensate pressure */
		v_x1_u32r = (((int32_t)p->t_fine) >> 1) - (int32_t)64000;
		v_x2_u32r = (((v_x1_u32r >> 2) * (v_x1_u32r >> 2)) >> 11)
		* ((int32_t)p->dig_p6);
		v_x2_u32r = v_x2_u32r + ((v_x1_u32r * ((int32_t)p->dig_p5)) << 1);
		v_x2_u32r = (v_x2_u32r >> 2) + (((int32_t)p->dig_p4) << 16);
		v_x1_u32r =
		(((p->dig_p3 * (((v_x1_u32r >> 2) * (v_x1_u32r >> 2)) >> 13)) >> 3)
		 + ((((int32_t)p->dig_p2) * v_x1_u32r) >> 1)) >> 18;
		v_x1_u32r = ((((32768 + v_x1_u32r)) * ((int32_t)p->dig_p1)) >> 15);

		if(v_x1_u32r == 0) {
			return; /* Avoid exception caused by division by zero */
		}

		pressure = (((uint32_t)(((int32_t)1048576) - upress) - (v_x2_u32r >> 12)))
		* 3125;
		if(pressure < 0x80000000) {
			pressure = (pressure << 1) / ((uint32_t)v_x1_u32r);
		} else {
			pressure = (pressure / (uint32_t)v_x1_u32r) * 2;
		}

		v_x1_u32r = (((int32_t)p->dig_p9)
				   * ((int32_t)(((pressure >> 3) * (pressure >> 3)) >> 13))) >> 12;
		v_x2_u32r = (((int32_t)(pressure >> 2)) * ((int32_t)p->dig_p8)) >> 13;
		pressure = (uint32_t)((int32_t)pressure
							+ ((v_x1_u32r + v_x2_u32r + p->dig_p7) >> 4));

		*press = pressure;
	}
    else
    {
    	*temp = SENSOR_ERROR;
    }
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Returns a reading from the sensor
 * \param type BMP_280_SENSOR_TYPE_TEMP or BMP_280_SENSOR_TYPE_PRESS
 * \return Temperature (centi degrees C) or Pressure (Pascal).
 */
static int
bmp280_value(int type)
{
	int rv;

	if(sensor_status != SENSOR_STATUS_READY) {
		PRINTF("Sensor disabled or starting up (%d)\n", sensor_status);
		return SENSOR_ERROR;
	}

	if((type != BMP_280_SENSOR_TYPE_TEMP) && type != BMP_280_SENSOR_TYPE_PRESS) {
		PRINTF("Invalid type\n");
		return SENSOR_ERROR;
	} else {

		if(type == BMP_280_SENSOR_TYPE_TEMP) {
			rv = (int)temp;
		} else if(type == BMP_280_SENSOR_TYPE_PRESS) {
			rv = (int)pres;
		}
	}
	return rv;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Configuration function for the BMP280 sensor.
 *
 * \param type Activate, enable or disable the sensor. See below
 * \param enable
 *
 * When type == SENSORS_HW_INIT we turn on the hardware
 * When type == SENSORS_ACTIVE and enable==1 we enable the sensor
 * When type == SENSORS_ACTIVE and enable==0 we disable the sensor
 */
static int
bmp280_configure(int type, int enable)
{
	uint8_t ID;
	switch(type) {
		case SENSORS_HW_INIT:
			SoftI2CInit();

			bmp280_init();
			ID = SoftI2Cread_char_register(BMP280_I2C_ADDRESS, ADDR_PROD_ID);
			if(ID != 0x58)
				sensor_status = SENSOR_STATUS_DISABLED;
			else
				bmp280_enable_sensor(0);
			sensor_status = SENSOR_STATUS_INITIALISED;
			break;
		case SENSORS_ACTIVE:
			/* Must be initialised first */
			if(sensor_status == SENSOR_STATUS_DISABLED)
			  return SENSOR_STATUS_DISABLED;

			if(enable) {
			  bmp280_enable_sensor(1);
			  ctimer_set(&startup_timer, SENSOR_STARTUP_DELAY, notify_ready, NULL);
			  sensor_status = SENSOR_STATUS_NOT_READY;
			} else {
			  ctimer_stop(&startup_timer);
			  bmp280_enable_sensor(0);
			  sensor_status = SENSOR_STATUS_INITIALISED;
			}
			break;
		default:
			break;
	}

  return sensor_status;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Returns the status of the sensor
 * \param type SENSORS_ACTIVE or SENSORS_READY
 * \return 1 if the sensor is enabled
 */
static int
bmp280_status(int type)
{
	/*switch(type) {
		case SENSORS_ACTIVE:
		case SENSORS_READY:
			return enabled;
			break;
		default:
			break;
	}
	return SENSOR_STATUS_DISABLED;*/
	return sensor_status;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(bmp_280_sensor, "BMP280", bmp280_value, bmp280_configure, bmp280_status);
/*---------------------------------------------------------------------------*/
/** @} */
#endif
