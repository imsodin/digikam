/* ============================================================
 *
 * This file is a part of digiKam project
 * https://www.digikam.org
 *
 * Date        : 2014-05-28
 * Description : overlay for GPS location indicator
 *
 * Copyright (C) 2014-2019 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#ifndef DIGIKAM_ITEM_COORDINATES_OVERLAY_H
#define DIGIKAM_ITEM_COORDINATES_OVERLAY_H

// Qt includes

#include <QAbstractButton>
#include <QAbstractItemView>

// Local includes

#include "itemviewhoverbutton.h"
#include "itemdelegateoverlay.h"
#include "itemviewdelegate.h"

namespace Digikam
{

class CoordinatesOverlayWidget : public QAbstractButton
{
    Q_OBJECT

public:

    explicit CoordinatesOverlayWidget(QWidget* const parent = nullptr);

protected:

    virtual void paintEvent(QPaintEvent*);
};

// ----------------------------------------------------------------------

class ItemCoordinatesOverlay : public AbstractWidgetDelegateOverlay
{
    Q_OBJECT
    REQUIRE_DELEGATE(ItemViewDelegate)

public:

    explicit ItemCoordinatesOverlay(QObject* const parent);
    CoordinatesOverlayWidget* buttonWidget() const;

protected:

    void updatePosition();

    virtual QWidget* createWidget();
    virtual void setActive(bool active);
    virtual void visualChange();
    virtual bool checkIndex(const QModelIndex& index) const;
    virtual void slotEntered(const QModelIndex& index);

protected:

    QPersistentModelIndex m_index;
};

} // namespace Digikam

#endif // DIGIKAM_ITEM_COORDINATES_OVERLAY_H
