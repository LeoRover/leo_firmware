#ifndef LEO_FIRMWARE_INCLUDE_SENSORS_IMU_H_
#define LEO_FIRMWARE_INCLUDE_SENSORS_IMU_H_

#include <II2C.h>
#include <hMutex.h>

#include <MPU9250/MPU9250.h>

class IMU {
 public:
  explicit IMU(hFramework::II2C &i2c) : mpu_(i2c) {}

  void init();
  void update();
  void calGyroAccel();
  void calMag();

  float ax, ay, az;      // accelerometer data
  float gx, gy, gz;      // gyroscope data
  float mx, my, mz;      // magnetometer data
  float qx, qy, qz, qw;  // quaternion data

 private:
  MPU9250 mpu_;
  float ares_, gres_, mres_;
  float abias_[3], gbias_[3], mscale_[3], mbias_[3];
  float magCal_[3];
  hFramework::hMutex mpu_mutex_;
};

#endif  // LEO_FIRMWARE_INCLUDE_SENSORS_IMU_H_
