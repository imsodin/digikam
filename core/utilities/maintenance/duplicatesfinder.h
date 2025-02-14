/* ============================================================
 *
 * This file is a part of digiKam project
 * https://www.digikam.org
 *
 * Date        : 2012-01-20
 * Description : Duplicates items finder.
 *
 * Copyright (C) 2012-2019 by Gilles Caulier <caulier dot gilles at gmail dot com>
 * Copyright (C) 2015      by Mohamed_Anwer <m_dot_anwer at gmx dot com>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#ifndef DIGIKAM_DUPLICATES_FINDER_H
#define DIGIKAM_DUPLICATES_FINDER_H

// Qt includes

#include <QString>
#include <QObject>

// Local includes

#include "album.h"
#include "maintenancetool.h"

namespace Digikam
{

class DuplicatesFinder : public MaintenanceTool
{
    Q_OBJECT

public:

    /** Version to find all duplicates in the set of images
     */
    explicit DuplicatesFinder(const QList<qlonglong>& imageIds,
                              int minSimilarity = 90,
                              int maxSimilarity = 100,
                              int searchResultRestriction = 0,
                              ProgressItem* const parent = nullptr);

    /** Version to find all duplicates over a specific list to PAlbums and TAlbums
     */
    DuplicatesFinder(const AlbumList& albums,
                     const AlbumList& tags,
                     int albumTagRelation = 0,
                     int minSimilarity = 90,
                     int maxSimilarity = 100,
                     int searchResultRestriction = 0,
                     ProgressItem* const parent = nullptr);

    /** Version to find all duplicates over whole collections
     */
    explicit DuplicatesFinder(const int minSimilarity = 90,
                              int maxSimilarity = 100,
                              int searchResultRestriction = 0,
                              ProgressItem* const parent = nullptr);
    ~DuplicatesFinder();

private Q_SLOTS:

    void slotStart();
    void slotDone();
    void slotCancel();
    void slotDuplicatesSearchTotalAmount(int);
    void slotDuplicatesSearchProcessedAmount(int);

private:

    class Private;
    Private* const d;
};

} // namespace Digikam

#endif // DIGIKAM_DUPLICATES_FINDER_H
