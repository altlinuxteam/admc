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

#include "tabs/account_tab.h"
#include "edits/string_edit.h"
#include "edits/expiry_edit.h"
#include "edits/unlock_edit.h"
#include "edits/account_option_edit.h"

#include <QFormLayout>

// TODO: logon hours, logon computers

AccountTab::AccountTab() {
    auto upn_edit = new StringEdit(ATTRIBUTE_USER_PRINCIPAL_NAME, CLASS_USER, &edits, this);

    auto unlock_edit = new UnlockEdit(&edits, this);

    auto expiry_edit = new ExpiryEdit(&edits, this);

    QList<AccountOption> options;
    for (int i = 0; i < AccountOption_COUNT; i++) {
        const AccountOption option = (AccountOption) i;
        options.append(option);
    }
    QMap<AccountOption, AccountOptionEdit *> option_edits;
    AccountOptionEdit::make_many(options, &option_edits, &edits, this);
    QWidget *options_widget = AccountOptionEdit::layout_many(options, option_edits);

    edits_connect_to_tab(edits, this);

    auto edits_layout = new QFormLayout();

    const QList<AttributeEdit *> top_edits = {
        upn_edit,
        unlock_edit,
        expiry_edit,
    };
    edits_add_to_layout(top_edits, edits_layout);

    auto layout = new QVBoxLayout();
    setLayout(layout);
    layout->addLayout(edits_layout);
    layout->addWidget(options_widget);
}
