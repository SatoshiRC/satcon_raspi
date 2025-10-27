// Raspberry Pi: IMU + GPIO interrupt example

#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <array>
#include <memory>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include "ICM20948/ICM20948_raspi.h"
#include "ICM20948_USER.h"
#include "GPIOInterrupt.h"
#include "parser.hpp"

static std::atomic<bool> g_stop{false};
void onSignal(int){ g_stop = true; }

int measure(std::shared_ptr<GPIOInterrupt> irq, std::shared_ptr<std::ofstream> imuOfs, int uart_fd);

int intCounter = 0;

int main(){
	std::shared_ptr imuOfs = std::make_shared<std::ofstream>();
	
	// Configure INT pin connection
	const char* gpiochip = "gpiochip0"; // Raspberry Pi default
	const int INT_GPIO_BCM = 4;         // BCM GPIO number connected to ICM-20948 INT pin

	// Configure serial port
	std::string device = "/dev/ttyS0";
    int baud = 115200;
	int fd = open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        std::cerr << "Failed to open " << device << "\n";
        return 1;
    }
    struct termios tio;
    if (tcgetattr(fd, &tio) < 0) {
        std::cerr << "tcgetattr failed\n";
        close(fd);
        return 1;
    }
	cfmakeraw(&tio);
    cfsetispeed(&tio, B115200);
    cfsetospeed(&tio, B115200);
    tio.c_cflag |= CLOCAL | CREAD;
	tio.c_cc[VTIME] = 10;
    tcsetattr(fd, TCSANOW, &tio);

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
	std::shared_ptr<GPIOInterrupt> irq = std::make_shared<GPIOInterrupt>(gpiochip, INT_GPIO_BCM, GPIOInterrupt::Edge::Rising);
	// GPIOInterrupt irq(gpiochip, INT_GPIO_BCM, GPIOInterrupt::Edge::Rising);
	irq->setCallback([&](bool rising){
		intCounter++;
		if (!rising) return; // handle only rising edge
		// Read IMU data-ready status and then fetch data
		Vector3D<int16_t> accel{}, gyro{};
		icm20948User.getIMU(accel, gyro);
		// std::cout << "INT: Accel[g]=" << accel[0] << "," << accel[1] << "," << accel[2]
		// 		  << " Gyro[rad/s]=" << gyro[0] << "," << gyro[1] << "," << gyro[2] << std::endl;
		*imuOfs << accel[0] << "," << accel[1] << "," << accel[2]
				  << gyro[0] << "," << gyro[1] << "," << gyro[2] << std::endl;
		imuOfs->flush();
	});

	// Handle Ctrl+C to exit
	std::signal(SIGINT, onSignal);
	std::cout << "Waiting for interrupts on GPIO" << INT_GPIO_BCM << " (Ctrl+C to exit)..." << std::endl;

	std::thread measureThread(measure, irq, imuOfs, fd);
	measureThread.detach();

	while (!g_stop.load()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

	irq->stop();
	imu->end();
	close(fd);
	return 0;
}

int measure(std::shared_ptr<GPIOInterrupt> irq, std::shared_ptr<std::ofstream> imuOfs, int uart_fd){
	while(true){
		std::cout << "Prease press \"start\" " << std::endl;
		std::string str;
		std::cin >> str;

		// Configure log file
		std::filesystem::path dir = getenv("HOME");
		dir /= "satcon_log";
		auto now = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now());
		std::time_t t = std::chrono::system_clock::to_time_t(now);
		std::tm tm{};
		localtime_r(&t, &tm);
		std::ostringstream oss;
		oss << std::put_time(&tm, "%H_%M_%S");
		std::string nowStr = oss.str();
		dir /= nowStr;
		auto res = std::filesystem::create_directories(dir);
		if(res){
			std::cout << dir.c_str() << " has been created" << std::endl;
		}
		imuOfs->open(dir/"imu.csv");
		std::ofstream ofs(dir/"radioImu.csv");

		*imuOfs << "ax, ay, az, gx, gy, gz" << std::endl;
		ofs << "ax, ay, az, gx, gy, gz" << std::endl;


		if(str == "start"){
			intCounter = 0;
			std::cout << "start task" << std::endl;

			std::cout << uart_fd << std::endl;
			std::string message = "start"; // 送信する文字列
        	ssize_t bytes_written = write(uart_fd, message.c_str(), message.length());
			std::cout << "written bytes : " << bytes_written << std::endl;
			
			irq->start();
			usleep(18*1000*1000);
		}else{
			continue;
		}
		irq->stop();
		std::cout << "finish task, IMU interrupt : " << intCounter  << std::endl;

		std::string message = "transmit"; // 送信する文字列
		ssize_t bytes_written = write(uart_fd, message.c_str(), message.length());

		std::vector<uint8_t> readbuf(128);
		std::vector<uint8_t> buffer;
		while (true) {
			ssize_t n = read(uart_fd, readbuf.data(), readbuf.size());
			if (n > 0) {
				buffer.insert(buffer.end(), readbuf.begin(), readbuf.begin() + n);
				while (true) {
					auto f = satcon::find_and_parse(buffer);
					if (!f) break;
					auto fr = *f;
					ofs << fr.ax << "," << fr.ay << "," << fr.az << "," << fr.gx << "," << fr.gy << "," << fr.gz << "\n";
					ofs.flush();
				}
			} else {
				// no data
				break;
			}
		}

		imuOfs->close();
		ofs.close();
	}
	return 0;
}
