/* ============================================================
 *
 * This file is a part of digiKam project
 * https://www.digikam.org
 *
 * Date        : 2017-05-06
 * Description : interface to database information for shared tools.
 *
 * Copyright (C) 2017-2019 by Gilles Caulier <caulier dot gilles at gmail dot com>
 * Copyright (C) 2017      by Mario Frank <mario dot frank at uni minus potsdam dot de>
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

#include "dbinfoiface.h"

// Qt includes

#include <QDir>

// Local includes

#include "albumselecttabs.h"
#include "applicationsettings.h"
#include "album.h"
#include "albummodel.h"
#include "albummanager.h"
#include "albumfiltermodel.h"
#include "albumselectwidget.h"
#include "coredb.h"
#include "collectionmanager.h"
#include "coredbnamefilter.h"
#include "collectionscanner.h"
#include "digikamapp.h"
#include "digikam_debug.h"
#include "itemattributeswatch.h"
#include "itemiconview.h"
#include "itemcopyright.h"
#include "iteminfo.h"
#include "photoinfocontainer.h"
#include "videoinfocontainer.h"
#include "itemposition.h"
#include "itemsortsettings.h"
#include "coredbsearchxml.h"
#include "itemlister.h"
#include "itemlisterreceiver.h"
#include "dio.h"

namespace Digikam
{

class Q_DECL_HIDDEN DBInfoIface::Private
{
public:

    explicit Private()
      : albumManager(AlbumManager::instance()),
        albumsChooser(nullptr),
        albumSelector(nullptr),
        operationType(ApplicationSettings::Unspecified),
        withGroupedIsSet(false),
        withGrouped(false)
    {
    }

    AlbumManager*                      albumManager;
    AlbumSelectTabs*                   albumsChooser;
    AlbumSelectWidget*                 albumSelector;

    QList<QUrl>                        itemUrls;

    ApplicationSettings::OperationType operationType;
    bool                               withGroupedIsSet;
    bool                               withGrouped;

public:

    /** get the images from the Physical album in database and return the items found.
     */
    QList<QUrl> imagesFromPAlbum(PAlbum* const album) const
    {
        // get the images from the database and return the items found

        CoreDB::ItemSortOrder sortOrder = CoreDB::NoItemSorting;

        switch (ApplicationSettings::instance()->getImageSortOrder())
        {
            default:
            case ItemSortSettings::SortByFileName:
                sortOrder = CoreDB::ByItemName;
                break;

            case ItemSortSettings::SortByFilePath:
                sortOrder = CoreDB::ByItemPath;
                break;

            case ItemSortSettings::SortByCreationDate:
                sortOrder = CoreDB::ByItemDate;
                break;

            case ItemSortSettings::SortByRating:
                sortOrder = CoreDB::ByItemRating;
                break;
                // ByISize not supported
        }

        QStringList list = CoreDbAccess().db()->getItemURLsInAlbum(album->id(), sortOrder);
        QList<QUrl> urlList;
        CoreDbNameFilter nameFilter(ApplicationSettings::instance()->getAllFileFilter());

        for (QStringList::const_iterator it = list.constBegin() ; it != list.constEnd() ; ++it)
        {
            if (nameFilter.matches(*it))
            {
                urlList << QUrl::fromLocalFile(*it);
            }
        }

        return urlList;
    }

    /** get the images from the Tags album in database and return the items found.
     */
    QList<QUrl> imagesFromTAlbum(TAlbum* const album) const
    {
        QStringList list = CoreDbAccess().db()->getItemURLsInTag(album->id());
        QList<QUrl> urlList;
        CoreDbNameFilter nameFilter(ApplicationSettings::instance()->getAllFileFilter());

        for (QStringList::const_iterator it = list.constBegin() ; it != list.constEnd() ; ++it)
        {
            if (nameFilter.matches(*it))
            {
                urlList << QUrl::fromLocalFile(*it);
            }
        }

        return urlList;
    }

    /** get the images from the search album in database and return the items found.
     */
    QList<QUrl> imagesFromSAlbum(SAlbum* const album) const
    {
        QList<QUrl> urlList;
        CoreDbNameFilter nameFilter(ApplicationSettings::instance()->getAllFileFilter());

        if (album->isDuplicatesSearch())
        {
            // duplicates search album -> get the id list from the query
            SearchXmlReader reader(album->query());
            reader.readToFirstField();

            // Get the defined image ids.
            QList<int> list;
            list << reader.valueToIntList();

            foreach (int imageId, list)
            {
                ItemInfo imageInfo(imageId);
                if (imageInfo.isVisible())
                {
                    // if the image is visible, i.e. existent and not deleted,
                    // add the url if the name filter matches
                    QUrl imageUrl = imageInfo.fileUrl();
/*
                    qCDebug(DIGIKAM_GENERAL_LOG) << "Duplicates search Image url "
                                                 << imageUrl.toString();
*/
                    if (nameFilter.matches(imageUrl.toString()))
                    {
                        urlList << imageUrl;
                    }
                }
            }
        }
        else
        {
            // If we do not have a duplicates search, we use the image lister to get the images.
            ItemLister lister;
            lister.setListOnlyAvailable(true);

            ItemListerValueListReceiver receiver;

            if (album->searchType() == DatabaseSearch::HaarSearch)
            {
                lister.listHaarSearch(&receiver, album->query());
            }
            else
            {
                lister.listSearch(&receiver, album->query(), 0, -1);
            }

            if (!receiver.hasError)
            {
                // if there were no error, fetch and process the results.
                foreach (const ItemListerRecord &record, receiver.records)
                {
                    ItemInfo imageInfo(record);
                    QUrl imageUrl = imageInfo.fileUrl();
/*
                    qCDebug(DIGIKAM_GENERAL_LOG) << "Fuzzy/other search Image url "
                                                 << imageUrl.toString();
*/
                    if (nameFilter.matches(imageUrl.toString()))
                    {
                        urlList << imageUrl;
                    }
                }
            }
        }

        return urlList;
    }

    /** Remove grouped images if user chose/chooses to.
     */
    QList<QUrl> resolveGroupsFromAlbums(const QList<QUrl>& urlList)
    {
        QList<QUrl> lst = urlList;

        if (!(withGroupedIsSet && withGrouped))
        {
            foreach (const QUrl& url, urlList)
            {
                ItemInfo info = ItemInfo::fromUrl(url);

                if (info.hasGroupedImages())
                {
                    if (!withGroupedIsSet)
                    {
                        withGroupedIsSet = true;
                        withGrouped      = ApplicationSettings::instance()->askGroupingOperateOnAll(operationType);

                        if (withGrouped)
                        {
                            break;
                        }
                    }

                    foreach (const ItemInfo& i, info.groupedImages())
                    {
                        lst.removeOne(i.fileUrl());
                    }
                }
            }
        }

        return lst;
    }

    bool includeGroupedFromSelected()
    {
        if (withGroupedIsSet)
        {
            return withGrouped;
        }

        withGroupedIsSet = true;
        withGrouped      = DigikamApp::instance()->view()->selectedNeedGroupResolving(operationType);

        return withGrouped;
    }
};

DBInfoIface::DBInfoIface(QObject* const parent, const QList<QUrl>& lst,
                         const ApplicationSettings::OperationType type)
    : DInfoInterface(parent),
      d(new Private)
{
    setObjectName(QLatin1String("DBInfoIface"));
    d->itemUrls      = lst;
    d->operationType = type;
}

DBInfoIface::~DBInfoIface()
{
    delete d;
}

void DBInfoIface::slotDateTimeForUrl(const QUrl& url, const QDateTime& dt, bool updModDate)
{
    DIO::instance()->slotDateTimeForUrl(url, dt, updModDate);
}

void DBInfoIface::slotMetadataChangedForUrl(const QUrl& url)
{
    // Refresh Database with new metadata from file.
    CollectionScanner scanner;

    scanner.scanFile(url.toLocalFile(), CollectionScanner::Rescan);
    ItemAttributesWatch::instance()->fileMetadataChanged(url);
}

QList<QUrl> DBInfoIface::currentAlbumItems() const
{
    if (!d->itemUrls.isEmpty())
    {
        return d->itemUrls;
    }

    if (d->albumManager->currentAlbums().isEmpty())
    {
        return QList<QUrl>();
    }

    Album* const currAlbum = d->albumManager->currentAlbums().first();
    QList<QUrl> imageList  = d->resolveGroupsFromAlbums(albumItems(currAlbum));

    if (imageList.isEmpty())
    {
        imageList = DigikamApp::instance()->view()->allUrls(d->includeGroupedFromSelected());
    }

    return imageList;
}

QList<QUrl> DBInfoIface::currentSelectedItems() const
{
    if (!d->itemUrls.isEmpty())
    {
        return d->itemUrls;
    }

    return DigikamApp::instance()->view()->selectedUrls(d->includeGroupedFromSelected());
}

QList<QUrl> DBInfoIface::allAlbumItems() const
{
    QList<QUrl> imageList;

    const AlbumList palbumList = d->albumManager->allPAlbums();

    for (AlbumList::ConstIterator it = palbumList.constBegin();
         it != palbumList.constEnd(); ++it)
    {
        // don't add the root album
        if ((*it)->isRoot())
        {
            continue;
        }

        PAlbum* const p = dynamic_cast<PAlbum*>(*it);

        if (p)
        {
            imageList.append(d->imagesFromPAlbum(p));
        }
    }

    return imageList;
}

DBInfoIface::DInfoMap DBInfoIface::albumInfo(int gid) const
{
    Album* const a = d->albumManager->findAlbum(gid);

    if (!a)
    {
        return DInfoMap();
    }

    DInfoMap map;
    map.insert(QLatin1String("title"), a->title());

    PAlbum* const p = dynamic_cast<PAlbum*>(a);

    if (p)
    {
        map.insert(QLatin1String("caption"),  p->caption());
        map.insert(QLatin1String("date"),     p->date());
        map.insert(QLatin1String("path"),     p->folderPath());
    }

    return map;
}

DBInfoIface::DInfoMap DBInfoIface::itemInfo(const QUrl& url) const
{
    DInfoMap map;

    ItemInfo info = ItemInfo::fromUrl(url);

    if (!info.isNull())
    {
        map.insert(QLatin1String("name"),        info.name());
        map.insert(QLatin1String("title"),       info.title());
        map.insert(QLatin1String("comment"),     info.comment());
        map.insert(QLatin1String("orientation"), info.orientation());
        map.insert(QLatin1String("datetime"),    info.dateTime());
        map.insert(QLatin1String("rating"),      info.rating());
        map.insert(QLatin1String("colorlabel"),  info.colorLabel());
        map.insert(QLatin1String("picklabel"),   info.pickLabel());
        map.insert(QLatin1String("filesize"),    info.fileSize());
        map.insert(QLatin1String("dimensions"),  info.dimensions());

        // Get digiKam Tags Path list of picture from database.
        // Ex.: "City/Paris/Monuments/Notre Dame"
        QList<int> tagIds    = info.tagIds();
        QStringList tagspath = AlbumManager::instance()->tagPaths(tagIds, false);
        map.insert(QLatin1String("tagspath"),    tagspath);

        // Get digiKam Tags name (keywords) list of picture from database.
        // Ex.: "Notre Dame"
        QStringList tags     = AlbumManager::instance()->tagNames(tagIds);
        map.insert(QLatin1String("keywords"),    tags);

        // Get GPS location of picture from database.
        ItemPosition pos    = info.imagePosition();

        if (!pos.isEmpty())
        {
            map.insert(QLatin1String("latitude"),  pos.latitudeNumber());
            map.insert(QLatin1String("longitude"), pos.longitudeNumber());
            map.insert(QLatin1String("altitude"),  pos.altitude());
        }

        // Get Copyright information of picture from database.
        ItemCopyright rights        = info.imageCopyright();
        map.insert(QLatin1String("creators"),     rights.creator());
        map.insert(QLatin1String("credit"),       rights.credit());
        map.insert(QLatin1String("rights"),       rights.rights());
        map.insert(QLatin1String("source"),       rights.source());

        PhotoInfoContainer photoInfo = info.photoInfoContainer();
        map.insert(QLatin1String("make"),            photoInfo.make);
        map.insert(QLatin1String("model"),           photoInfo.model);
        map.insert(QLatin1String("exposuretime"),    photoInfo.exposureTime);
        map.insert(QLatin1String("sensitivity"),     photoInfo.sensitivity);
        map.insert(QLatin1String("aperture"),        photoInfo.aperture);
        map.insert(QLatin1String("focallength"),     photoInfo.focalLength);
        map.insert(QLatin1String("focalLength35mm"), photoInfo.focalLength35mm);

        // TODO: add more video metadata as needed
        VideoInfoContainer videoInfo = info.videoInfoContainer();
        map.insert(QLatin1String("videocodec"),   videoInfo.videoCodec);

        qCDebug(DIGIKAM_GENERAL_LOG) << "Database Info populated for" << url;
    }
    else
    {
        qCDebug(DIGIKAM_GENERAL_LOG) << "Database Info is NULL for" << url;

    }

    return map;
}

void DBInfoIface::setItemInfo(const QUrl& url, const DInfoMap& map) const
{
    ItemInfo info    = ItemInfo::fromUrl(url);
    QStringList keys = map.keys();

    if (map.contains(QLatin1String("orientation")))
    {
        info.setOrientation(map[QLatin1String("orientation")].toInt());
        keys.removeAll(QLatin1String("orientation"));
    }

    if (map.contains(QLatin1String("rating")))
    {
        info.setRating(map[QLatin1String("rating")].toInt());
        keys.removeAll(QLatin1String("rating"));
    }

    if  (map.contains(QLatin1String("colorlabel")))
    {
        info.setColorLabel(map[QLatin1String("colorlabel")].toInt());
        keys.removeAll(QLatin1String("colorlabel"));
    }

    if  (map.contains(QLatin1String("picklabel")))
    {
        info.setPickLabel(map[QLatin1String("picklabel")].toInt());
        keys.removeAll(QLatin1String("picklabel"));
    }

    if (!keys.isEmpty())
    {
        qCWarning(DIGIKAM_GENERAL_LOG) << "Keys not yet supported in DMetaInfoIface::setItemInfo():" << keys;
    }
}

QList<QUrl> DBInfoIface::albumItems(Album* const album) const
{
    if (!album)
    {
        return QList<QUrl>();
    }

    QList<QUrl> imageList;

    switch (album->type())
    {
        case Album::PHYSICAL:
        {
            PAlbum* const p = dynamic_cast<PAlbum*>(album);

            if (p)
            {
                imageList = d->imagesFromPAlbum(p);
            }

            break;
        }

        case Album::TAG:
        {
            TAlbum* const p = dynamic_cast<TAlbum*>(album);

            if (p)
            {
                imageList = d->imagesFromTAlbum(p);
            }

            break;
        }

        case Album::SEARCH:
        {
            SAlbum* const s = dynamic_cast<SAlbum*>(album);

            if (s)
            {
                imageList = d->imagesFromSAlbum(s);
            }

            break;
        }

        default:
        {
            qCWarning(DIGIKAM_GENERAL_LOG) << "Unknown album type";
            break;
        }
    }

    return d->resolveGroupsFromAlbums(imageList);
}

QList<QUrl> DBInfoIface::albumItems(int id) const
{
    return albumItems(d->albumManager->findAlbum(id));
}

QList<QUrl> DBInfoIface::albumsItems(const DAlbumIDs& lst) const
{
    QList<QUrl> imageList;

    foreach (int gid, lst)
    {
        imageList << albumItems(gid);
    }

    return d->resolveGroupsFromAlbums(imageList);
}

QWidget* DBInfoIface::albumChooser(QWidget* const parent) const
{
    if (!d->albumsChooser)
    {
        d->albumsChooser = new AlbumSelectTabs(objectName(), parent);
    }

    connect(d->albumsChooser, SIGNAL(signalAlbumSelectionChanged()),
            this, SIGNAL(signalAlbumChooserSelectionChanged()));

    return d->albumsChooser;
}

DBInfoIface::DAlbumIDs DBInfoIface::albumChooserItems() const
{
    if (!d->albumsChooser)
    {
        return DAlbumIDs();
    }

    AlbumList lst = d->albumsChooser->selectedAlbums();
    DAlbumIDs ids;

    foreach (Album* const a, lst)
    {
        if (a)
            ids << a->globalID();
    }

    return ids;
}

bool DBInfoIface::supportAlbums() const
{
    return true;
}

QWidget* DBInfoIface::uploadWidget(QWidget* const parent) const
{
    if (!d->albumSelector)
    {
        d->albumSelector = new AlbumSelectWidget(parent);

        connect(d->albumSelector, SIGNAL(itemSelectionChanged()),
                this, SIGNAL(signalUploadUrlChanged()));
    }

    return d->albumSelector;
}

QUrl DBInfoIface::uploadUrl() const
{
    QUrl url;

    if (d->albumSelector)
    {
        PAlbum* const p = d->albumSelector->currentAlbum();

        if (p)
        {
            url = QUrl::fromLocalFile(p->folderPath());
        }
    }

    return url;
}

QUrl DBInfoIface::defaultUploadUrl() const
{
    QUrl place       = QUrl::fromLocalFile(QDir::homePath());
    QStringList pics = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);

    if (!pics.isEmpty())
        place = QUrl::fromLocalFile(pics.first());

    Album* const album = AlbumManager::instance()->currentAlbums().first();

    if (album->type() == Album::PHYSICAL)
    {
        PAlbum* const p = dynamic_cast<PAlbum*>(album);

        if (p)
        {
            place = QUrl::fromLocalFile(p->folderPath());
        }
    }
    else
    {
        QStringList cols = CollectionManager::instance()->allAvailableAlbumRootPaths();

        if (!cols.isEmpty())
        {
            place = QUrl::fromLocalFile(cols.first());
        }
    }

    return place;
}

QAbstractItemModel* DBInfoIface::tagFilterModel()
{
    TagModel* const tagModel                    = new TagModel(AbstractAlbumModel::IgnoreRootAlbum, this);
    TagPropertiesFilterModel* const filterModel = new TagPropertiesFilterModel(this);
    filterModel->setSourceAlbumModel(tagModel);
    filterModel->sort(0);

    return filterModel;
}

} // namespace Digikam
