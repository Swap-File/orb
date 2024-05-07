#ifndef _IMU_H
#define _IMU_H

void imu_init(void);
int imu_update(void);
void enter_sleep(bool blink);

#endif