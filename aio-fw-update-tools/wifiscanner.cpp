#include "wifiscanner.h"
#include <QDebug>

// 全局回调函数（转换为成员函数调用）
void CALLBACK WlanCallbackWrapper(PWLAN_NOTIFICATION_DATA pNotifData, PVOID pContext) {
    WiFiScanner* scanner = static_cast<WiFiScanner*>(pContext);
    if (scanner) scanner->handleWlanNotification(pNotifData);
}

WiFiScanner::WiFiScanner(QObject *parent) : QObject(parent) {
    // 将工作对象移动到工作线程
    this->moveToThread(&m_workerThread);
    connect(&m_workerThread, &QThread::finished, this, &WiFiScanner::cleanUpWiFiApi);

    // 连接线程执行信号
    connect(this, &WiFiScanner::startScan, this, &WiFiScanner::performScan);

    // 启动线程
    m_workerThread.start();
}

WiFiScanner::~WiFiScanner() {
    stopScan();
    m_workerThread.quit();
    m_workerThread.wait();
}

void WiFiScanner::startScan() {
    if (!m_scanActive) {
        m_scanActive = true;
        QMetaObject::invokeMethod(this, "startScan"); // 确保在工作线程执行
    }
}

void WiFiScanner::stopScan() {
    m_scanActive = false;
}

void WiFiScanner::initWiFiApi() {
    if (m_hClient) return; // 已初始化

    DWORD dwClientVersion = 2;
    DWORD dwNegotiatedVersion = 0;

    DWORD dwResult = WlanOpenHandle(dwClientVersion, NULL,
                                   &dwNegotiatedVersion, &m_hClient);

    if (dwResult != ERROR_SUCCESS) {
        emit scanError(tr("Failed to open WLAN handle (Error: %1)").arg(dwResult));
        return;
    }

    // 注册回调函数
    dwResult = WlanRegisterNotification(m_hClient, WLAN_NOTIFICATION_SOURCE_ACM,
                                       TRUE, WlanCallbackWrapper,
                                       this, NULL, NULL);

    if (dwResult != ERROR_SUCCESS) {
        emit scanError(tr("Notification registration failed (Error: %1)").arg(dwResult));
        cleanUpWiFiApi();
        return;
    }

    // 获取无线接口
    PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
    dwResult = WlanEnumInterfaces(m_hClient, NULL, &pIfList);

    if (dwResult != ERROR_SUCCESS || pIfList->dwNumberOfItems == 0) {
        emit scanError(tr("No WiFi interfaces found"));
        if (pIfList) WlanFreeMemory(pIfList);
        return;
    }

    // 使用第一个无线接口
    m_interfaceGuid = pIfList->InterfaceInfo[0].InterfaceGuid;
    WlanFreeMemory(pIfList);
}

void WiFiScanner::cleanUpWiFiApi() {
    if (m_hClient) {
        WlanCloseHandle(m_hClient, NULL);
        m_hClient = nullptr;
    }
}

void WiFiScanner::performScan() {
    initWiFiApi();

    if (!m_hClient || !m_scanActive) return;

    emit scanStarted();

    // 开始扫描
    DWORD dwResult = WlanScan(m_hClient, &m_interfaceGuid, NULL, NULL, NULL);

    if (dwResult != ERROR_SUCCESS) {
        emit scanError(tr("Scan failed to start (Error: %1)").arg(dwResult));
        emit scanFinished();
    }
}

// 处理WLAN通知回调
void WiFiScanner::handleWlanNotification(PWLAN_NOTIFICATION_DATA pNotifData) {
    if (!m_scanActive) return;

    switch (pNotifData->NotificationCode) {
    case wlan_notification_acm_scan_complete: {
        QList<WiFiNetwork> networks;

        PWLAN_BSS_LIST pBssList = NULL;
        DWORD dwResult = WlanGetNetworkBssList(m_hClient, &m_interfaceGuid,
                                              NULL, dot11_BSS_type_any,
                                              true, NULL, &pBssList);

        if (dwResult == ERROR_SUCCESS) {
            // 收集所有网络
            for (DWORD i = 0; i < pBssList->dwNumberOfItems; i++) {
                PWLAN_BSS_ENTRY entry = &pBssList->wlanBssEntries[i];

                QString ssid = QString::fromLatin1(
                    (char*)entry->dot11Ssid.ucSSID,
                    entry->dot11Ssid.uSSIDLength
                );

                // 跳过隐藏网络
                if (ssid.isEmpty() || ssid.isNull()) continue;

                // 获取MAC地址
                QString bssid = QByteArray((char*)entry->dot11Bssid, 6)
                                .toHex(':').toUpper();

                // 获取安全信息
                QString encryption;
                if (entry->dot11BssType == dot11_BSS_type_infrastructure) {
                    encryption = (entry->wlanRateSet.uRateSetLength > 0) ? "WPA2" : "Open";
                } else {
                    encryption = "Unknown";
                }

                WiFiNetwork network(
                    ssid,
                    entry->lRssi, // 信号强度
                    bssid,
                    entry->ulChCenterFrequency, // 频率
                    entry->ulChCenterFrequency / 1000, // 转换为MHz
                    encryption
                );

                networks.append(network);
            }

            // 按信号强度排序
            std::sort(networks.begin(), networks.end(),
                     [](const WiFiNetwork &a, const WiFiNetwork &b) {
                         return a.signalStrength > b.signalStrength;
                     });

            emit networksFound(networks);
        }

        if (pBssList) WlanFreeMemory(pBssList);
        emit scanFinished();
        break;
    }
    case wlan_notification_acm_scan_fail:
        emit scanError("WiFi scan failed");
        emit scanFinished();
        break;
    }
}
