/* ============================================================
 *
 * This file is a part of digiKam project
 * https://www.digikam.org
 *
 * Date        : 2013-02-25
 * Description : Table view column helpers: Thumbnail column
 *
 * Copyright (C) 2013 by Michael G. Hansen <mike at mghansen dot de>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#include "tableview_column_thumbnail.h"

// Qt includes

#include <QPainter>
#include <QApplication>
#include <QDesktopWidget>
#include <QStyleOptionViewItem>

// KDE includes

#include <klocalizedstring.h>

// Local includes

#include "itemfiltermodel.h"
#include "tableview.h"
#include "thumbnailloadthread.h"
#include "thumbnailsize.h"
#include "metaengine.h"

namespace
{
    const int ThumbnailBorder = 2;
}

namespace Digikam
{

namespace TableViewColumns
{

ColumnThumbnail::ColumnThumbnail(TableViewShared* const tableViewShared,
                                 const TableViewColumnConfiguration& pConfiguration,
                                 QObject* const parent)
    : TableViewColumn(tableViewShared, pConfiguration, parent),
      m_thumbnailSize(s->tableView->getThumbnailSize().size())
{
    connect(s->thumbnailLoadThread, SIGNAL(signalThumbnailLoaded(LoadingDescription,QPixmap)),
            this, SLOT(slotThumbnailLoaded(LoadingDescription,QPixmap)));
}

ColumnThumbnail::~ColumnThumbnail()
{
}

bool ColumnThumbnail::CreateFromConfiguration(TableViewShared* const tableViewShared,
                                              const TableViewColumnConfiguration& pConfiguration,
                                              TableViewColumn** const pNewColumn,
                                              QObject* const parent)
{
    if (pConfiguration.columnId!=QLatin1String("thumbnail"))
    {
        return false;
    }

    *pNewColumn = new ColumnThumbnail(tableViewShared, pConfiguration, parent);

    return true;
}

TableViewColumnDescription ColumnThumbnail::getDescription()
{
    return TableViewColumnDescription(QLatin1String("thumbnail"), i18n("Thumbnail"));
}

TableViewColumn::ColumnFlags ColumnThumbnail::getColumnFlags() const
{
    return ColumnCustomPainting;
}

QString ColumnThumbnail::getTitle() const
{
    return i18n("Thumbnail");
}

QVariant ColumnThumbnail::data(TableViewModel::Item* const item, const int role) const
{
    Q_UNUSED(item)
    Q_UNUSED(role)

    // we do not return any data, but paint(...) something
    return QVariant();
}

bool ColumnThumbnail::paint(QPainter* const painter, const QStyleOptionViewItem& option, TableViewModel::Item* const item) const
{
    if (option.state & QStyle::State_Selected)
    {
        painter->fillRect(option.rect, option.palette.highlight());
    }

    const ItemInfo info = s->tableViewModel->infoFromItem(item);

    if (!info.isNull())
    {
        QSize availableSize = option.rect.size() - QSize(ThumbnailBorder, ThumbnailBorder);
        QSize imageSize     = info.dimensions();
        int maxSize         = m_thumbnailSize;

        // When the image is rotated, swap width and height.
        if (info.orientation() > MetaEngine::ORIENTATION_VFLIP)
        {
            imageSize.transpose();
        }

        if (imageSize.isValid() && (imageSize.width() > imageSize.height()))
        {
            // for landscape pictures, try to use all available horizontal space
            qreal scaleFactor = qreal(availableSize.height()) / qreal(imageSize.height());
            maxSize           = imageSize.width() * scaleFactor;
        }

        // Calculate the maximum thumbnail size
        // The idea here is that for landscape images, we adjust the height to
        // to be as high as the row height as long as the width can stretch enough
        // because the column is wider than the thumbnail size.
        maxSize       = qMin(maxSize, availableSize.width());

        // However, digiKam limits the thumbnail size, so we also do that here
        maxSize       = qMin(maxSize, (int)ThumbnailSize::maxThumbsSize());
        double ratio  = QApplication::desktop()->devicePixelRatioF();
        maxSize       = qRound((double)maxSize * ratio);

        QPixmap thumbnail;

        if (s->thumbnailLoadThread->find(info.thumbnailIdentifier(), thumbnail, maxSize))
        {
            thumbnail.setDevicePixelRatio(ratio);
            /// @todo Is slotThumbnailLoaded still called when the thumbnail is found right away?
            QSize pixmapSize = (QSizeF(thumbnail.size()) / ratio).toSize();
            pixmapSize       = pixmapSize.boundedTo(availableSize);
            QSize alignSize  = option.rect.size();

            QPoint startPoint((alignSize.width()  - pixmapSize.width())  / 2,
                              (alignSize.height() - pixmapSize.height()) / 2);
            startPoint            += option.rect.topLeft();

            painter->drawPixmap(QRect(startPoint, pixmapSize), thumbnail,
                                QRect(QPoint(0, 0), thumbnail.size()));

            return true;
        }
    }

    // we did not get to paint a thumbnail...
    return false;
}

QSize ColumnThumbnail::sizeHint(const QStyleOptionViewItem& option, TableViewModel::Item* const item) const
{
    Q_UNUSED(option)
    Q_UNUSED(item)

    /// @todo On portrait pictures, the borders are too close. There should be a gap. Is this setting okay?
    const int thumbnailSizeWithBorder = m_thumbnailSize+ThumbnailBorder;
    return QSize(thumbnailSizeWithBorder, thumbnailSizeWithBorder);
}

void ColumnThumbnail::slotThumbnailLoaded(const LoadingDescription& loadingDescription, const QPixmap& thumb)
{
    if (thumb.isNull())
    {
        return;
    }

    /// @todo Find a way to do this without the ItemFilterModel
    const QModelIndex imageModelIndex = s->imageModel->indexForPath(loadingDescription.filePath);

    if (!imageModelIndex.isValid())
    {
        return;
    }

    const qlonglong imageId = s->imageModel->imageId(imageModelIndex);

    emit(signalDataChanged(imageId));
}

void ColumnThumbnail::updateThumbnailSize()
{
    /// @todo Set minimum column width to m_thumbnailSize

    m_thumbnailSize = s->tableView->getThumbnailSize().size();

    emit(signalAllDataChanged());
}

} // namespace TableViewColumns

} // namespace Digikam
