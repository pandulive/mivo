#include "KeyPad.h"

    FT232HHandler::FT232HHandler(std::function<void(int)> button_callback)
            : ftdi(ftdi_new()), prev_state(0xFF), running(false), callback(button_callback) {
            if (!ftdi) {
                throw std::runtime_error("Failed to create FTDI context.");
            }
        }

    void FT232HHandler::reverse() {    //Previously it was ~FT232HHandler()
            stop();
            if (ftdi) {
                ftdi_usb_close(ftdi);
                ftdi_free(ftdi);
            }
        }

    void FT232HHandler::initialize() {
        if (ftdi_usb_open(ftdi, 0x0403, 0x6014) < 0) {
            throw std::runtime_error("Unable to open FTDI device. Check connection.");
        }
        if (ftdi_set_bitmode(ftdi, 0x00, BITMODE_BITBANG) < 0) {
            throw std::runtime_error("Failed to set bit-bang mode.");
        }
    }

    void FT232HHandler::start() {
        running = true;
        gpio_thread = std::thread([this]() {
            while (running) {
                unsigned char gpio_state;
                if (ftdi_read_pins(ftdi, &gpio_state) < 0) {
                    std::cerr << "Failed to read GPIO state.\n";
                    break;
                }
                if (gpio_state != prev_state) {
                    // Detect specific button presses
                    if (!(gpio_state & 0x04)) callback(1); // Button 1 4
                    if (!(gpio_state & 0x08)) callback(2); // Button 2 8
                    if (!(gpio_state & 0x01)) callback(3); // Button 3 1
                    if (!(gpio_state & 0x02)) callback(4); // Button 4 2
                    prev_state = gpio_state;
                }
                usleep(250000);  // Poll every 250ms
            }
        });
    }

    void FT232HHandler::stop() {
        running = false;
        if (gpio_thread.joinable()) {
            gpio_thread.join();
        }
    }

