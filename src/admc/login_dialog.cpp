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

#include "login_dialog.h"
#include "ad_interface.h"
#include "settings.h"
#include "utils.h"

#include <QLineEdit>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QMessageBox>
#include <QString>
#include <QGridLayout>

LoginDialog::LoginDialog(QWidget *parent)
: QDialog(parent)
{
    resize(500, 300);

    const auto label = new QLabel(tr("Login dialog"), this);

    const auto domain_edit_label = new QLabel(tr("Domain: "), this);
    domain_edit = new QLineEdit(this);

    const auto site_edit_label = new QLabel(tr("Site: "), this);
    site_edit = new QLineEdit(this);

    const auto login_button = new QPushButton(tr("Login"), this);
    const auto cancel_button = new QPushButton(tr("Cancel"), this);
    login_button->setAutoDefault(false);
    cancel_button->setAutoDefault(false);

    const auto hosts_list_label = new QLabel(tr("Select host:"), this);
    hosts_list = new QListWidget(this);
    hosts_list->setEditTriggers(QAbstractItemView::NoEditTriggers);

    const auto layout = new QGridLayout(this);
    layout->addWidget(label, 0, 0);
    layout->addWidget(domain_edit_label, 1, 0);
    layout->addWidget(domain_edit, 1, 1);
    layout->addWidget(site_edit_label, 1, 3);
    layout->addWidget(site_edit, 1, 4);
    layout->addWidget(hosts_list_label, 3, 0);
    layout->addWidget(hosts_list, 4, 0, 1, 5);
    layout->addWidget(cancel_button, 5, 0, Qt::AlignLeft);
    layout->addWidget(login_button, 5, 4, Qt::AlignRight);

    connect(
        domain_edit, &QLineEdit::editingFinished,
        this, &LoginDialog::load_hosts);
    connect(
        site_edit, &QLineEdit::editingFinished,
        this, &LoginDialog::load_hosts);
    connect(
        hosts_list, &QListWidget::itemDoubleClicked,
        this, &LoginDialog::on_host_double_clicked);
    connect(
        login_button, &QAbstractButton::clicked,
        this, &LoginDialog::on_login_button);
    connect(
        cancel_button, &QAbstractButton::clicked,
        this, &LoginDialog::on_cancel_button);
    connect(
        this, &QDialog::finished,
        this, &LoginDialog::on_finished);
}

void LoginDialog::open() {
    // Load session values from settings
    const QString domain = Settings::instance()->get_variant(VariantSetting_Domain).toString();
    const QString site = Settings::instance()->get_variant(VariantSetting_Site).toString();
    const QString host = Settings::instance()->get_variant(VariantSetting_Host).toString();
    
    domain_edit->setText(domain);
    site_edit->setText(site);

    load_hosts();

    // Select saved session host in hosts list
    QList<QListWidgetItem *> found_hosts = hosts_list->findItems(host, Qt::MatchExactly);
    if (!found_hosts.isEmpty()) {
        QListWidgetItem *host_item = found_hosts[0];
        hosts_list->setCurrentItem(host_item);
    }

    QDialog::open();

    if (found_hosts.isEmpty() && host != "") {
        QMessageBox::warning(this, tr("Warning"), tr("Failed to find saved session's host"));
    }
}

void LoginDialog::on_host_double_clicked(QListWidgetItem *item) {
    const QString host = item->text();

    complete(host);
}

void LoginDialog::on_login_button(bool) {
    // NOTE: listwidget has to have focus to properly return current item...
    hosts_list->setFocus();
    QListWidgetItem *current_item = hosts_list->currentItem();

    if (current_item == nullptr) {
        QMessageBox::warning(this, tr("Error"), tr("Need to select a host to login."));
    } else {
        const QString host = current_item->text();

        complete(host);
    } 
}

void LoginDialog::on_cancel_button(bool) {
    done(QDialog::Rejected);
}

void LoginDialog::on_finished() {
    hosts_list->clear();
}

void LoginDialog::load_hosts() {
    const QString domain = domain_edit->text();
    const QString site = site_edit->text();

    QList<QString> hosts = get_domain_hosts(domain, site);

    hosts_list->clear();
    for (auto h : hosts) {
        new QListWidgetItem(h, hosts_list);
    }
}

void LoginDialog::complete(const QString &host) {
    const QString domain = domain_edit->text();

    const bool login_success = AdInterface::instance()->login(host, domain);

    if (login_success) {
        const QString site = site_edit->text();
        Settings::instance()->set_variant(VariantSetting_Domain, domain);
        Settings::instance()->set_variant(VariantSetting_Site, site);
        Settings::instance()->set_variant(VariantSetting_Host, host);

        accept();
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Failed to login!"));
    }
}
