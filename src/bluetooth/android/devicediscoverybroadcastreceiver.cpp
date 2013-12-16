/****************************************************************************
**
** Copyright (C) 2013 Lauri Laanmets (Proekspert AS) <lauri.laanmets@eesti.ee>
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtBluetooth module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "android/devicediscoverybroadcastreceiver_p.h"
#include <QtCore/QDebug>
#include <QtBluetooth/QBluetoothAddress>
#include <QtBluetooth/QBluetoothDeviceInfo>

QT_BEGIN_NAMESPACE

DeviceDiscoveryBroadcastReceiver::DeviceDiscoveryBroadcastReceiver(QObject* parent): AndroidBroadcastReceiver(parent)
{
   addAction(QStringLiteral("android.bluetooth.device.action.FOUND"));
   addAction(QStringLiteral("android.bluetooth.adapter.action.DISCOVERY_STARTED"));
   addAction(QStringLiteral("android.bluetooth.adapter.action.DISCOVERY_FINISHED"));
}

void DeviceDiscoveryBroadcastReceiver::onReceive(JNIEnv *env, jobject context, jobject intent)
{
    Q_UNUSED(context);
    Q_UNUSED(env);

    QAndroidJniObject intentObject(intent);
    const QString action = intentObject.callObjectMethod("getAction", "()Ljava/lang/String;").toString();

    qDebug() << "DeviceDiscoveryBroadcastReceiver::onReceive() - event:" << action;

    if (action == QStringLiteral("android.bluetooth.adapter.action.DISCOVERY_FINISHED") ) {
        emit finished();
    } else if (action == QStringLiteral("android.bluetooth.adapter.action.DISCOVERY_STARTED") ) {

    } else if (action == QStringLiteral("android.bluetooth.device.action.FOUND")) {
        //get BluetoothDevice
        QAndroidJniObject keyExtra = QAndroidJniObject::fromString(
                            QStringLiteral("android.bluetooth.device.extra.DEVICE"));
        QAndroidJniObject bluetoothDevice =
                intentObject.callObjectMethod("getParcelableExtra",
                                              "(Ljava/lang/String;)Landroid/os/Parcelable;",
                                              keyExtra.object<jstring>());

        if (!bluetoothDevice.isValid())
            return;

        const QString deviceName = bluetoothDevice.callObjectMethod<jstring>("getName").toString();
        const QBluetoothAddress deviceAddress(bluetoothDevice.callObjectMethod<jstring>("getAddress").toString());
        keyExtra = QAndroidJniObject::fromString(
                            QStringLiteral("android.bluetooth.device.extra.RSSI"));
        int rssi = intentObject.callMethod<jshort>("getShortExtra",
                                                "(Ljava/lang/String;S)S",
                                                keyExtra.object<jstring>(),
                                                0);
        QAndroidJniObject bluetoothClass = bluetoothDevice.callObjectMethod("getBluetoothClass",
                                                                            "()Landroid/bluetooth/BluetoothClass;");
        if (!bluetoothClass.isValid())
            return;
        int classType = bluetoothClass.callMethod<jint>("getDeviceClass");


        static QList<qint32> services;
        if (services.count() == 0)
            services << QBluetoothDeviceInfo::PositioningService
                     << QBluetoothDeviceInfo::NetworkingService
                     << QBluetoothDeviceInfo::RenderingService
                     << QBluetoothDeviceInfo::CapturingService
                     << QBluetoothDeviceInfo::ObjectTransferService
                     << QBluetoothDeviceInfo::AudioService
                     << QBluetoothDeviceInfo::TelephonyService
                     << QBluetoothDeviceInfo::InformationService;

        //Matching BluetoothClass.Service values
        qint32 result = 0;
        qint32 current = 0;
        for (int i = 0; i < services.count(); i++) {
            current = services.at(i);
            int id = (current << 16);
            if (bluetoothClass.callMethod<jboolean>("hasService", "(I)Z", id))
                result |= current;
        }

        result = result << 13;
        classType |= result;

        QBluetoothDeviceInfo info(deviceAddress, deviceName, classType);
        info.setRssi(rssi);

        emit deviceDiscovered(info);
    }
}

QT_END_NAMESPACE

