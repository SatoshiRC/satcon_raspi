// Raspberry Pi: IMU + GPIO interrupt example

#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <array>
#include <memory>

#include "ICM20948/ICM20948_raspi.h"
#include "ICM20948_USER.h"
#include "GPIOInterrupt.h"

static std::atomic<bool> g_stop{false};
void onSignal(int){ g_stop = true; }

int measure(std::shared_ptr<GPIOInterrupt> irq);

int main(){
	// Configure INT pin connection
	const char* gpiochip = "gpiochip0"; // Raspberry Pi default
	const int INT_GPIO_BCM = 4;         // BCM GPIO number connected to ICM-20948 INT pin

	// Create IMU instance
	ICM20948_raspi *imu = new ICM20948_raspi("/dev/i2c-1", ICM20948::Address::LOW);
	ICM20948_USER icm20948User(imu);

	// Initialize I2C communication
	if (!imu->begin()) {
		std::cerr << "Failed to initialize ICM20948 (I2C)" << std::endl;
		return 1;
	}

    try{
		icm20948User.confirmConnection();
	}catch(std::runtime_error &e){
        std::cerr << "Error : Icm20948 is not detected" << std::endl;
	}
    icm20948User.init();

	// Set up GPIO interrupt on the INT pin
	GPIOInterrupt irq(gpiochip, INT_GPIO_BCM, GPIOInterrupt::Edge::Rising);
	irq.setCallback([&](bool rising){
		if (!rising) return; // handle only rising edge
		// Read IMU data-ready status and then fetch data
		imu->readIMU();
		std::array<float,3> accel{}, gyro{};
		imu->getAccel(accel);
		imu->getGyro(gyro);
		std::cout << "INT: Accel[g]=" << accel[0] << "," << accel[1] << "," << accel[2]
				  << " Gyro[rad/s]=" << gyro[0] << "," << gyro[1] << "," << gyro[2] << std::endl;
	});
	std::shared_ptr<GPIOInterrupt> pIrq = std::make_shared<GPIOInterrupt>(irq);

	// Handle Ctrl+C to exit
	std::signal(SIGINT, onSignal);
	std::cout << "Waiting for interrupts on GPIO" << INT_GPIO_BCM << " (Ctrl+C to exit)..." << std::endl;

	std::thread measureThread(measure, pIrq);
	measureThread.detach();

	while (!g_stop.load()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

	irq.stop();
	imu->end();
	return 0;
}

int measure(std::shared_ptr<GPIOInterrupt> irq){
	std::cout << "Prease press \"start\" " << std::endl;
	std::string str;
	std::cin >> str;
	if(str == "start"){
		irq->start();
		usleep(18*1000*1000);
	}
}