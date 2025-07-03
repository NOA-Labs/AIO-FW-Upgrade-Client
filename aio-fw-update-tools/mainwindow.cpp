#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QFileDialog>
#include <QDebug>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

//    m_scanner = new WiFiScanner(this);

//    connectSignals();

    checkWifiAvailable();
    ui->openDevStatus->setStyleSheet("color: blue");
    ui->openDevStatus->setText(tr(""));

    //ui->fwUpdateBtn->setEnabled(false);

    connect(ui->resetBtn, &QPushButton::clicked, [=](){
        resetAllWidget();
        if(isSerialOpen){
            serial.write("reboot\r\n");
        }
    });
    //扫描设备scan device.
    scanBLETimeoutTimer = new QTimer(this);
    scanBLETimeoutTimer->setInterval(1000);
    connect(scanBLETimeoutTimer, &QTimer::timeout, [=](){
        curBlueToothScanSec++;
        ui->openDevStatus->setText(QString("扫描中 %1").arg(curBlueToothScanSec));
        if(curBlueToothScanSec >= bluetoothScanTimeout){
            isBluetoothScan = false;
            ui->scanDevBtn->setText(tr("扫描设备"));
            scanBLETimeoutTimer->stop();
            ui->openDevStatus->setText(tr("扫描完成"));
            ui->openDevBtn->setEnabled(true);
        }
    });
    connect(ui->scanDevBtn, &QPushButton::clicked, [=](){
        if(isBluetoothScan){
           isBluetoothScan = false;
           ui->scanDevBtn->setText(tr("扫描设备"));
           ui->openDevStatus->setStyleSheet("color: blue;");
           ui->openDevStatus->setText(tr("停止扫描"));
           ui->openDevBtn->setEnabled(true);
           serial.write("stop\r\n");
           scanBLETimeoutTimer->stop();
           return;
        }
        if(isSerialOpen){
            isBluetoothScan = true;
            curBlueToothScanSec = 0;
            ui->scanDevBtn->setText(tr("停止扫描"));
            ui->openDevBtn->setEnabled(false);
            ui->devList->clear();
            ui->openDevStatus->setStyleSheet("color: blue;");
            ui->openDevStatus->setText(QString("扫描中 %1").arg(curBlueToothScanSec));
            scanBLETimeoutTimer->start();
            serial.write("scan\r\n");
        }
    });
    //连接设备
    connectBLETimeoutTimer = new QTimer(this);
    connectBLETimeoutTimer->setInterval(10000);
    connect(connectBLETimeoutTimer, &QTimer::timeout, [=](){
        ui->openDevStatus->setStyleSheet("color: red;");
        ui->openDevStatus->setText("连接失败");
        ui->scanDevBtn->setEnabled(true);
        connectBLETimeoutTimer->stop();
        isBluetoothConnecting = false;
    });
    connect(ui->openDevBtn, &QPushButton::clicked, [=](){
        int index = ui->devList->currentRow();
        qDebug()<<"select current index: "<<index;
        if(index >= 0){
            if(isSerialOpen){
                if(!isBluetoothConnected){
                    QString cmd = QString("connect:%1\r\n").arg(index);
                    serial.write(cmd.toStdString().c_str());
                    qDebug()<<cmd;
                    ui->openDevStatus->setStyleSheet("color: blue;");
                    ui->openDevStatus->setText(tr("正在连接设备"));
                    ui->scanDevBtn->setEnabled(false);
                    connectBLETimeoutTimer->start();
                    isBluetoothConnecting = true;
                    isBluetoothConnected = false;
                    serverUUID.clear();
                    charactUUID.clear();
                }
                else{
                    isBluetoothConnected = false;
                    isSetUUID = false;
                    ui->openDevBtn->setText(tr("连接设备"));
                    ui->openDevStatus->setStyleSheet("color: red;");
                    ui->openDevStatus->setText("设备断开");
                    ui->scanDevBtn->setEnabled(true);
                }
            }
        }
    });
    //打开固件
    connect(ui->openFwFileBtn, &QPushButton::clicked, [=](){
        QString path = QFileDialog::getOpenFileName(this, tr("打开文件"), "", "image (*.bin)");
//        QFileInfo fileInfo(path);
//        fwFileName = fileInfo.fileName();
        fwFileName = path;
        ui->fwFilePath->setText(fwFileName);
    });

    //C0 5B 9F 28 04 14 56 B8 FA 08 00 00 00 00 C0
    //C0 E7 4D 06 DD 96 60 EA 67 40 F4 01 F4 01 C0 //
    //C0 06 8D B8 FF 27 93 3A DD 08 01 00 00 00 C0 //get wifi mac
    //C0 75 31 52 86 44 84 90 E6 09 8C AA B5 80 C0 //open wifi.
//    connect(ui->enterFwUpdateModeBtn, &QPushButton::clicked, m_scanner, &WiFiScanner::startScan);
    connect(ui->enterFwUpdateModeBtn, &QPushButton::clicked, [=](){//发送数据让设备进入升级状态；
        if(serverUUID.isEmpty() || charactUUID.isEmpty()){//没有收到正确的uuid，需要重新复位
            ui->information->append(tr("\r\nUUID无效，请点击 “复位” 按钮，重新开始。"));
            return;
        }

        if(isSerialOpen){
            uint8_t cmd[] = {0xC0, 0x75, 0x31, 0x52, 0x86, 0x00, 0x00, 0x00, 0x00, 0x09, 0x8c, 0xaa, 0xb5, 0x80, 0xC0};
            QByteArray dev = ui->devList->currentItem()->text().toUtf8();
            QByteArray id = getDevMacAddr(dev);
            qDebug()<<"Device mac address: "<<id;
            QByteArray item;
            item.clear();
            item.append(id[0]);
            item.append(id[1]);
            cmd[10] = item.toUInt(nullptr, 16);
            item.clear();
            item.append(id[3]);
            item.append(id[4]);
            cmd[11] = item.toUInt(nullptr, 16);
            item.clear();
            item.append(id[6]);
            item.append(id[7]);
            cmd[12] = item.toUInt(nullptr, 16);
            item.clear();
            item.append(id[9]);
            item.append(id[10]);
            cmd[13] = item.toUInt(nullptr, 16);
            uint32_t crc = crc32_le(0, &cmd[1], sizeof(cmd) - 2);
            qDebug()<<"crc: "<< crc;
            cmd[8] = uint8_t((crc >> 24) & 0xff);
            cmd[7] = uint8_t((crc >> 16) & 0xff);
            cmd[6] = uint8_t((crc >> 8)  & 0xff);
            cmd[5] = uint8_t( crc        & 0xff);
            QByteArray send;
            send.append("write:");
            for(int i = 0; i < (int)sizeof(cmd); i++){
                send.append(QString("%1").arg(cmd[i], 2, 16, QChar('0')));
            }
            qDebug()<<send;
            serial.write(send);
            startUpdateTimer->start();
        }
    });
    startUpdateTimer = new QTimer(this);
    startUpdateTimer->setInterval(1500);
    connect(startUpdateTimer, &QTimer::timeout, [=](){
        startUpdateTimer->stop();
        //ui->fwUpdateBtn->setEnabled(true);
    });
    //固件更新开始按钮；
    connect(ui->fwUpdateBtn, &QPushButton::clicked, [=](){
        if(!isFwUpdateRuning){
            ui->information->clear();
            ui->information->setFontPointSize(10);
            process.setWorkingDirectory(QString("./remote_ota"));
            process.start("cmd", QStringList()<<"/c"<<QString("export.bat %1").arg(fwFileName));
            isFwUpdateRuning = true;
            ui->fwUpdateBtn->setText(tr("停止"));
        }
        else{
            process.kill();
            isFwUpdateRuning = false;
            ui->fwUpdateBtn->setText("开始");
        }
    });
    //清除显示
    connect(ui->clearBtn, &QPushButton::clicked, [=](){
        ui->information->clear();
    });
    connect(&process, &QProcess::readyReadStandardOutput, [=](){
        QString str = QString::fromLocal8Bit(process.readAllStandardOutput());
        ui->information->append(str);
        qDebug()<<str;
    });

    connect(&process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
         [=](int exitCode, QProcess::ExitStatus exitStatus){
        process.kill();
        ui->fwUpdateBtn->setText("开始");
        isFwUpdateRuning = false;
    });

    serialScanTimer = new QTimer(this);
    serialScanTimer->setInterval(1000);
    serialScanTimer->start();
    connect(serialScanTimer, &QTimer::timeout, [=](){
        foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts()){
            bool newPort = true;
            foreach(const QSerialPortInfo &e, serialInfoList){
                if(e.portName() == info.portName()){
                    newPort = false;
                    break;
                }
            }
            if(newPort){
                serialInfoList << info;
                ui->serialCBox->addItem(info.description() + " (" + info.portName() + ")");
            }
        }
    });

    readTimeoutTimer = new QTimer(this);
    readTimeoutTimer->setInterval(10);
    connect(readTimeoutTimer, &QTimer::timeout, [=](){
        readTimeoutTimer->stop();
        ui->information->append(receiveBytes.toStdString().c_str());
        if(isBluetoothScan){
            addDevToList(receiveBytes);
        }
        else if(isBluetoothConnecting){//正在连接中
            if(receiveBytes.indexOf("successful") > 0){
                ui->openDevStatus->setText("连接成功");
                connectBLETimeoutTimer->stop();
                isBluetoothConnecting = false;
                isBluetoothConnected = true;
                ui->openDevBtn->setText(tr("关闭连接"));
            }
        }
        else if(isBluetoothConnected){
            setSeverUUID(receiveBytes);
            setCharactUUID(receiveBytes);
            sendUUID();
        }
        receiveBytes.clear();
    });
    connect(ui->openSerialBtn, &QPushButton::clicked, [=](){
        if(isSerialOpen){
            isSerialOpen = false;
            resetAllWidget();
            closeSerialPort();
            return;
        }
        openSerialPort();
    });

    connect(&serial, &QSerialPort::readyRead, this, [=](){
        readTimeoutTimer->start();
        receiveBytes.append(serial.readAll());
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::connectSignals() {
    // 连接扫描器信号到主窗口槽函数
    connect(m_scanner, &WiFiScanner::scanStarted, this, &MainWindow::onScanStarted);
    connect(m_scanner, &WiFiScanner::scanFinished, this, &MainWindow::onScanFinished);
    connect(m_scanner, &WiFiScanner::networksFound, this, &MainWindow::onNetworksFound);
    connect(m_scanner, &WiFiScanner::scanError, this, &MainWindow::onScanError);
}

void MainWindow::onScanStarted() {

}

void MainWindow::onScanFinished() {

}

void MainWindow::onNetworksFound(const QList<WiFiNetwork> &networks) {
    for (const WiFiNetwork &network : networks) {
        // 格式化信号强度图标
        QString strength;
        if (network.signalStrength >= -50) strength = "📶📶📶";
        else if (network.signalStrength >= -60) strength = "📶📶";
        else if (network.signalStrength >= -70) strength = "📶";
        else strength = "⚡";

        QString itemText = QString("%1 %2 %3dBm (%4)")
                              .arg(strength)
                              .arg(network.ssid)
                              .arg(network.signalStrength)
                              .arg(network.encryption);

        ui->information->append(itemText);
    }
}

void MainWindow::onScanError(const QString &errorMessage) {
    ui->information->append("Error: " + errorMessage);
}

bool MainWindow::openSerialPort()
{
    QSerialPortInfo info = serialInfoList.at(ui->serialCBox->currentIndex());
    qDebug()<<"current port: "<<info.description()<<" ("<<info.portName()<<")";
    serial.setBaudRate(QSerialPort::Baud115200);
    serial.setPortName(info.portName());
    serial.setDataBits(QSerialPort::Data8);
    serial.setParity(QSerialPort::NoParity);
    serial.setStopBits(QSerialPort::OneStop);
    if(serial.open(QIODevice::ReadWrite)){
        ui->openSerialBtn->setText("关闭");
        isSerialOpen = true;
        ui->information->clear();
        ui->information->setFontPointSize(10);
        QString txt;
        txt = "打开串口: " + info.description() + " (" + info.portName() + ")";
        ui->information->append(txt);
        ui->serialCBox->setEnabled(false);
        serialScanTimer->stop();
    }
    else{//打开失败
        ui->information->clear();
        ui->information->setFontPointSize(10);
        QString txt;
        txt = "串口打开失败";
        ui->information->append(txt);

        return false;
    }
    return true;
}

bool MainWindow::closeSerialPort()
{
    serial.close();
    ui->openSerialBtn->setText("打开");
    ui->serialCBox->setEnabled(true);
    serialScanTimer->start();
    return true;
}

void MainWindow::addDevToList(QByteArray info)
{
    //Dev: Meter-DE49 [d9:f6:55:75:de:49]
    int start = info.indexOf("ev:");
    int end = info.indexOf("]");
    if((start >= 0) && (end > 0)){//找到了字符串
        int offset = strlen("ev:");
        QByteArray str = info.mid(start + offset, end - start - offset + 1);
        ui->devList->addItem(str);
        qDebug() << str;
    }
}

QByteArray MainWindow::getDevMacAddr(QByteArray &info)
{
    int start = info.indexOf("[");
    int end = info.indexOf("]");

    QByteArray str = info.mid(start + 1, end - start - 1);

    return str;
}
void MainWindow::resetAllWidget()
{
    isFwUpdateRuning = false;
    isBluetoothScan = false;
    isBluetoothConnecting = false;
    isBluetoothConnected = false;
    isSetUUID = false;
    ui->openDevStatus->setText(tr(""));
    ui->openDevBtn->setText(tr("连接设备"));
    ui->openDevBtn->setEnabled(true);
    ui->scanDevBtn->setText("扫描设备");
    ui->scanDevBtn->setEnabled(true);
    ui->devList->clear();
    ui->information->clear();
    scanBLETimeoutTimer->stop();
    connectBLETimeoutTimer->stop();
    serverUUID.clear();
    charactUUID.clear();
    //ui->fwUpdateBtn->setEnabled(false);
}

void MainWindow::setSeverUUID(QByteArray info)
{
    int start = info.indexOf("0000ffe0");

    if(start > 0){
         QByteArray str = info.mid(start, 36);
         serverUUID = str;
         qDebug()<< "Server UUID: " << str;
    }
}
void MainWindow::setCharactUUID(QByteArray info)
{
    int start = info.indexOf("0000ffe1");

    if(start > 0){
         QByteArray str = info.mid(start, 36);
         charactUUID = str;
         qDebug()<< "Charactristics UUID: " << str;
    }
}
void MainWindow::sendUUID()
{
    if(!isSetUUID)isSetUUID = true;
    else return;

    if(!serverUUID.isEmpty() && !charactUUID.isEmpty() && isSerialOpen){
        QString cmd;
        cmd.append("uuid|");
        cmd.append(serverUUID);
        cmd.append("|");
        cmd.append(charactUUID);
        cmd.append("|");
        qDebug()<<cmd;
        serial.write(cmd.toStdString().c_str());
        serverUUID.clear();
        charactUUID.clear();
    }
}

bool MainWindow::checkWifiAvailable()
{
    QProcess process;
    process.start("netsh", QStringList() << "wlan" << "show" << "networks|findstr" << "SSID");
//    process.start("netsh", QStringList() << "wlan" << "show" << "networks");
    process.waitForFinished();
    QString output = QString::fromUtf8(process.readAllStandardOutput());
    qDebug() << output;
    process.kill();
    return false;
}
