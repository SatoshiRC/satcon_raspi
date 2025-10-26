/*
 * ICM20948_USER.h
 *
 *  Created on: Feb 10, 2023
 *      Author: OHYA Satoshi
 */

#ifndef ICM20948_ICM20948_USER_H_
#define ICM20948_ICM20948_USER_H_

#include "ICM20948/ICM20948.h"
#include "Vector3D/Vector3D.h"
#include "stdexcept"
#include <array>
#include <unistd.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>


class ICM20948_USER{
public:
	ICM20948_USER(ICM20948 *icm20948):icm20948(icm20948){
		__isCalibrated = false;
	}

	void confirmConnection();
	void init();

	uint16_t calibration(Vector3D<float> &gyro);
	bool isCalibrated(){
		return __isCalibrated;
	}

	void getIMU(Vector3D<int16_t> &accel, Vector3D<int16_t> &gyro);

	void update(){};

private:
	ICM20948 *icm20948;
	bool __isCalibrated;
	Vector3D<double> accelAverage={-0.04,-0.05,-0.0633};
	Vector3D<double> gyroAverage={};
	uint16_t averageCounter;
};

#endif /* ICM20948_ICM20948_USER_H_ */
