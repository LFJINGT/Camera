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
    void saveMetaData(const int CameraId);

private slots:
    void setCamera(const QCameraDevice &cameraDevice, const int CameraId); // 初始化摄像头

    void startCamera(const int CameraId); // 打开摄像头
    void stopCamera(const int CameraId); // 关闭摄像头

    void record(const int CameraId); // 记录
    void pause(const int CameraId); // 暂停
    void stop(const int CameraId); // 停止
    void setMuted(bool,const int CameraId); // 静音

    void takeImage(const int CameraId); // 捕获图像
    void displayCaptureError(int, QImageCapture::Error, const QString &errorString);

    void configureCaptureSettings(); // 相机设置
    void configureVideoSettings(); // 视频设置
    void configureImageSettings(); // 图像设置

    void displayRecorderError(const int CameraId);
    void displayCameraError(const int CameraId);

    void updateCameraDevice(QAction *action); // 更新相机设备

    void updateCameraActive(bool active);
    void updateCaptureMode();
    void updateRecorderState(QMediaRecorder::RecorderState state); // 更新记录状态
    void setExposureCompensation(int index,const int CameraId); // 设置曝光补偿

    void updateRecordTime(const int CameraId);

    void processCapturedImage(int requestId, const QImage &img);

    void displayViewfinder(); // 显示器
    void displayCapturedImage();

    void readyForCapture(bool ready);
    void imageSaved(int id, const QString &fileName);

    void updateCameras();

    void showMetaDataDialog();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::Camera *ui;

    QActionGroup *videoDevicesGroup  = nullptr;

    QMediaDevices m_devices[4];
    QMediaCaptureSession m_captureSession[4];
//    QMediaCaptureSession m_captureSession_2;
//    QMediaCaptureSession m_captureSession_3;
//    QMediaCaptureSession m_captureSession_4;
    // 相机
    QScopedPointer<QCamera> m_camera[4];
//    QScopedPointer<QCamera> m_camera_2;
//    QScopedPointer<QCamera> m_camera_3;
//    QScopedPointer<QCamera> m_camera_4;
    QScopedPointer<QAudioInput> m_audioInput;
    QImageCapture *m_imageCapture[4];
    // 媒体声音
    QScopedPointer<QMediaRecorder> m_mediaRecorder[4];
//    QScopedPointer<QMediaRecorder> m_mediaRecorder_2;
//    QScopedPointer<QMediaRecorder> m_mediaRecorder_3;
//    QScopedPointer<QMediaRecorder> m_mediaRecorder_4;


    bool m_isCapturingImage = false; // 是否正在捕获图像
    bool m_applicationExiting = false; // 是否退出
    bool m_doImageCapture = true; // 图像捕获功能开关

    MetaDataDialog *m_metaDataDialog = nullptr;
};

#endif
