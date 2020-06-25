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

#include "utils.h"
#include "ad_interface.h"

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QAbstractItemView>
#include <QModelIndex>

// Converts index all the way down to source index, going through whatever chain of proxies is present
QModelIndex convert_to_source(const QModelIndex &index) {
    const QAbstractItemModel *current_model = index.model();
    QModelIndex current_index = index;

    while (true) {
        const QSortFilterProxyModel *proxy = qobject_cast<const QSortFilterProxyModel *>(current_model);

        if (proxy != nullptr) {
            current_index = proxy->mapToSource(current_index);

            current_model = proxy->sourceModel();
        } else {
            return current_index;
        }
    }
}

// Row index can be an index of any column in target row and of any proxy in the proxy chain
QString get_dn_from_index(const QModelIndex &base_row_index, int dn_column) {
    const QModelIndex row_index = convert_to_source(base_row_index);
    const QModelIndex dn_index = row_index.siblingAtColumn(dn_column);
    const QString dn = dn_index.data().toString();

    return dn;
}

QIcon get_entry_icon(const QString &dn) {
    // TODO: change to custom, good icons, add those icons to installation?
    // TODO: are there cases where an entry can have multiple icons due to multiple objectClasses and one of them needs to be prioritized?
    QMap<QString, QString> class_to_icon = {
        {"groupPolicyContainer", "x-office-address-book"},
        {"container", "folder"},
        {"organizationalUnit", "network-workgroup"},
        {"person", "avatar-default"},
        {"group", "application-x-smb-workgroup"},
        {"builtinDomain", "emblem-system"},
    };
    QString icon_name = "dialog-question";
    for (auto c : class_to_icon.keys()) {
        if (AD()->attribute_value_exists(dn, "objectClass", c)) {
            icon_name = class_to_icon[c];
            break;  
        }
    }

    QIcon icon = QIcon::fromTheme(icon_name);

    return icon;
}

// Set root index of view to head index of current model
// Do this to hide a head node in view while retaining it in model
// for drag and drop purposes
void set_root_to_head(QAbstractItemView *view) {
    const QAbstractItemModel *view_model = view->model();
    const QModelIndex head_index = view_model->index(0, 0);
    view->setRootIndex(head_index);
}
