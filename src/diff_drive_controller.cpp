#include <cmath>
#include <cstdio>

#include <hSystem.h>
#include <peripherals.h>

#include <leo_firmware/diff_drive_controller.h>
#include <leo_firmware/logging.h>
#include <leo_firmware/parameters.h>
#include <leo_firmware/utils.h>

using hFramework::sys;

static constexpr float PI = 3.141592653F;

void DiffDriveController::init() {
  wheel_FL_ = new WheelController(hFramework::hMotC, true);
  wheel_RL_ = new WheelController(hFramework::hMotD, true);
  wheel_FR_ = new WheelController(hFramework::hMotA, false);
  wheel_RR_ = new WheelController(hFramework::hMotB, false);

  sys.taskCreate(std::bind(&DiffDriveController::controllerLoop, this));
  if (params.dd_input_timeout > 0.0) {
    last_update_ = sys.getRefTime();
    sys.taskCreate(std::bind(&DiffDriveController::inputWatchdog, this));
  }
}

void DiffDriveController::setSpeed(const float linear, const float angular) {
  const float angular_multiplied =
      angular * params.dd_angular_velocity_multiplier;
  const float wheel_L_lin_vel =
      linear - (angular_multiplied * params.dd_wheel_separation / 2.0F);
  const float wheel_R_lin_vel =
      linear + (angular_multiplied * params.dd_wheel_separation / 2.0F);
  const float wheel_L_ang_vel = wheel_L_lin_vel / params.dd_wheel_radius;
  const float wheel_R_ang_vel = wheel_R_lin_vel / params.dd_wheel_radius;
  const float enc_L_speed =
      params.motor_encoder_resolution * wheel_L_ang_vel / (2.0F * PI);
  const float enc_R_speed =
      params.motor_encoder_resolution * wheel_R_ang_vel / (2.0F * PI);

  wheel_FL_->setSpeed(enc_L_speed);
  wheel_RL_->setSpeed(enc_L_speed);
  wheel_FR_->setSpeed(enc_R_speed);
  wheel_RR_->setSpeed(enc_R_speed);

  if (params.dd_input_timeout > 0.0F) last_update_ = sys.getRefTime();
}

Odom DiffDriveController::getOdom() {
  hFramework::hMutexGuard m(mutex_odom_);
  return odom_;
}

void DiffDriveController::resetOdom() {
  hFramework::hMutexGuard m(mutex_odom_);
  odom_.pose_x = 0.0F;
  odom_.pose_y = 0.0F;
  odom_.pose_yaw = 0.0F;
}

void DiffDriveController::updateWheelStates() {
  hFramework::hMutexGuard m(mutex_wheel_);
  positions[0] =
      2.0F * PI * wheel_FL_->getDistance() / params.motor_encoder_resolution;
  positions[1] =
      2.0F * PI * wheel_RL_->getDistance() / params.motor_encoder_resolution;
  positions[2] =
      2.0F * PI * wheel_FR_->getDistance() / params.motor_encoder_resolution;
  positions[3] =
      2.0F * PI * wheel_RR_->getDistance() / params.motor_encoder_resolution;

  velocities[0] =
      2.0F * PI * wheel_FL_->getSpeed() / params.motor_encoder_resolution;
  velocities[1] =
      2.0F * PI * wheel_RL_->getSpeed() / params.motor_encoder_resolution;
  velocities[2] =
      2.0F * PI * wheel_FR_->getSpeed() / params.motor_encoder_resolution;
  velocities[3] =
      2.0F * PI * wheel_RR_->getSpeed() / params.motor_encoder_resolution;

  efforts[0] =
      wheel_FL_->getPower() == 0.0F ? 0.0F : wheel_FL_->getPower() * -0.1F;
  efforts[1] =
      wheel_RL_->getPower() == 0.0F ? 0.0F : wheel_RL_->getPower() * -0.1F;
  efforts[2] =
      wheel_FR_->getPower() == 0.0F ? 0.0F : wheel_FR_->getPower() * -0.1F;
  efforts[3] =
      wheel_RR_->getPower() == 0.0F ? 0.0F : wheel_RR_->getPower() * -0.1F;
}

void DiffDriveController::controllerLoop() {
  uint32_t t = sys.getRefTime();
  const uint32_t dt = 10;
  while (true) {
    mutex_wheel_.lock();
    {
      wheel_FL_->update(dt);
      wheel_RL_->update(dt);
      wheel_FR_->update(dt);
      wheel_RR_->update(dt);
    }
    mutex_wheel_.unlock();

    // speed in ticks/sec
    const float FL_speed = wheel_FL_->getSpeed();
    const float RL_speed = wheel_RL_->getSpeed();
    const float FR_speed = wheel_FR_->getSpeed();
    const float RR_speed = wheel_RR_->getSpeed();

    const float L_speed = (FL_speed + RL_speed) / 2.0F;
    const float R_speed = (FR_speed + RR_speed) / 2.0F;

    // velocity in radians per second
    const float L_ang_vel =
        2.0F * PI * L_speed / params.motor_encoder_resolution;
    const float R_ang_vel =
        2.0F * PI * R_speed / params.motor_encoder_resolution;

    // velocity in meters per second
    const float L_lin_vel = L_ang_vel * params.dd_wheel_radius;
    const float R_lin_vel = R_ang_vel * params.dd_wheel_radius;

    mutex_odom_.lock();
    {
      // linear (m/s) and angular (r/s) velocities of the robot
      odom_.vel_lin = (L_lin_vel + R_lin_vel) / 2.0F;
      odom_.vel_ang = (R_lin_vel - L_lin_vel) / params.dd_wheel_separation;

      odom_.vel_ang /= params.dd_angular_velocity_multiplier;

      // Integrate the velocity using the rectangle rule
      odom_.pose_yaw += odom_.vel_ang * 0.01F;
      if (odom_.pose_yaw > 2.0F * PI)
        odom_.pose_yaw -= 2.0F * PI;
      else if (odom_.pose_yaw < 0.0F)
        odom_.pose_yaw += 2.0F * PI;

      odom_.pose_x += odom_.vel_lin * std::cos(odom_.pose_yaw) * 0.01F;
      odom_.pose_y += odom_.vel_lin * std::sin(odom_.pose_yaw) * 0.01F;
    }
    mutex_odom_.unlock();

    sys.delaySync(t, dt);
  }
}

void DiffDriveController::inputWatchdog() {
  while (true) {
    while (sys.getRefTime() < last_update_ + params.dd_input_timeout)
      sys.delay(last_update_ + params.dd_input_timeout - sys.getRefTime() + 1);

    setSpeed(0.0F, 0.0F);
  }
}
