#include <opencv4/opencv2/opencv.hpp>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <string>
#include <sstream>

std::vector<std::string> executeCommand(const char* cmd) {
    std::vector<std::string> result;
    char buffer[128];
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result.push_back(buffer);
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}

int main() {
    std::cout << "Daftar kamera yang tersedia:" << std::endl;
    std::vector<std::string> devices = executeCommand("v4l2-ctl --list-devices");

    for (const auto& line : devices) {
        std::cout << line;
    }

    int cameraIndex;
    std::cout << "\nMasukkan indeks kamera (misal: 0 untuk kamera default, 1 untuk webcam eksternal, dsb.)";
    std::cin >> cameraIndex;

    cv::VideoCapture cap(cameraIndex);

    if (!cap.isOpened()) {
        std::cerr << "Error: Tidak bisa membuka kamera" << std::endl;
        return -1;
    }

    cv::Mat frame;
    while (true) {
        cap >> frame;
        if (frame.empty()) break;

        cv::imshow("Live Camera", frame);

        if (cv::waitKey(10) == 27) break;
    }

    cv::destroyAllWindows();
    return 0;
}