// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

// HostName
const static QString HostnameService = QStringLiteral("org.freedesktop.hostname1");
const static QString HostnamePath = QStringLiteral("/org/freedesktop/hostname1");
const static QString HostnameInterface = QStringLiteral("org.freedesktop.hostname1");

// Updater
const static QString UpdaterService = QStringLiteral("org.deepin.dde.Lastore1");
const static QString UpdaterPath = QStringLiteral("/org/deepin/dde/Lastore1");
const static QString UpdaterInterface = QStringLiteral("org.deepin.dde.Lastore1.Updater");

// Manager
const static QString ManagerService = QStringLiteral("org.deepin.dde.Lastore1");
const static QString ManagerPath = QStringLiteral("/org/deepin/dde/Lastore1");
const static QString ManagerInterface = QStringLiteral("org.deepin.dde.Lastore1.Manager");

// PowerInter
const static QString PowerService = QStringLiteral("org.deepin.dde.Power1");
const static QString PowerPath = QStringLiteral("/org/deepin/dde/Power1");
const static QString PowerInterface = QStringLiteral("org.deepin.dde.Power1");

// Atomic Upgrade
const static QString AtomicUpdaterService = QStringLiteral("org.deepin.AtomicUpgrade1");
const static QString AtomicUpdaterPath = QStringLiteral("/org/deepin/AtomicUpgrade1");
const static QString AtomicUpdaterJobInterface = QStringLiteral("org.deepin.AtomicUpgrade1");

// LockService
const static QString LockService = QStringLiteral("org.deepin.dde.LockService1");
const static QString LockPath = QStringLiteral("/org/deepin/dde/LockService1");
const static QString LockInterface = QStringLiteral("org.deepin.dde.LockService1");

const static QString PropertiesInterface = QStringLiteral("org.freedesktop.DBus.Properties");
const static QString PropertiesChanged = QStringLiteral("PropertiesChanged");
