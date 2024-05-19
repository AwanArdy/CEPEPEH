#include <qt5/QtWidgets/QApplication>
#include <qt5/QtWidgets/QPushButton>
#include <qt5/QtWidgets/QVBoxLayout>
#include <qt5/QtWidgets/QWidget>
#include <qt5/QtWidgets/QComboBox>
#include <qt5/QtWidgets/QLabel>
#include <opencv4/opencv2/opencv.hpp>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <string>
#include <sstream>
#include <qt5/QtCore/QTimer>
#include <qt5/QtGui/QImage>
#include <qt5/QtGui/QPixmap>

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

std::string getCameraName(const std::string& devicePath) {
    std::string cmd = "udevadm info --query=all --name=" + devicePath + " | grep ID_MODEL=";
    std::vector<std::string> output = executeCommand(cmd.c_str());
    if (!output.empty()) {
        // Mengambil nama model
        std::string modelName = output[0];
        // Menghapus 'ID_MODEL' dari awal string
        modelName.erase(0, modelName.find('=') + 1);
        // Menghapus karakter newline dari akhir string
        modelName.erase(modelName.find_last_not_of("\n") + 1);
        return modelName;
    }
    return "unknown device";
}

class CameraApp : public QWidget {
    Q_OBJECT

public:
    CameraApp(QWidget *parent = 0);

private slots:
    void startCamera(int cameraIndex);
    void updateFrame();

private:
    QComboBox *cameraSelector;
    QLabel *cameraView;
    cv::VideoCapture cap;
    QTimer *timer;
    cv::CascadeClassifier faceCascade;
};

CameraApp::CameraApp(QWidget *parent)
    : QWidget(parent), timer(new QTimer(this)) {
    QVBoxLayout *layout = new QVBoxLayout(this);

    cameraSelector = new QComboBox(this);
    std::vector<std::string> devices = executeCommand("v4l2-ctl --list-devices");

    int deviceIndex = 0;
    std::string currentDevice;
    for (const auto& line : devices) {
        if (line.find("/dev/video") != std::string::npos) {
            std::istringstream iss(line);
            iss >> currentDevice;
            std::string cameraName = getCameraName(currentDevice);
            cameraSelector->addItem(QString::fromStdString(cameraName + " (" + currentDevice + ")"), QVariant::fromValue(deviceIndex));
            deviceIndex++;
        }
    }

    QPushButton *startButton = new QPushButton("Start Camera", this);
    connect(startButton, &QPushButton::clicked, this, [this]() {
        startCamera(cameraSelector->currentIndex());
    });

    cameraView = new QLabel(this);

    layout->addWidget(cameraSelector);
    layout->addWidget(startButton);
    layout->addWidget(cameraView);

    setLayout(layout);

    // Load Haar cascade for face detection
    // Change the path to match the location of your haarcascade_frontalface_default.xml file
    std::string faceCascadePath = "/home/awanardy/Documents/Projek/CEPEPEH/haarcascade_frontalface_default.xml";
    if (!faceCascade.load(faceCascadePath)) {
        std::cerr << "Error loading " << faceCascadePath << std::endl;
        exit(1);
    }

    // Start the internal camera by default (index 0)
    startCamera(1);
}

void CameraApp::startCamera(int cameraIndex) {
    cap.open(cameraIndex, cv::CAP_V4L2);

    if (!cap.isOpened()) {
        std::cerr << "Error: Tidak bisa membuka kamera dengan indeks " << cameraIndex << std::endl;
        return;
    }

    connect(timer, &QTimer::timeout, this, &CameraApp::updateFrame);
    timer->start(30);
}

void CameraApp::updateFrame() {
    cv::Mat frame;
    cap >> frame;
    if (frame.empty()) return;

    std::vector<cv::Rect> faces;
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    cv::equalizeHist(gray, gray);

    faceCascade.detectMultiScale(gray, faces);

    for (size_t i = 0; i < faces.size(); i++) {
        cv::rectangle(frame, faces[i], cv::Scalar(0, 255, 0), 2);
    }

    cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
    QImage image(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);
    cameraView->setPixmap(QPixmap::fromImage(image));
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    CameraApp window;
    window.resize(800, 600);
    window.show();
    return app.exec();
}

#include "camera_app.moc"