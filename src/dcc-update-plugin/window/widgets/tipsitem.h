// SPDX-FileCopyrightText: 2011 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DCC_UPDATE_RESULTITEM_H
#define DCC_UPDATE_RESULTITEM_H

#include "widgets/settingsitem.h"
#include "dimagebutton.h"
#include "common.h"

namespace dcc {
namespace widgets {
class NormalLabel;
}
}

namespace dcc {
namespace update {

class TipsItem : public dcc::widgets::SettingsItem
{
    Q_OBJECT

public:
    explicit TipsItem(QFrame* parent = 0);

    void setStatus(UpdatesStatus type);
    void setMessage(const QString &message);

private:
    dcc::widgets::NormalLabel* m_message;
    QLabel *m_icon;
    QString m_pix;
    UpdatesStatus m_status;
};

} // namespace update
} // namespace dcc

#endif // DCC_UPDATE_RESULTITEM_H
