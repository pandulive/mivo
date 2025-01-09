#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>

// Function to calculate the color temperature of an image
void calculateColorTemperature(const std::string& imagePath) {
    // Load the image
    cv::Mat image = cv::imread(imagePath);
    if (image.empty()) {
        std::cerr << "Failed to load image." << std::endl;
        return;
    }

    // Convert to LAB color space
    cv::Mat labImage;
    cv::cvtColor(image, labImage, cv::COLOR_BGR2Lab);

    // Split LAB image into channels
    std::vector<cv::Mat> labChannels(3);
    cv::split(labImage, labChannels);
    cv::Mat l = labChannels[0];
    cv::Mat a = labChannels[1];
    cv::Mat b = labChannels[2];

    // Calculate the average 'a' and 'b' channel values
    double avgA = cv::mean(a)[0];
    double avgB = cv::mean(b)[0];

    // Approximate color temperature using a heuristic formula
    double colorTemperature = 5000 + (avgA - avgB) * 100;
    colorTemperature = std::max(1000.0, std::min(colorTemperature, 10000.0)); // Clamp between 1000K and 10000K

    std::cout << "Estimated Color Temperature: " << static_cast<int>(colorTemperature) << "K" << std::endl;
}

int main() {
    std::string imagePath = "captured_frame.jpg"; // Path to the image
    calculateColorTemperature(imagePath);
    return 0;
}
