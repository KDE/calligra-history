/* This file is part of the KDE project
   Copyright (C) 2005 Dag Andersen <danders@get2net.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library Cost Public
   License as published by the Free Software Foundation;
   version 2 of the License.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library Cost Public License for more details.

   You should have received a copy of the GNU Library Cost Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef KPTTASKCOSTPANEL_H
#define KPTTASKCOSTPANEL_H

#include "kpttaskcostpanelbase.h"

class KCommand;

namespace KPlato
{

class KPTTaskCostPanel;
class KPTAccounts;
class KPTPart;
class KPTTask;

class KPTTaskCostPanelImpl : public KPTTaskCostPanelBase {
    Q_OBJECT
public:
    KPTTaskCostPanelImpl(QWidget *parent=0, const char *name=0);

signals:
    void changed();

public slots:
    void slotChanged();
};

class KPTTaskCostPanel : public KPTTaskCostPanelImpl {
    Q_OBJECT
public:
    KPTTaskCostPanel(KPTTask &task, KPTAccounts &accounts, QWidget *parent=0, const char *name=0);

    KCommand *buildCommand(KPTPart *part);

    bool ok();

    void setStartValues(KPTTask &task);

protected:
    void setCurrentItem(QComboBox *box, QString name);
    
private:
    KPTTask &m_task;
    KPTAccounts &m_accounts;
    QStringList m_accountList;
};

} //KPlato namespace

#endif // KPTTASKCOSTPANEL_H
