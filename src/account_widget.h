/*
 * ADMC - AD Management Center
 *
 * Copyright (C) 2020 BaseALT Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ACCOUNT_WIDGET_H
#define ACCOUNT_WIDGET_H

#include "ad_interface.h"

#include <QWidget>
#include <QList>

class QString;
class QLineEdit;
class QCheckBox;
class QLabel;
class QPushButton;

struct UACCheck {
    QCheckBox *check;
    AccountOption option;
};

// Shows member objects of targeted group
class AccountWidget final : public QWidget {
Q_OBJECT

public:
    AccountWidget(QWidget *parent);

    void change_target(const QString &dn);

private slots:
    void on_unlock_button();
    void on_logon_name_edit();
    void on_expiry_never_check();
    void on_expiry_set_check();
    void on_expiry_edit_button();
    
private:
    QString target_dn;
    QLineEdit *logon_name_edit;
    QList<UACCheck> uac_checks;

    QCheckBox *expiry_never_check;
    QCheckBox *expiry_set_check;
    QLabel *expiry_display;
    QPushButton *expiry_edit_button;

    void reset_logon_name_edit();
};

#endif /* ACCOUNT_WIDGET_H */