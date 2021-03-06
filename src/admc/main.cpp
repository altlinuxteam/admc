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

#include "config.h"
#include "main_window.h"
#include "settings.h"
#include "ad_object.h"

#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>

int main(int argc, char **argv) {
    qRegisterMetaTypeStreamOperators<AdObject>("AdObject");

    // NOTE: this is needed to pass this type from thread's
    // signal in find_widget.cpp. Without doing this,
    // passing this type from thread results in a runtime
    // error.
    qRegisterMetaType<QHash<QString, AdObject>>("QHash<QString, AdObject>");

    QApplication app(argc, argv);
    app.setApplicationDisplayName(ADMC_APPLICATION_DISPLAY_NAME);
    app.setApplicationName(ADMC_APPLICATION_NAME);
    app.setApplicationVersion(ADMC_VERSION);
    app.setOrganizationName(ADMC_ORGANIZATION);
    app.setOrganizationDomain(ADMC_ORGANIZATION_DOMAIN);

    QTranslator translator;
    const QLocale saved_locale = SETTINGS()->get_variant(VariantSetting_Locale).toLocale();
    translator.load(saved_locale, QString(), QString(), ":/translations");
    app.installTranslator(&translator);

    // NOTE: these translations are for qt-defined text, like standard dialog buttons
    QTranslator qt_translator;
    qt_translator.load("qt_" + saved_locale.name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qt_translator);

    MainWindow main_window;
    main_window.show();

    const int retval = app.exec();

    return retval;
}
