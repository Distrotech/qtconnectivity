/****************************************************************************
**
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

#include "localdevicebroadcastreceiver_p.h"

QT_BEGIN_NAMESPACE

LocalDeviceBroadcastReceiver::LocalDeviceBroadcastReceiver(QObject *parent) :
    AndroidBroadcastReceiver(parent), previousScanMode(0)
{
    addAction(QStringLiteral("android.bluetooth.device.action.BOND_STATE_CHANGED"));
    addAction(QStringLiteral("android.bluetooth.adapter.action.SCAN_MODE_CHANGED"));
}

void LocalDeviceBroadcastReceiver::onReceive(JNIEnv *env, jobject context, jobject intent)
{
    Q_UNUSED(context);
    Q_UNUSED(env);

    QAndroidJniObject intentObject(intent);
    const QString action = intentObject.callObjectMethod("getAction", "()Ljava/lang/String;").toString();
    const QString arg = QStringLiteral("LocalDeviceBroadcastReceiver::onReceive() - event: %1").arg(action);

    __android_log_print(ANDROID_LOG_DEBUG,"Qt", arg.toLatin1().constData());

    if (action == QStringLiteral("android.bluetooth.adapter.action.SCAN_MODE_CHANGED")) {
        QAndroidJniObject extrasBundle =
                intentObject.callObjectMethod("getExtras","()Landroid/os/Bundle;");
        QAndroidJniObject keyExtra = QAndroidJniObject::fromString(
                    QStringLiteral("android.bluetooth.adapter.extra.SCAN_MODE"));

        int extra = extrasBundle.callMethod<jint>("getInt",
                                                  "(Ljava/lang/String;)I",
                                                  keyExtra.object<jstring>());

        if (previousScanMode != extra) {
            previousScanMode = extra;

            switch (extra) {
                case 20: //BluetoothAdapter.SCAN_MODE_NONE
                    emit hostModeStateChanged(QBluetoothLocalDevice::HostPoweredOff);
                    break;
                case 21: //BluetoothAdapter.SCAN_MODE_CONNECTABLE
                    emit hostModeStateChanged(QBluetoothLocalDevice::HostConnectable);
                    break;
                case 23: //BluetoothAdapter.SCAN_MODE_CONNECTABLE_DISCOVERABLE
                    emit hostModeStateChanged(QBluetoothLocalDevice::HostDiscoverable);
                    break;
                default:
                    qWarning() << "Unknown Host State";
                    break;
            }
        }
    }
}

QT_END_NAMESPACE
