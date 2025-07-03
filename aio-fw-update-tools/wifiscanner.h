#ifndef WIFISCANNER_H
#define WIFISCANNER_H

#include <QObject>
#include <QList>
#include <QThread>
#include <Windows.h>
#include <wlanapi.h>

// WiFi网络数据结构
struct WiFiNetwork {
    QString ssid;
    int signalStrength; // dBm值
    QString bssid;      // MAC地址
    int channel;        // 频道
    int frequency;      // 频率(MHz)
    QString encryption; // 加密类型

    // 简化创建
    WiFiNetwork(QString s, int sig, QString mac, int ch, int freq, QString enc)
        : ssid(s), signalStrength(sig), bssid(mac), channel(ch), frequency(freq), encryption(enc) {}
};

class WiFiScanner : public QObject
{
    Q_OBJECT
public:
    explicit WiFiScanner(QObject *parent = nullptr);
    ~WiFiScanner();

    void handleWlanNotification(PWLAN_NOTIFICATION_DATA pNotifData);
public slots:
    void startScan();  // 开始扫描
    void stopScan();   // 停止扫描

signals:
    void scanStarted();  // 扫描开始信号
    void scanFinished(); // 扫描完成信号
    void networksFound(const QList<WiFiNetwork> &networks); // 发现网络信号
    void scanError(const QString &errorMessage); // 错误信号

private:
    HANDLE m_hClient = nullptr;
    GUID m_interfaceGuid;
    bool m_scanActive = false;

    // 工作线程
    QThread m_workerThread;

    void initWiFiApi();  // 初始化WLAN API
    void cleanUpWiFiApi(); // 清理资源

    // 扫描实际执行方法（在线程中运行）
    void performScan();

};

#endif // WIFISCANNER_H
