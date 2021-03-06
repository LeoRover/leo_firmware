#ifndef LEO_FIRMWARE_INCLUDE_PARAMETERS_H_
#define LEO_FIRMWARE_INCLUDE_PARAMETERS_H_

#include <ros.h>

struct Parameters {
  // TF frames
  char robot_frame_id[50] = "base_link";
  char odom_frame_id[50] = "odom";
  char imu_frame_id[50] = "imu";
  char gps_frame_id[50] = "gpu";

  // Servo
  int servo_voltage = 2;
  int servo_period[6] = {20000, 20000, 20000, 20000, 20000, 20000};
  int servo_angle_min[6] = {-90, -90, -90, -90, -90, -90};
  int servo_angle_max[6] = {90, 90, 90, 90, 90, 90};
  int servo_width_min[6] = {1000, 1000, 1000, 1000, 1000, 1000};
  int servo_width_max[6] = {2000, 2000, 2000, 2000, 2000, 2000};

  // Motor
  float motor_encoder_resolution = 878.4F;
  int motor_encoder_pullup = 1;
  float motor_max_speed = 800.0F;
  float motor_pid_p = 0.0F;
  float motor_pid_i = 0.005F;
  float motor_pid_d = 0.0F;
  float motor_power_limit = 1000.0F;
  float motor_torque_limit = 1000.0F;

  // Differential drive
  float dd_wheel_radius = 0.0625F;
  float dd_wheel_separation = 0.33F;
  float dd_angular_velocity_multiplier = 1.91F;
  float dd_input_timeout = 500.0F;

  void load(ros::NodeHandle &nh);
};

extern Parameters params;

#endif