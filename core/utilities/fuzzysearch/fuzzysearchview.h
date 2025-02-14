/* ============================================================
 *
 * This file is a part of digiKam project
 * https://www.digikam.org
 *
 * Date        : 2008-05-19
 * Description : Fuzzy search sidebar tab contents.
 *
 * Copyright (C) 2016-2018 by Mario Frank <mario dot frank at uni minus potsdam dot de>
 * Copyright (C) 2008-2019 by Gilles Caulier <caulier dot gilles at gmail dot com>
 * Copyright (C) 2008-2012 by Marcel Wiesweg <marcel dot wiesweg at gmx dot de>
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

#ifndef DIGIKAM_FUZZY_SEARCH_VIEW_H
#define DIGIKAM_FUZZY_SEARCH_VIEW_H

// Qt includes

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QScrollArea>
#include <QDropEvent>

// Local includes

#include "statesavingobject.h"

class QMimeData;
class QPixmap;

namespace Digikam
{

class Album;
class FuzzySearchFolderView;
class ItemInfo;
class LoadingDescription;
class SAlbum;
class PAlbum;
class TAlbum;
class SearchModel;
class SearchModificationHelper;
class SearchTextBar;

class FuzzySearchView : public QScrollArea, public StateSavingObject
{
    Q_OBJECT

public:

    explicit FuzzySearchView(SearchModel* const searchModel,
                             SearchModificationHelper* const searchModificationHelper,
                             QWidget* const parent = nullptr);
    virtual ~FuzzySearchView();

    SAlbum* currentAlbum() const;
    void setCurrentAlbum(SAlbum* const album);

    void setActive(bool val);
    void setItemInfo(const ItemInfo& info);

    void newDuplicatesSearch(PAlbum* const album);
    void newDuplicatesSearch(const QList<PAlbum*>& albums);
    void newDuplicatesSearch(const QList<TAlbum*>& albums);

    virtual void setConfigGroup(const KConfigGroup& group);
    void doLoadState();
    void doSaveState();

protected:

    void dragEnterEvent(QDragEnterEvent* e);
    void dragMoveEvent(QDragMoveEvent* e);
    void dropEvent(QDropEvent* e);

private Q_SLOTS:

    void slotTabChanged(int);

    void slotHSChanged(int h, int s);
    void slotVChanged(int v);
    void slotPenColorChanged(const QColor&);
    void slotClearSketch();
    void slotSaveSketchSAlbum();
    void slotCheckNameEditSketchConditions();

    void slotAlbumSelected(Album* album);

    void slotSaveImageSAlbum();
    void slotCheckNameEditImageConditions();
    void slotThumbnailLoaded(const LoadingDescription&, const QPixmap&);

    void slotDirtySketch();
    void slotTimerSketchDone();
    void slotUndoRedoStateChanged(bool, bool);

    void slotMinLevelImageChanged(int);
    void slotMaxLevelImageChanged(int);
    void slotFuzzyAlbumsChanged();
    void slotTimerImageDone();

    void slotApplicationSettingsChanged();

private:

    void setCurrentImage(qlonglong imageid);
    void setCurrentImage(const ItemInfo& info);

    void createNewFuzzySearchAlbumFromSketch(const QString& name, bool force = false);
    void createNewFuzzySearchAlbumFromImage(const QString& name, bool force = false);
    bool dragEventWrapper(const QMimeData* data) const;

    void setColor(QColor c);

    QWidget* setupFindSimilarPanel() const;
    QWidget* setupSketchPanel()      const;
    void     setupConnections();

private:

    class Private;
    Private* const d;
};

} // namespace Digikam

#endif // DIGIKAM_FUZZY_SEARCH_VIEW_H
