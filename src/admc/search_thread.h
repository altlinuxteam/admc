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

#ifndef SEARCH_THREAD_H
#define SEARCH_THREAD_H

/**
 * A thread that performs an AD search operation. Useful for
 * searches that are expected to take a long time. For
 * regular small searches this is overkill. results_ready()
 * signal returns search results as they arrive. If search
 * has multiple pages, then results_ready() will be emitted
 * multiple times. Use stop() to stop search. Note that search is
 * not stopped immediately but when current results page is
 * done processing. SearchThread deletes itself when it's
 * finished.
 */

#include <QThread>

class AdObject;
template <typename T> class QList;
template <typename K, typename V> class QHash;

class SearchThread final : public QThread
{
    Q_OBJECT

public:
    SearchThread(const QString &filter_arg, const QString search_base_arg, const QList<QString> attrs_arg);

    void stop();

signals:
    void results_ready(const QHash<QString, AdObject> &results);

private:
    QString filter;
    QString search_base;
    QList<QString> attrs;
    bool stop_flag;

    void run() override;
};

#endif /* SEARCH_THREAD_H */
