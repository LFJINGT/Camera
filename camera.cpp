#include "camera.h"
#include "ui_camera.h"
#include "videosettings.h"
#include "imagesettings.h"
#include "metadatadialog.h"

#include <QMediaRecorder>
#include <QVideoWidget>
#include <QCameraDevice>
#include <QMediaMetaData>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioInput>

#include <QMessageBox>
#include <QPalette>
#include <QImage>

#include <QtWidgets>
#include <QMediaDevices>
#include <QMediaFormat>

#include <QDebug>

#define DEV true

Camera::Camera()
        : ui(new Ui::Camera)
{
    ui->setupUi(this);

    //sample rate:
    m_audioInput.reset(new QAudioInput);
    m_captureSession[0].setAudioInput(m_audioInput.get());

    //Camera devices:

    videoDevicesGroup = new QActionGroup(this);
    videoDevicesGroup->setExclusive(true);
    updateCameras();

    connect(&m_devices[0], &QMediaDevices::videoInputsChanged, this, &Camera::updateCameras);

    connect(videoDevicesGroup, &QActionGroup::triggered, this, &Camera::updateCameraDevice);
    connect(ui->captureWidget, &QTabWidget::currentChanged, this, &Camera::updateCaptureMode);

    connect(ui->metaDataButton, &QPushButton::clicked, this, &Camera::showMetaDataDialog);

//    setCamera(QMediaDevices::defaultVideoInput(), 0);

#if DEV
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    for (const QCameraDevice &cameraDevice : cameras) {
        qDebug() << cameraDevice.id();
        qDebug() << cameraDevice.description();
    }

#endif

}

void Camera::setCamera(const QCameraDevice &cameraDevice,const int CameraId)
{
    // 初始化摄像头
    m_camera[0].reset(new QCamera(cameraDevice));

    // 初始化捕获会话
    m_captureSession[0].setCamera(m_camera[0].data());

    connect(m_camera[0].data(), &QCamera::activeChanged, this, &Camera::updateCameraActive);
    connect(m_camera[0].data(), &QCamera::errorOccurred, this, &Camera::displayCameraError);

    // 设置声音输入
    if (!m_mediaRecorder[0]) {
        m_mediaRecorder[0].reset(new QMediaRecorder);
        m_captureSession[0].setRecorder(m_mediaRecorder[0].data());
        connect(m_mediaRecorder[0].data(), &QMediaRecorder::recorderStateChanged, this, &Camera::updateRecorderState);
    }

    m_imageCapture[0] = new QImageCapture;
    m_captureSession[0].setImageCapture(m_imageCapture[0]);

    connect(m_mediaRecorder[0].data(), &QMediaRecorder::durationChanged, this, [=](){this->updateRecordTime(CameraId);});
    connect(m_mediaRecorder[0].data(), &QMediaRecorder::errorChanged, this, &Camera::displayRecorderError);

    connect(ui->exposureCompensation, &QAbstractSlider::valueChanged, this, &Camera::setExposureCompensation);

    // 设置摄像头会话至显示窗口
    m_captureSession[0].setVideoOutput(ui->viewfinder);

    updateCameraActive(m_camera[0]->isActive());
    updateRecorderState(m_mediaRecorder[0]->recorderState());

    connect(m_imageCapture[0], &QImageCapture::readyForCaptureChanged, this, &Camera::readyForCapture);
    connect(m_imageCapture[0], &QImageCapture::imageCaptured, this, &Camera::processCapturedImage);
    connect(m_imageCapture[0], &QImageCapture::imageSaved, this, &Camera::imageSaved);
    connect(m_imageCapture[0], &QImageCapture::errorOccurred, this, &Camera::displayCaptureError);
    readyForCapture(m_imageCapture[0]->isReadyForCapture());

    updateCaptureMode();

    if (m_camera[0]->cameraFormat().isNull()) {
        auto formats = cameraDevice.videoFormats();
        if (!formats.isEmpty()) {
            // Choose a decent camera format: Maximum resolution at at least 30 FPS
            // we use 29 FPS to compare against as some cameras report 29.97 FPS...
            QCameraFormat bestFormat;
            for (const auto &fmt : formats) {
                if (bestFormat.maxFrameRate() < 29 && fmt.maxFrameRate() > bestFormat.maxFrameRate())
                    bestFormat = fmt;
                else if (bestFormat.maxFrameRate() == fmt.maxFrameRate() &&
                         bestFormat.resolution().width()*bestFormat.resolution().height() <
                         fmt.resolution().width()*fmt.resolution().height())
                    bestFormat = fmt;
            }

            m_camera[0]->setCameraFormat(bestFormat);
            m_mediaRecorder[0]->setVideoFrameRate(bestFormat.maxFrameRate());
        }
    }

    m_camera[0]->start();
}

void Camera::keyPressEvent(QKeyEvent * event)
{
    if (event->isAutoRepeat())
        return;

    switch (event->key()) {
        case Qt::Key_CameraFocus:
            displayViewfinder();
            event->accept();
            break;
        case Qt::Key_Camera:
            if (m_doImageCapture) {
                takeImage(0);
            } else {
                if (m_mediaRecorder[0]->recorderState() == QMediaRecorder::RecordingState)
                    stop(0);
                else
                    record(0);
            }
            event->accept();
            break;
        default:
            QMainWindow::keyPressEvent(event);
    }
}

void Camera::keyReleaseEvent(QKeyEvent *event)
{
    QMainWindow::keyReleaseEvent(event);
}

void Camera::updateRecordTime(const int CameraId)
{
    QString str = QString("Recorded %1 sec").arg(m_mediaRecorder[CameraId]->duration()/1000);
    ui->statusbar->showMessage(str);
}

void Camera::processCapturedImage(int requestId, const QImage& img)
{
    Q_UNUSED(requestId);
    QImage scaledImage = img.scaled(ui->viewfinder->size(),
                                    Qt::KeepAspectRatio,
                                    Qt::SmoothTransformation);

    ui->lastImagePreviewLabel->setPixmap(QPixmap::fromImage(scaledImage));

    // Display captured image for 4 seconds.
    displayCapturedImage();
    QTimer::singleShot(4000, this, &Camera::displayViewfinder);
}

void Camera::configureCaptureSettings()
{
    if (m_doImageCapture)
        configureImageSettings();
    else
        configureVideoSettings();
}

void Camera::configureVideoSettings()
{
    VideoSettings settingsDialog(m_mediaRecorder[0].data());
    settingsDialog.setWindowFlags(settingsDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);

    if (settingsDialog.exec())
        settingsDialog.applySettings();
}

void Camera::configureImageSettings()
{
    ImageSettings settingsDialog(m_imageCapture[0]);
    settingsDialog.setWindowFlags(settingsDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);

    if (settingsDialog.exec()) {
        settingsDialog.applyImageSettings();
    }
}

void Camera::record(const int CameraId)
{
    m_mediaRecorder[CameraId]->record();
    updateRecordTime(CameraId);
}

void Camera::pause(const int CameraId)
{
    m_mediaRecorder[CameraId]->pause(CameraId);
}

void Camera::stop(const int CameraId)
{
    m_mediaRecorder[CameraId]->stop(CameraId);
}

void Camera::setMuted(bool muted,const int CameraId)
{
    m_captureSession[CameraId].audioInput()->setMuted(muted);
}

void Camera::takeImage(const int CameraId)
{
    m_isCapturingImage = true;
    m_imageCapture[CameraId]->captureToFile();
}

void Camera::displayCaptureError(int id, const QImageCapture::Error error, const QString &errorString)
{
    Q_UNUSED(id);
    Q_UNUSED(error);
    QMessageBox::warning(this, tr("Image Capture Error"), errorString);
    m_isCapturingImage = false;
}

void Camera::startCamera(const int CameraId)
{
    m_camera[CameraId]->start();
}

void Camera::stopCamera(const int CameraId)
{
    m_camera[CameraId]->stop();
}

void Camera::updateCaptureMode()
{
    int tabIndex = ui->captureWidget->currentIndex();
    m_doImageCapture = (tabIndex == 0);
}

void Camera::updateCameraActive(bool active)
{
    if (active) {
        ui->actionStartCamera->setEnabled(false);
        ui->actionStopCamera->setEnabled(true);
        ui->captureWidget->setEnabled(true);
        ui->actionSettings->setEnabled(true);
    } else {
        ui->actionStartCamera->setEnabled(true);
        ui->actionStopCamera->setEnabled(false);
        ui->captureWidget->setEnabled(false);
        ui->actionSettings->setEnabled(false);
    }
}

void Camera::updateRecorderState(QMediaRecorder::RecorderState state)
{
    switch (state) {
        case QMediaRecorder::StoppedState:
            ui->recordButton->setEnabled(true);
            ui->pauseButton->setEnabled(true);
            ui->stopButton->setEnabled(false);
            ui->metaDataButton->setEnabled(true);
            break;
        case QMediaRecorder::PausedState:
            ui->recordButton->setEnabled(true);
            ui->pauseButton->setEnabled(false);
            ui->stopButton->setEnabled(true);
            ui->metaDataButton->setEnabled(false);
            break;
        case QMediaRecorder::RecordingState:
            ui->recordButton->setEnabled(false);
            ui->pauseButton->setEnabled(true);
            ui->stopButton->setEnabled(true);
            ui->metaDataButton->setEnabled(false);
            break;
    }
}

void Camera::setExposureCompensation(int index,const int CameraId)
{
    m_camera[CameraId]->setExposureCompensation(index*0.5);
}

void Camera::displayRecorderError(const int CameraId)
{
    if (m_mediaRecorder[CameraId]->error() != QMediaRecorder::NoError)
        QMessageBox::warning(this, tr("Capture Error"), m_mediaRecorder[0]->errorString());
}

void Camera::displayCameraError(const int CameraId)
{
    if (m_camera[CameraId]->error() != QCamera::NoError)
        QMessageBox::warning(this, tr("Camera Error"), m_camera[CameraId]->errorString());
}

void Camera::updateCameraDevice(QAction *action)
{
    setCamera(qvariant_cast<QCameraDevice>(action->data()),0);
}

void Camera::displayViewfinder()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void Camera::displayCapturedImage()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void Camera::readyForCapture(bool ready)
{
    ui->takeImageButton->setEnabled(ready);
}

void Camera::imageSaved(int id, const QString &fileName)
{
    Q_UNUSED(id);
    ui->statusbar->showMessage(tr("Captured \"%1\"").arg(QDir::toNativeSeparators(fileName)));

    m_isCapturingImage = false;
    if (m_applicationExiting)
        close();
}

void Camera::closeEvent(QCloseEvent *event)
{
    if (m_isCapturingImage) {
        setEnabled(false);
        m_applicationExiting = true;
        event->ignore();
    } else {
        event->accept();
    }
}

void Camera::updateCameras()
{
    ui->menuDevices->clear();
    const QList<QCameraDevice> availableCameras = QMediaDevices::videoInputs();
    for (const QCameraDevice &cameraDevice : availableCameras) {
        QAction *videoDeviceAction = new QAction(cameraDevice.description(), videoDevicesGroup);
        videoDeviceAction->setCheckable(true);
        videoDeviceAction->setData(QVariant::fromValue(cameraDevice));
        if (cameraDevice == QMediaDevices::defaultVideoInput())
            videoDeviceAction->setChecked(true);

        ui->menuDevices->addAction(videoDeviceAction);
    }
}


void Camera::showMetaDataDialog()
{
    if (!m_metaDataDialog)
        m_metaDataDialog = new MetaDataDialog(this);
    m_metaDataDialog->setAttribute(Qt::WA_DeleteOnClose, false);
    if (m_metaDataDialog->exec() == QDialog::Accepted)
        saveMetaData(0);
}

void Camera::saveMetaData(const int CameraId)
{
    QMediaMetaData data;
    for (int i = 0; i < QMediaMetaData::NumMetaData; i++) {
        QString val = m_metaDataDialog->m_metaDataFields[i]->text();
        if (!val.isEmpty()) {
            auto key = static_cast<QMediaMetaData::Key>(i);
            if (i == QMediaMetaData::CoverArtImage) {
                QImage coverArt(val);
                data.insert(key, coverArt);
            }
            else if (i == QMediaMetaData::ThumbnailImage) {
                QImage thumbnail(val);
                data.insert(key, thumbnail);
            }
            else if (i == QMediaMetaData::Date) {
                QDateTime date = QDateTime::fromString(val);
                data.insert(key, date);
            }
            else {
                data.insert(key, val);
            }
        }
    }
    m_mediaRecorder[CameraId]->setMetaData(data);
}