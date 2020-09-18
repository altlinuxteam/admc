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

#include "gplink_tab.h"
#include "object_context_menu.h"
#include "utils.h"
#include "dn_column_proxy.h"
#include "select_dialog.h"

#include <QTreeView>
#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QMenu>
#include <QPushButton>
#include <QDebug>

#define LDAP_PREFIX "LDAP://"

const QHash<GplinkOption, int> gplink_option_to_int_map = {
    {GplinkOption_None,     0},
    {GplinkOption_Disable,  1},
    {GplinkOption_Enforce,  2},
    {GplinkOption_COUNT,    -1},
};

enum GplinkColumn {
    GplinkColumn_Name,
    GplinkColumn_Option,
    GplinkColumn_DN,
    GplinkColumn_COUNT
};

GplinkOption gplink_option_from_string(const QString &option_string) {
    const int option_int = option_string.toInt();
    const QList<GplinkOption> keys = gplink_option_to_int_map.keys(option_int);
    const GplinkOption option = keys[0];

    return option;
};

QString gplink_option_to_string(const GplinkOption option) {
    const int option_int = gplink_option_to_int_map[option];
    const QString option_string = QString::number(option_int);

    return option_string;
}

QString gplink_option_to_display_string(const GplinkOption option) {
    switch (option) {
        case GplinkOption_None: return QObject::tr("None");
        case GplinkOption_Disable: return QObject::tr("Disable");
        case GplinkOption_Enforce: return QObject::tr("Enforce");
        case GplinkOption_COUNT: {};
    }

    return QObject::tr("UNKNOWN OPTION");
};

// NOTE: DN == GPO. But sticking to GPO terminology in this case

// TODO: confirm that input gplink is valid. Do sanity checks?

Gplink::Gplink() {

}

Gplink::Gplink(const QString &gplink_string) {
    if (gplink_string.isEmpty()) {
        return;
    }

    // "[gpo_1;option_1][gpo_2;option_2][gpo_3;option_3]..."
    // =>
    // {"gpo_1;option_1", "gpo_2;option_2", "gpo_3;option_3"}
    QString gplink_string_without_brackets = gplink_string;
    gplink_string_without_brackets.replace("[", "");
    const QList<QString> gplink_string_split = gplink_string_without_brackets.split(']');

    for (auto part : gplink_string_split) {
        if (part.isEmpty()) {
            continue;
        }

        // "gpo;option"
        // =>
        // gpo and option
        const QList<QString> part_split = part.split(';');
        QString gpo = part_split[0];
        gpo.replace(LDAP_PREFIX, "", Qt::CaseSensitive);
        const QString option_string = part_split[1];
        const GplinkOption option = gplink_option_from_string(option_string);

        gpos_in_order.append(gpo);
        options[gpo] = option;
    }
}

QString Gplink::to_string() const {
    QString gplink_string;

    for (auto gpo : gpos_in_order) {
        const GplinkOption option = options[gpo];
        const QString option_string = gplink_option_to_string(option);

        const QString part = QString("[%1%2;%3]").arg(LDAP_PREFIX, gpo, option_string);
        
        gplink_string += part;
    }

    return gplink_string;
}

bool Gplink::equals(const Gplink &other) const {
    return ((gpos_in_order == other.gpos_in_order) && (options == other.options));
}

void Gplink::add(const QString &gpo) {
    gpos_in_order.append(gpo);
    options[gpo] = GplinkOption_None;
}

void Gplink::remove(const QString &gpo) {
    gpos_in_order.removeAll(gpo);
    options.remove(gpo);
}

void Gplink::move_up(const QString &gpo) {
    const int current_index = gpos_in_order.indexOf(gpo);

    if (current_index > 0) {
        const int new_index = current_index - 1;
        gpos_in_order.move(current_index, new_index);
    }
}

void Gplink::move_down(const QString &gpo) {
    const int current_index = gpos_in_order.indexOf(gpo);
    if (current_index < gpos_in_order.size() - 1) {
        const int new_index = current_index + 1;

        gpos_in_order.move(current_index, new_index);
    }
}

GplinkTab::GplinkTab(DetailsWidget *details_arg)
: DetailsTab(details_arg)
{   
    view = new QTreeView(this);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    view->setAllColumnsShowFocus(true);

    model = new QStandardItemModel(0, GplinkColumn_COUNT, this);
    model->setHorizontalHeaderItem(GplinkColumn_Name, new QStandardItem(tr("Name")));
    model->setHorizontalHeaderItem(GplinkColumn_Option, new QStandardItem(tr("Option")));
    model->setHorizontalHeaderItem(GplinkColumn_DN, new QStandardItem(tr("DN")));

    const auto dn_column_proxy = new DnColumnProxy(GplinkColumn_DN, this);

    setup_model_chain(view, model, {dn_column_proxy});

    auto add_button = new QPushButton(tr("Add"));
    auto remove_button = new QPushButton(tr("Remove"));
    auto button_layout = new QHBoxLayout();
    button_layout->addWidget(add_button);
    button_layout->addWidget(remove_button);

    const auto layout = new QVBoxLayout();
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(view);
    layout->addLayout(button_layout);

    connect(
        remove_button, &QAbstractButton::clicked,
        this, &GplinkTab::on_remove_button);
    connect(
        add_button, &QAbstractButton::clicked,
        this, &GplinkTab::on_add_button);
    QObject::connect(
        view, &QWidget::customContextMenuRequested,
        this, &GplinkTab::on_context_menu);
}

bool GplinkTab::changed() const {
    return !current_gplink.equals(original_gplink);
}

bool GplinkTab::verify() {
    return true;
}

void GplinkTab::apply() {
    const QString gplink_string = current_gplink.to_string();
    AdInterface::instance()->attribute_replace(target(), ATTRIBUTE_GPLINK, gplink_string);
}

void GplinkTab::reload() {
    const QString gplink_string = AdInterface::instance()->attribute_get(target(), ATTRIBUTE_GPLINK);
    original_gplink = Gplink(gplink_string);
    current_gplink = original_gplink;

    edited();
}

// TODO: not sure which object classes can have gplink, for now only know of OU's.
bool GplinkTab::accepts_target() const {
    const bool is_ou = AdInterface::instance()->is_ou(target());

    return is_ou;
}

// TODO: similar to code in ObjectContextMenu
void GplinkTab::on_context_menu(const QPoint pos) {
    const QModelIndex base_index = view->indexAt(pos);
    if (!base_index.isValid()) {
        return;
    }
    const QModelIndex index = convert_to_source(base_index);
    const QString gpo = get_dn_from_index(index, GplinkColumn_DN);

    const QPoint global_pos = view->mapToGlobal(pos);

    auto menu = new QMenu(this);
    menu->addAction(tr("Remove link"), [this, gpo]() {
        const QList<QString> removed = {gpo};
        remove(removed);
        edited();
    });
    menu->addAction(tr("Move up"), [this, gpo]() {
        current_gplink.move_up(gpo);
        edited();
    });
    menu->addAction(tr("Move down"), [this, gpo]() {
        current_gplink.move_down(gpo);
        edited();
    });

    auto option_submenu = menu->addMenu(tr("Set option"));
    for (int i = 0; i < GplinkOption_COUNT; i++) {
        const GplinkOption option = (GplinkOption) i;
        const QString option_display_string = gplink_option_to_display_string(option);
        option_submenu->addAction(option_display_string, [this, gpo, option]() {
            current_gplink.options[gpo] = option;
            edited();
        });
    }
    
    menu->popup(global_pos);
}

void GplinkTab::on_add_button() {
    const QList<QString> classes = {CLASS_GP_CONTAINER};
    const QList<QString> selected = SelectDialog::open(classes, SelectDialogMultiSelection_Yes);

    if (selected.size() > 0) {
        add(selected);
    }
}

void GplinkTab::on_remove_button() {
    const QItemSelectionModel *selection_model = view->selectionModel();
    const QList<QModelIndex> selected_raw = selection_model->selectedIndexes();

    QList<QString> selected;
    for (auto index : selected_raw) {
        const QModelIndex converted = convert_to_source(index);
        const QString gpo = get_dn_from_index(converted, GplinkColumn_DN);

        selected.append(gpo);
    }

    remove(selected);    
}

void GplinkTab::add(QList<QString> gps) {
    for (auto gp : gps) {
        current_gplink.add(gp);
    }

    edited();
}

void GplinkTab::remove(QList<QString> gps) {
    for (auto gp : gps) {
        current_gplink.remove(gp);
    }

    edited();
}

// TODO: members tab needs this as well. DetailsTab::on_edit_changed() slot is weird in general. Also, idk if "edited" is a good name, sounds like a getter for a bool.
void GplinkTab::edited() {
    model->removeRows(0, model->rowCount());

    for (auto gpo : current_gplink.gpos_in_order) {
        const GplinkOption option = current_gplink.options[gpo];
        const QString option_display_string = gplink_option_to_display_string(option);
        const QString name = AdInterface::instance()->get_name_for_display(gpo);

        const QList<QStandardItem *> row = make_item_row(GplinkColumn_COUNT);
        row[GplinkColumn_Name]->setText(name);
        row[GplinkColumn_Option]->setText(option_display_string);
        row[GplinkColumn_DN]->setText(gpo);

        model->appendRow(row);
    }

    on_edit_changed();
}
