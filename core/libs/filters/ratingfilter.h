/* ============================================================
 *
 * This file is a part of digiKam project
 * https://www.digikam.org
 *
 * Date        : 2007-10-09
 * Description : a widget to filter album contents by rating
 *
 * Copyright (C) 2007-2019 by Gilles Caulier <caulier dot gilles at gmail dot com>
 * Copyright (C) 2007      by Arnd Baecker <arnd dot baecker at web dot de>
 * Copyright (C) 2014      by Mohamed_Anwer <m_dot_anwer at gmx dot com>
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

#ifndef DIGIKAM_RATING_FILTER_H
#define DIGIKAM_RATING_FILTER_H

// Local includes

#include "dlayoutbox.h"
#include "itemfiltersettings.h"
#include "ratingwidget.h"

class QAction;

namespace Digikam
{

class RatingFilterWidget : public RatingWidget
{
    Q_OBJECT

public:

    explicit RatingFilterWidget(QWidget* const parent);
    ~RatingFilterWidget();

    void setRatingFilterCondition(ItemFilterSettings::RatingCondition cond);
    ItemFilterSettings::RatingCondition ratingFilterCondition();

    void setExcludeUnratedItems(bool excluded);
    bool isUnratedItemsExcluded();

Q_SIGNALS:

    void signalRatingFilterChanged(int, ItemFilterSettings::RatingCondition, bool);

protected:

    void mousePressEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);

private:

    void updateRatingTooltip();

private Q_SLOTS:

    void slotRatingChanged();

private:

    class Private;
    Private* const d;
};

// -----------------------------------------------------------------------------

class RatingFilter : public DHBox
{
    Q_OBJECT

public:

    explicit RatingFilter(QWidget* const parent);
    ~RatingFilter();

    void setRating(int val);
    int  rating() const;

    void setRatingFilterCondition(ItemFilterSettings::RatingCondition cond);
    ItemFilterSettings::RatingCondition ratingFilterCondition();

    void setExcludeUnratedItems(bool excluded);
    bool isUnratedItemsExcluded();

Q_SIGNALS:

    void signalRatingFilterChanged(int, ItemFilterSettings::RatingCondition, bool);

private Q_SLOTS:

    void slotOptionsMenu();
    void slotOptionsTriggered(QAction*);

private:

    class Private;
    Private* const d;
};

} // namespace Digikam

#endif // DIGIKAM_RATING_FILTER_H
