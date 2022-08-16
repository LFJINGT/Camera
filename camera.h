#ifndef CAMERA_H
#define CAMERA_H

#include <QCamera>
#include <QImageCapture>
#include <QMediaRecorder>
#include <QScopedPointer>
#include <QMediaMetaData>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QAudioInput>

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class Camera; }
class QActionGroup;
QT_END_NAMESPACE

class MetaDataDialog;

class Camera : public QMainWindow
{
    Q_OBJECT

public:
    Camera();

public slots:
    void saveMetaData();

private slots:
    void setCamera(const QCameraDevice &cameraDevice, const int CameraId); // 初始化摄像头

    void startCamera(); // 打开摄像头
    void stopCamera(); // 关闭摄像头

//    void record(); // 记录
//    void pause(); // 暂停
//    void stop(); // 停止
//    void setMuted(bool); // 静音

//    void takeImage(); // 捕获图像
//    void displayCaptureError(int, QImageCapture::Error, const QString &errorString);

//    void configureCaptureSettings(); // 相机设置
//    void configureVideoSettings(); // 视频设置
//    void configureImageSettings(); // 图像设置
//
//    void displayRecorderError();
    void displayCameraError(const int CameraId); // 摄像头报错

    void updateCameraDevice(QAction *action); // 切换相机设备
    void updateCameraDevice_2(QAction *action);
    void updateCameraDevice_3(QAction *action);
    void updateCameraDevice_4(QAction *action);

    void updateCameraActive(bool active);
//    void updateCaptureMode();
//    void updateRecorderState(QMediaRecorder::RecorderState state); // 更新记录状态
//    void setExposureCompensation(int index); // 设置曝光补偿
//
//    void updateRecordTime();
//
//    void processCapturedImage(int requestId, const QImage &img);
//
//    void displayViewfinder(); // 显示器
//    void displayCapturedImage();
//
//    void readyForCapture(bool ready);
//    void imageSaved(int id, const QString &fileName);

    void updateCameras(const int CameraId);

    void showMetaDataDialog();

protected:
//    void keyPressEvent(QKeyEvent *event) override;
//    void keyReleaseEvent(QKeyEvent *event) override;
//    void closeEvent(QCloseEvent *event) override;

private:
    Ui::Camera *ui;

    QActionGroup *videoDevicesGroup[4]  = {nullptr};

    QMediaDevices m_devices;
    QMediaCaptureSession m_captureSession[4];
    // 相机
    QScopedPointer<QCamera> m_camera[4];
    QScopedPointer<QAudioInput> m_audioInput;
    QImageCapture *m_imageCapture;
    // 媒体声音
    QScopedPointer<QMediaRecorder> m_mediaRecorder;


    bool m_isCapturingImage = false; // 是否正在捕获图像
    bool m_applicationExiting = false; // 是否退出
    bool m_doImageCapture = true; // 图像捕获功能开关

    MetaDataDialog *m_metaDataDialog = nullptr;
};

#endif
