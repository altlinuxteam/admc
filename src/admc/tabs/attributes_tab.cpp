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

#include "tabs/attributes_tab.h"
#include "editors/attribute_editor.h"
#include "ad_interface.h"
#include "ad_config.h"
#include "ad_object.h"
#include "utils.h"
#include "attribute_display.h"

#include <QTreeView>
#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QMessageBox>
#include <QPushButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QFrame>

enum AttributesColumn {
    AttributesColumn_Name,
    AttributesColumn_Value,
    AttributesColumn_Type,
    AttributesColumn_COUNT,
};

QString attribute_type_display_string(const AttributeType type);

AttributesTab::AttributesTab() {
    model = new QStandardItemModel(0, AttributesColumn_COUNT, this);
    set_horizontal_header_labels_from_map(model, {
        {AttributesColumn_Name, tr("Name")},
        {AttributesColumn_Value, tr("Value")},
        {AttributesColumn_Type, tr("Type")}
    });

    view = new QTreeView(this);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setAllColumnsShowFocus(true);
    view->setSortingEnabled(true);

    proxy = new AttributesTabProxy(this);

    proxy->setSourceModel(model);
    view->setModel(proxy);

    auto filter_button = new QPushButton(tr("Filter"));

    const auto layout = new QVBoxLayout();
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(view);
    layout->addWidget(filter_button);

    connect(
        view, &QAbstractItemView::doubleClicked,
        this, &AttributesTab::on_double_clicked);
    connect(
        filter_button, &QAbstractButton::clicked,
        this, &AttributesTab::open_filter_dialog);
}

void AttributesTab::on_double_clicked(const QModelIndex &proxy_index) {
    const QModelIndex index = proxy->mapToSource(proxy_index);
    
    const QList<QStandardItem *> row =
    [this, index]() {
        QList<QStandardItem *> out;
        for (int col = 0; col < AttributesColumn_COUNT; col++) {
            const QModelIndex item_index = index.siblingAtColumn(col);
            QStandardItem *item = model->itemFromIndex(item_index);
            out.append(item);
        }
        return out;
    }();

    const QString attribute = row[AttributesColumn_Name]->text();
    const QList<QByteArray> values = current[attribute];

    AttributeEditor *editor = AttributeEditor::make(attribute, values, this);
    if (editor != nullptr) {
        connect(
            editor, &QDialog::accepted,
            [this, editor, attribute, row]() {
                const QList<QByteArray> new_values = editor->get_new_values();

                current[attribute] = new_values;
                load_row(row, attribute, new_values);

                emit edited();
            });

        editor->open();
    } else {
        QMessageBox::critical(this, tr("Error"), tr("No editor is available for this attribute type."));
    }
}

void AttributesTab::open_filter_dialog() {
    auto dialog = new QDialog();

    auto layout = new QVBoxLayout();
    dialog->setLayout(layout);

    QHash<AttributeFilter, QCheckBox *> checks;

    auto make_frame =
    []() -> QFrame * {
        auto frame = new QFrame();
        frame->setFrameShape(QFrame::Box);
        frame->setFrameShadow(QFrame::Raised);
        frame->setLayout(new QVBoxLayout());

        return frame;
    };

    auto add_check =
    [this, &checks](const QString text, const AttributeFilter filter) {
        auto check = new QCheckBox(text);
        const bool is_checked = proxy->filters[filter];
        check->setChecked(is_checked);

        checks.insert(filter, check);
    };

    add_check(tr("Unset"), AttributeFilter_Unset);
    add_check(tr("System only"), AttributeFilter_SystemOnly);
    add_check(tr("Mandatory"), AttributeFilter_Mandatory);
    add_check(tr("Optional"), AttributeFilter_Optional);

    auto first_frame = make_frame();
    first_frame->layout()->addWidget(checks[AttributeFilter_Unset]);
    first_frame->layout()->addWidget(checks[AttributeFilter_SystemOnly]);

    auto second_frame = make_frame();
    second_frame->layout()->addWidget(checks[AttributeFilter_Mandatory]);
    second_frame->layout()->addWidget(checks[AttributeFilter_Optional]);

    layout->addWidget(first_frame);
    layout->addWidget(second_frame);

    auto button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(
        button_box, &QDialogButtonBox::accepted,
        dialog, &QDialog::accept);
    connect(
        button_box, &QDialogButtonBox::rejected,
        dialog, &QDialog::reject);

    layout->addWidget(button_box);

    connect(
        dialog, &QDialog::accepted,
        [this, checks]() {
            for (const AttributeFilter filter : checks.keys()) {
                const QCheckBox *check = checks[filter];
                proxy->filters[filter] = check->isChecked();
            }

            proxy->invalidate();
        });

    dialog->open();
}

void AttributesTab::showEvent(QShowEvent *event) {
    resize_columns(view,
    {
        {AttributesColumn_Name, 0.4},
        {AttributesColumn_Value, 0.8},
    });
}

void AttributesTab::load(const AdObject &object) {
    original.clear();

    for (auto attribute : object.attributes()) {
        original[attribute] = object.get_values(attribute);
    }

    // Add attributes without values
    const QList<QString> object_classes = object.get_strings(ATTRIBUTE_OBJECT_CLASS);
    const QList<QString> optional_attributes = ADCONFIG()->get_optional_attributes(object_classes);
    for (const QString attribute : optional_attributes) {
        if (!original.contains(attribute)) {
            original[attribute] = QList<QByteArray>();
        }
    }

    current = original;

    proxy->load(object);

    model->removeRows(0, model->rowCount());

    for (auto attribute : original.keys()) {
        const QList<QStandardItem *> row = make_item_row(AttributesColumn_COUNT);
        const QList<QByteArray> values = original[attribute];

        model->appendRow(row);
        load_row(row, attribute, values);
    }

    view->sortByColumn(AttributesColumn_Name, Qt::AscendingOrder);
}

void AttributesTab::apply(const QString &target) const {
    for (const QString &attribute : current.keys()) {
        const QList<QByteArray> current_values = current[attribute];
        const QList<QByteArray> original_values = original[attribute];

        if (current_values != original_values) {
            AD()->attribute_replace_values(target, attribute, current_values);
        }
    }
}

void AttributesTab::load_row(const QList<QStandardItem *> &row, const QString &attribute, const QList<QByteArray> &values) {
    const QString display_values = attribute_display_values(attribute, values);
    const AttributeType type = ADCONFIG()->get_attribute_type(attribute);
    const QString type_display = attribute_type_display_string(type);

    row[AttributesColumn_Name]->setText(attribute);
    row[AttributesColumn_Value]->setText(display_values);
    row[AttributesColumn_Type]->setText(type_display);
}

AttributesTabProxy::AttributesTabProxy(QObject *parent)
: QSortFilterProxyModel(parent) {
    for (int i = 0; i < AttributeFilter_COUNT; i++) {
        const AttributeFilter filter = (AttributeFilter) i;
        filters[filter] = true;
    }
}

void AttributesTabProxy::load(const AdObject &object) {
    const QList<QString> object_classes = object.get_strings(ATTRIBUTE_OBJECT_CLASS);
    mandatory_attributes = ADCONFIG()->get_mandatory_attributes(object_classes).toSet();
    optional_attributes = ADCONFIG()->get_optional_attributes(object_classes).toSet();

    set_attributes = object.attributes().toSet();
}

bool AttributesTabProxy::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
    auto source = sourceModel();
    const QString attribute = source->index(source_row, AttributesColumn_Name, source_parent).data().toString();
    const bool system_only = ADCONFIG()->get_attribute_is_system_only(attribute);
    const bool unset = !set_attributes.contains(attribute);
    const bool mandatory = mandatory_attributes.contains(attribute);
    const bool optional = optional_attributes.contains(attribute);

    if (unset && !filters[AttributeFilter_Unset]) {
        return false;
    }

    if (system_only && !filters[AttributeFilter_SystemOnly]) {
        return false;
    }

    if (mandatory && !filters[AttributeFilter_Mandatory]) {
        return false;
    }

    if (optional && !filters[AttributeFilter_Optional]) {
        return false;
    }
    
    return true;
}

QString attribute_type_display_string(const AttributeType type) {
    switch (type) {
        case AttributeType_Boolean: return AttributesTab::tr("Boolean");
        case AttributeType_Enumeration: return AttributesTab::tr("Enumeration");
        case AttributeType_Integer: return AttributesTab::tr("Integer");
        case AttributeType_LargeInteger: return AttributesTab::tr("Large Integer");
        case AttributeType_StringCase: return AttributesTab::tr("String Case");
        case AttributeType_IA5: return AttributesTab::tr("IA5");
        case AttributeType_NTSecDesc: return AttributesTab::tr("NT Security Descriptor");
        case AttributeType_Numeric: return AttributesTab::tr("Numeric");
        case AttributeType_ObjectIdentifier: return AttributesTab::tr("Object Identifier");
        case AttributeType_Octet: return AttributesTab::tr("Octet");
        case AttributeType_ReplicaLink: return AttributesTab::tr("Replica Link");
        case AttributeType_Printable: return AttributesTab::tr("Printable");
        case AttributeType_Sid: return AttributesTab::tr("SID");
        case AttributeType_Teletex: return AttributesTab::tr("Teletex");
        case AttributeType_Unicode: return AttributesTab::tr("Unicode");
        case AttributeType_UTCTime: return AttributesTab::tr("UTC Time");
        case AttributeType_GeneralizedTime: return AttributesTab::tr("Generalized Time");
        case AttributeType_DNString: return AttributesTab::tr("DN String");
        case AttributeType_DNBinary: return AttributesTab::tr("DN Binary");
        case AttributeType_DSDN: return AttributesTab::tr("Distinguished Name");
    }
    return QString();
}
