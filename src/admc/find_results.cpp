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

#include "find_results.h"
#include "object_menu.h"
#include "details_dialog.h"
#include "utils.h"
#include "filter.h"
#include "ad_interface.h"
#include "ad_config.h"
#include "object_model.h"
#include "settings.h"

#include <QTreeView>
#include <QLabel>
#include <QHeaderView>
#include <QDebug>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QMenu>

// NOTE: loading unrestricted amount of results (10k for example) causes model destruction to take too long, which freezes the app when find dialog is closed.
#define RESULTS_COUNT_MAX 999

FindResults::FindResults()
: QWidget()
{   
    model = new QStandardItemModel(this);

    const QList<QString> header_labels = object_model_header_labels();
    model->setHorizontalHeaderLabels(header_labels);

    view = new QTreeView(this);
    view->setAcceptDrops(true);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    view->setRootIsDecorated(false);
    view->setItemsExpandable(false);
    view->setExpandsOnDoubleClick(false);
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    view->setDragDropMode(QAbstractItemView::DragDrop);
    view->setAllColumnsShowFocus(true);
    view->setSortingEnabled(true);
    view->header()->setSectionsMovable(true);

    SETTINGS()->setup_header_state(view->header(), VariantSetting_FindResultsHeader);

    view->setModel(model);

    setup_column_toggle_menu(view, model,
    {
        ADCONFIG()->get_column_index(ATTRIBUTE_NAME),
        ADCONFIG()->get_column_index(ATTRIBUTE_OBJECT_CLASS),
        ADCONFIG()->get_column_index(ATTRIBUTE_DESCRIPTION)
    });

    object_count_label = new QLabel();

    const auto layout = new QVBoxLayout();
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->addWidget(object_count_label);
    layout->addWidget(view);

    connect(
        view, &QWidget::customContextMenuRequested,
                this, &FindResults::open_context_menu);
}

void FindResults::load_menu(QMenu *menu) {
    menu->clear();
    add_object_actions_to_menu(menu, view, this);
}

void FindResults::open_context_menu(const QPoint pos) {
    auto menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);
    load_menu(menu);
    exec_menu_from_view(menu, view, pos);
}

void FindResults::load(const QString &filter, const QString &search_base) {
    show_busy_indicator();
    
    object_count_label->clear();

    model->removeRows(0, model->rowCount());
    
    const QList<QString> search_attributes = ADCONFIG()->get_columns();
    const QHash<QString, AdObject> search_results = AD()->search(filter, search_attributes, SearchScope_All, search_base);


    for (const AdObject object : search_results) {
        if (model->rowCount() == RESULTS_COUNT_MAX) {
            break;
        }

        const QList<QStandardItem *> row = make_item_row(ADCONFIG()->get_columns().count());

        load_object_row(row, object);

        model->appendRow(row);
    }

    view->sortByColumn(ADCONFIG()->get_column_index(ATTRIBUTE_NAME), Qt::AscendingOrder);

    const QString label_text = tr("%n object(s)", "", model->rowCount());
    object_count_label->setText(label_text);

    hide_busy_indicator();

    if (search_results.values().count() > RESULTS_COUNT_MAX) {
        QMessageBox::warning(this, tr("Warning"), "Reached result display limit, refine your filter to reduce result size.");
    }
}

QList<QList<QStandardItem *>> FindResults::get_selected_rows() const {
    const QList<QModelIndex> selected_rows = view->selectionModel()->selectedRows();

    QList<QList<QStandardItem *>> out;

    for (const QModelIndex row_index : selected_rows) {
        const int row = row_index.row();

        QList<QStandardItem *> row_copy;

        for (int col = 0; col < model->columnCount(); col++) {
            QStandardItem *item = model->item(row, col);
            QStandardItem *item_copy = item->clone();
            row_copy.append(item_copy);
        }

        out.append(row_copy);
    }

    return out;
}
