#ifndef KEYPAD_H_
#define KEYPAD_H_

#include<iostream>

#include <bitset>
#include <libftdi1/ftdi.h>
#include <unistd.h>
#include <thread>
#include <atomic>
#include <functional>

class FT232HHandler{
public:
    FT232HHandler();

    struct ftdi_context* ftdi;
    unsigned char prev_state;
    std::atomic<bool> running;
    std::thread gpio_thread;
    std::function<void(int)> callback;



    FT232HHandler(std::function<void(int)> button_callback);// ftdi(ftdi_new()), prev_state(0xFF), running(false), callback(button_callback);
    
    void initialize();

    void start();

    void stop();

    void reverse();

private:

    // void fullstop();

};

#endif // MAINWINDOW_H_