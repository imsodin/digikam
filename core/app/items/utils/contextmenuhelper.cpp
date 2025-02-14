/* ============================================================
 *
 * This file is a part of digiKam project
 * https://www.digikam.org
 *
 * Date        : 2009-02-15
 * Description : contextmenu helper class
 *
 * Copyright (C) 2009-2011 by Andi Clemens <andi dot clemens at gmail dot com>
 * Copyright (C) 2010-2019 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#include "contextmenuhelper.h"

// Qt includes

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QIcon>
#include <QMap>
#include <QMenu>
#include <QMimeData>
#include <QPointer>
#include <QString>
#include <QTimer>

// KDE includes

#include <kactioncollection.h>
#include <klocalizedstring.h>

#ifdef HAVE_KIO
#   include <kopenwithdialog.h>
#endif

// Local includes

#include "digikam_debug.h"
#include "album.h"
#include "coredb.h"
#include "albummanager.h"
#include "albumpointer.h"
#include "albummodificationhelper.h"
#include "abstractalbummodel.h"
#include "coredbaccess.h"
#include "digikamapp.h"
#include "dfileoperations.h"
#include "iteminfo.h"
#include "itemfiltermodel.h"
#include "lighttablewindow.h"
#include "queuemgrwindow.h"
#include "picklabelwidget.h"
#include "colorlabelwidget.h"
#include "ratingwidget.h"
#include "tagmodificationhelper.h"
#include "tagspopupmenu.h"
#include "fileactionmngr.h"
#include "tagscache.h"
#include "dimg.h"
#include "dxmlguiwindow.h"

#ifdef HAVE_AKONADICONTACT
#   include "akonadiiface.h"
#endif

#ifdef Q_OS_WIN
#   include <windows.h>
#   include <shellapi.h>
#endif

namespace Digikam
{

class Q_DECL_HIDDEN ContextMenuHelper::Private
{
public:

    explicit Private(ContextMenuHelper* const q)
      : gotoAlbumAction(nullptr),
        gotoDateAction(nullptr),
        setThumbnailAction(nullptr),
        imageFilterModel(nullptr),
        albumModel(nullptr),
        parent(nullptr),
        stdActionCollection(nullptr),
        q(q)
    {
    }

    QAction*                     gotoAlbumAction;
    QAction*                     gotoDateAction;
    QAction*                     setThumbnailAction;

    QList<qlonglong>             selectedIds;
    QList<QUrl>                  selectedItems;

    QMap<int, QAction*>          queueActions;
    QMap<QString, KService::Ptr> servicesMap;

    ItemFilterModel*             imageFilterModel;
    AbstractCheckableAlbumModel* albumModel;

    QMenu*                       parent;

    KActionCollection*           stdActionCollection;

    ContextMenuHelper*           q;

public:

    QModelIndex indexForAlbumFromAction(QObject* sender) const
    {
        QAction* action = nullptr;

        if ((action = qobject_cast<QAction*>(sender)))
        {
            Album* const album = action->data().value<AlbumPointer<> >();
            return albumModel->indexForAlbum(album);
        }

        return QModelIndex();
    }

    QAction* copyFromMainCollection(const QString& name) const
    {
        QAction* const mainAction = stdActionCollection->action(name);

        if (!mainAction)
        {
            return nullptr;
        }

        QAction* const action = new QAction(mainAction->icon(), mainAction->text(), q);
        action->setShortcut(mainAction->shortcut());
        action->setToolTip(mainAction->toolTip());
        return action;
    }
};

ContextMenuHelper::ContextMenuHelper(QMenu* const parent, KActionCollection* const actionCollection)
    : QObject(parent),
      d(new Private(this))
{
    d->parent = parent;

    if (!actionCollection)
    {
        d->stdActionCollection = DigikamApp::instance()->actionCollection();
    }
    else
    {
        d->stdActionCollection = actionCollection;
    }
}

ContextMenuHelper::~ContextMenuHelper()
{
    delete d;
}

void ContextMenuHelper::addAction(const QString& name, bool addDisabled)
{
    QAction* const action = d->stdActionCollection->action(name);
    addAction(action, addDisabled);
}

void ContextMenuHelper::addAction(QAction* action, bool addDisabled)
{
    if (!action)
    {
        return;
    }

    if (action->isEnabled() || addDisabled)
    {
        d->parent->addAction(action);
    }
}

void ContextMenuHelper::addSubMenu(QMenu* subMenu)
{
    d->parent->addMenu(subMenu);
}

void ContextMenuHelper::addSeparator()
{
    d->parent->addSeparator();
}

void ContextMenuHelper::addAction(QAction* action, QObject* recv, const char* slot, bool addDisabled)
{
    if (!action)
    {
        return;
    }

    connect(action, SIGNAL(triggered()), recv, slot);
    addAction(action, addDisabled);
}

void ContextMenuHelper::addStandardActionLightTable()
{
    QAction* action = nullptr;
    QStringList ltActionNames;
    ltActionNames << QLatin1String("image_add_to_lighttable")
                  << QLatin1String("image_lighttable");

    if (LightTableWindow::lightTableWindowCreated() && !LightTableWindow::lightTableWindow()->isEmpty())
    {
        action = d->stdActionCollection->action(ltActionNames.at(0));
    }
    else
    {
        action = d->stdActionCollection->action(ltActionNames.at(1));
    }

    addAction(action);
}

void ContextMenuHelper::addStandardActionThumbnail(const imageIds& ids, Album* album)
{
    if (d->setThumbnailAction)
    {
        return;
    }

    setSelectedIds(ids);

    if (album && ids.count() == 1)
    {
        if (album->type() == Album::PHYSICAL)
        {
            d->setThumbnailAction = new QAction(i18n("Set as Album Thumbnail"), this);
        }
        else if (album->type() == Album::TAG)
        {
            d->setThumbnailAction = new QAction(i18n("Set as Tag Thumbnail"), this);
        }

        addAction(d->setThumbnailAction);
        d->parent->addSeparator();
    }
}

void ContextMenuHelper::addOpenAndNavigateActions(const imageIds &ids)
{
    addAction(QLatin1String("image_edit"));
    addServicesMenu(ItemInfoList(ids).toImageUrlList());
    addAction(QLatin1String("move_selection_to_album"));

    // addServicesMenu() has stored d->selectedItems
    if (!d->selectedItems.isEmpty())
    {
        QAction* const openFileMngr = new QAction(i18n("Open in File Manager"), this);
        addAction(openFileMngr);

        connect(openFileMngr, SIGNAL(triggered()),
                this, SLOT(slotOpenInFileManager()));
    }

    addGotoMenu(ids);
}

void ContextMenuHelper::addServicesMenu(const QList<QUrl>& selectedItems)
{
    setSelectedItems(selectedItems);

#ifdef Q_OS_WIN

    if (selectedItems.length() == 1)
    {
        QAction* const openWith = new QAction(i18n("Open With"), this);
        addAction(openWith);

        connect(openWith, SIGNAL(triggered()),
                this, SLOT(slotOpenWith()));
    }

#else // Q_OS_WIN

    KService::List offers = DFileOperations::servicesForOpenWith(selectedItems);

    if (!offers.isEmpty())
    {
        QMenu* const servicesMenu = new QMenu(d->parent);
        qDeleteAll(servicesMenu->actions());

        QAction* const serviceAction = servicesMenu->menuAction();
        serviceAction->setText(i18n("Open With"));

        foreach (const KService::Ptr& service, offers)
        {
            QString name          = service->name().replace(QLatin1Char('&'), QLatin1String("&&"));
            QAction* const action = servicesMenu->addAction(name);
            action->setIcon(QIcon::fromTheme(service->icon()));
            action->setData(service->name());
            d->servicesMap[name]  = service;
        }

#   ifdef HAVE_KIO

        servicesMenu->addSeparator();
        servicesMenu->addAction(i18n("Other..."));

        addAction(serviceAction);

        connect(servicesMenu, SIGNAL(triggered(QAction*)),
                this, SLOT(slotOpenWith(QAction*)));
    }
    else
    {
        QAction* const serviceAction = new QAction(i18n("Open With..."), this);
        addAction(serviceAction);

        connect(serviceAction, SIGNAL(triggered()),
                this, SLOT(slotOpenWith()));

#   endif // HAVE_KIO

    }

#endif // Q_OS_WIN
}

void ContextMenuHelper::slotOpenWith()
{
    // call the slot with an "empty" action
    slotOpenWith(nullptr);
}

void ContextMenuHelper::slotOpenWith(QAction* action)
{
#ifdef Q_OS_WIN

    Q_UNUSED(action);

    // See Bug #380065 for details.

    if (d->selectedItems.length() == 1)
    {
        SHELLEXECUTEINFO sei = {};
        sei.cbSize           = sizeof(sei);
        sei.fMask            = SEE_MASK_INVOKEIDLIST | SEE_MASK_NOASYNC;
        sei.nShow            = SW_SHOWNORMAL;
        sei.lpVerb           = (LPCWSTR)QString::fromLatin1("openas").utf16();
        sei.lpFile           = (LPCWSTR)d->selectedItems.first().toLocalFile().utf16();
        ShellExecuteEx(&sei);

        qCDebug(DIGIKAM_GENERAL_LOG) << "ShellExecuteEx::openas called";
    }

#else // Q_OS_WIN

    KService::Ptr service;
    QList<QUrl> list = d->selectedItems;
    QString name     = action ? action->data().toString() : QString();

#   ifdef HAVE_KIO

    if (name.isEmpty())
    {
        QPointer<KOpenWithDialog> dlg = new KOpenWithDialog(list);

        if (dlg->exec() != KOpenWithDialog::Accepted)
        {
            delete dlg;
            return;
        }

        service = dlg->service();

        if (!service)
        {
            // User entered a custom command
            if (!dlg->text().isEmpty())
            {
                DFileOperations::runFiles(dlg->text(), list);
            }

            delete dlg;
            return;
        }

        delete dlg;
    }
    else

#   endif // HAVE_KIO

    {
        service = d->servicesMap[name];
    }

    DFileOperations::runFiles(service.data(), list);

#endif // Q_OS_WIN
}

void ContextMenuHelper::slotOpenInFileManager()
{
    DFileOperations::openInFileManager(d->selectedItems);
}

bool ContextMenuHelper::imageIdsHaveSameCategory(const imageIds& ids, DatabaseItem::Category category)
{
    bool sameCategory = true;
    QVariantList varList;

    foreach (const qlonglong& id, ids)
    {
        varList = CoreDbAccess().db()->getImagesFields(id, DatabaseFields::Category);

        if (varList.isEmpty() ||
            (DatabaseItem::Category)varList.first().toInt() != category)
        {
            sameCategory = false;
            break;
        }
    }

    return sameCategory;
}

void ContextMenuHelper::addActionNewTag(TagModificationHelper* helper, TAlbum* tag)
{
    QAction* const newTagAction = new QAction(QIcon::fromTheme(QLatin1String("tag-new")), i18n("New Tag..."), this);
    addAction(newTagAction);
    helper->bindTag(newTagAction, tag);

    connect(newTagAction, SIGNAL(triggered()),
            helper, SLOT(slotTagNew()));
}

void ContextMenuHelper::addActionDeleteTag(TagModificationHelper* helper, TAlbum* tag)
{
    QAction* const deleteTagAction = new QAction(QIcon::fromTheme(QLatin1String("edit-delete")), i18n("Delete Tag"), this);
    addAction(deleteTagAction);
    helper->bindTag(deleteTagAction, tag);

    connect(deleteTagAction, SIGNAL(triggered()),
            helper, SLOT(slotTagDelete()));
}

void ContextMenuHelper::addActionDeleteTags(Digikam::TagModificationHelper* helper, QList< TAlbum* > tags)
{
    QAction* const deleteTagsAction = new QAction(QIcon::fromTheme(QLatin1String("edit-delete")), i18n("Delete Tags"), this);
    addAction(deleteTagsAction);
    helper->bindMultipleTags(deleteTagsAction, tags);

    connect(deleteTagsAction, SIGNAL(triggered()),
            helper, SLOT(slotMultipleTagDel()));
}

void ContextMenuHelper::addActionTagToFaceTag(TagModificationHelper* helper, TAlbum* tag)
{
    QAction* const tagToFaceTagAction = new QAction(QIcon::fromTheme(QLatin1String("tag-properties")), i18n("Mark As Face Tag"), this);
    addAction(tagToFaceTagAction);
    helper->bindTag(tagToFaceTagAction, tag);

    connect(tagToFaceTagAction, SIGNAL(triggered()),
            helper, SLOT(slotTagToFaceTag()));
}

void ContextMenuHelper::addActionTagsToFaceTags(TagModificationHelper* helper, QList< TAlbum* > tags)
{
    QAction* const tagToFaceTagsAction = new QAction(QIcon::fromTheme(QLatin1String("tag-properties")), i18n("Mark As Face Tags"), this);
    addAction(tagToFaceTagsAction);
    helper->bindMultipleTags(tagToFaceTagsAction, tags);

    connect(tagToFaceTagsAction, SIGNAL(triggered()),
            helper, SLOT(slotMultipleTagsToFaceTags()));
}

void ContextMenuHelper::addActionEditTag(TagModificationHelper* helper, TAlbum* tag)
{
    QAction* const editTagAction = new QAction(QIcon::fromTheme(QLatin1String("tag-properties")), i18nc("Edit Tag Properties", "Properties..."), this);
    // This is only for the user to give a hint for the shortcut key
    editTagAction->setShortcut(Qt::ALT + Qt::Key_Return);
    addAction(editTagAction);
    helper->bindTag(editTagAction, tag);

    connect(editTagAction, SIGNAL(triggered()),
            helper, SLOT(slotTagEdit()));
}

void ContextMenuHelper::addActionDeleteFaceTag(TagModificationHelper* helper, TAlbum* tag)
{
    QAction* const deleteFaceTagAction = new QAction(QIcon::fromTheme(QLatin1String("user-trash")), i18n("Remove Face Tag"), this);
    deleteFaceTagAction->setWhatsThis(i18n("Removes the face property from the selected tag and the face region from the contained images. Can also untag the images if wished."));
    addAction(deleteFaceTagAction);
    helper->bindTag(deleteFaceTagAction, tag);

    connect(deleteFaceTagAction, SIGNAL(triggered()),
            helper, SLOT(slotFaceTagDelete()));
}

void ContextMenuHelper::addActionDeleteFaceTags(TagModificationHelper* helper, QList< TAlbum* > tags)
{
    QAction* const deleteFaceTagsAction = new QAction(QIcon::fromTheme(QLatin1String("user-trash")), i18n("Remove Face Tags"), this);
    deleteFaceTagsAction->setWhatsThis(i18n("Removes the face property from the selected tags and the face region from the contained images. Can also untag the images if wished."));
    addAction(deleteFaceTagsAction);
    helper->bindMultipleTags(deleteFaceTagsAction, tags);

    connect(deleteFaceTagsAction, SIGNAL(triggered()),
            helper, SLOT(slotMultipleFaceTagDel()));
}

void ContextMenuHelper::addActionNewAlbum(AlbumModificationHelper* helper, PAlbum* parentAlbum)
{
    QAction* const action = d->copyFromMainCollection(QLatin1String("album_new"));
    addAction(action);
    helper->bindAlbum(action, parentAlbum);

    connect(action, SIGNAL(triggered()),
            helper, SLOT(slotAlbumNew()));
}

void ContextMenuHelper::addActionDeleteAlbum(AlbumModificationHelper* helper, PAlbum* album)
{
    QAction* const action = d->copyFromMainCollection(QLatin1String("album_delete"));
    addAction(action, !(album->isRoot() || album->isAlbumRoot()));
    helper->bindAlbum(action, album);

    connect(action, SIGNAL(triggered()),
            helper, SLOT(slotAlbumDelete()));
}

void ContextMenuHelper::addActionEditAlbum(AlbumModificationHelper* helper, PAlbum* album)
{
    QAction* const action = d->copyFromMainCollection(QLatin1String("album_propsEdit"));
    addAction(action, !album->isRoot());
    helper->bindAlbum(action, album);

    connect(action, SIGNAL(triggered()),
            helper, SLOT(slotAlbumEdit()));
}

void ContextMenuHelper::addActionRenameAlbum(AlbumModificationHelper* helper, PAlbum* album)
{
    QAction* const action = d->copyFromMainCollection(QLatin1String("album_rename"));
    addAction(action, !(album->isRoot() || album->isAlbumRoot()));
    helper->bindAlbum(action, album);

    connect(action, SIGNAL(triggered()),
            helper, SLOT(slotAlbumRename()));
}

void ContextMenuHelper::addActionResetAlbumIcon(AlbumModificationHelper* helper, PAlbum* album)
{
    QAction* const action = new QAction(QIcon::fromTheme(QLatin1String("view-refresh")), i18n("Reset Album Icon"), this);
    addAction(action, !album->isRoot());
    helper->bindAlbum(action, album);

    connect(action, SIGNAL(triggered()),
            helper, SLOT(slotAlbumResetIcon()));
}

void ContextMenuHelper::addAssignTagsMenu(const imageIds &ids)
{
    setSelectedIds(ids);

    QMenu* const assignTagsPopup = new TagsPopupMenu(ids, TagsPopupMenu::RECENTLYASSIGNED, d->parent);
    assignTagsPopup->menuAction()->setText(i18n("A&ssign Tag"));
    assignTagsPopup->menuAction()->setIcon(QIcon::fromTheme(QLatin1String("tag")));
    d->parent->addMenu(assignTagsPopup);

    connect(assignTagsPopup, SIGNAL(signalTagActivated(int)),
            this, SIGNAL(signalAssignTag(int)));

    connect(assignTagsPopup, SIGNAL(signalPopupTagsView()),
            this, SIGNAL(signalPopupTagsView()));
}

void ContextMenuHelper::addRemoveTagsMenu(const imageIds &ids)
{
    setSelectedIds(ids);

    QMenu* const removeTagsPopup = new TagsPopupMenu(ids, TagsPopupMenu::REMOVE, d->parent);
    removeTagsPopup->menuAction()->setText(i18n("R&emove Tag"));
    removeTagsPopup->menuAction()->setIcon(QIcon::fromTheme(QLatin1String("tag")));
    d->parent->addMenu(removeTagsPopup);

    // Performance: Only check for tags if there are <250 images selected
    // Otherwise enable it regardless if there are tags or not
    if (ids.count() < 250)
    {
        QList<int> tagIDs = CoreDbAccess().db()->getItemCommonTagIDs(ids);
        bool enable       = false;

        foreach (int tag, tagIDs)
        {
            if (TagsCache::instance()->colorLabelForTag(tag) == -1 &&
                TagsCache::instance()->pickLabelForTag(tag)  == -1 &&
                TagsCache::instance()->isInternalTag(tag)    == false)
            {
                enable = true;
                break;
            }
        }

        removeTagsPopup->menuAction()->setEnabled(enable);
    }

    connect(removeTagsPopup, SIGNAL(signalTagActivated(int)),
            this, SIGNAL(signalRemoveTag(int)));
}

void ContextMenuHelper::addLabelsAction()
{
    QMenu* const menuLabels           = new QMenu(i18n("Assign Labe&ls"), d->parent);
    PickLabelMenuAction* const pmenu  = new PickLabelMenuAction(d->parent);
    ColorLabelMenuAction* const cmenu = new ColorLabelMenuAction(d->parent);
    RatingMenuAction* const rmenu     = new RatingMenuAction(d->parent);
    menuLabels->addAction(pmenu->menuAction());
    menuLabels->addAction(cmenu->menuAction());
    menuLabels->addAction(rmenu->menuAction());
    addSubMenu(menuLabels);

    connect(pmenu, SIGNAL(signalPickLabelChanged(int)),
            this, SIGNAL(signalAssignPickLabel(int)));

    connect(cmenu, SIGNAL(signalColorLabelChanged(int)),
            this, SIGNAL(signalAssignColorLabel(int)));

    connect(rmenu, SIGNAL(signalRatingChanged(int)),
            this, SIGNAL(signalAssignRating(int)));
}

void ContextMenuHelper::addCreateTagFromAddressbookMenu()
{
#ifdef HAVE_AKONADICONTACT
    AkonadiIface* const abc = new AkonadiIface(d->parent);

    connect(abc, SIGNAL(signalContactTriggered(QString)),
            this, SIGNAL(signalAddNewTagFromABCMenu(QString)));

    // AkonadiIface instance will be deleted with d->parent.
#endif
}

void ContextMenuHelper::slotDeselectAllAlbumItems()
{
    QAction* const selectNoneAction = d->stdActionCollection->action(QLatin1String("selectNone"));
    QTimer::singleShot(75, selectNoneAction, SIGNAL(triggered()));
}

void ContextMenuHelper::addImportMenu()
{
    QMenu* const menuImport       = new QMenu(i18n("Import"), d->parent);
    KXMLGUIClient* const client   = const_cast<KXMLGUIClient*>(d->stdActionCollection->parentGUIClient());
    QList<DPluginAction*> actions = DPluginLoader::instance()->pluginsActions(DPluginAction::GenericImport,
                                    dynamic_cast<KXmlGuiWindow*>(client));

    if (!actions.isEmpty())
    {
        foreach (DPluginAction* const ac, actions)
        {
            menuImport->addActions(QList<QAction*>() << ac);
        }
    }
    else
    {
        QAction* const notools = new QAction(i18n("No import tool available"), this);
        notools->setEnabled(false);
        menuImport->addAction(notools);
    }

    d->parent->addMenu(menuImport);
}

void ContextMenuHelper::addExportMenu()
{
    QMenu* const menuExport       = new QMenu(i18n("Export"), d->parent);
    KXMLGUIClient* const client   = const_cast<KXMLGUIClient*>(d->stdActionCollection->parentGUIClient());
    QList<DPluginAction*> actions = DPluginLoader::instance()->pluginsActions(DPluginAction::GenericExport,
                                    dynamic_cast<KXmlGuiWindow*>(client));

#if 0
    QAction* selectAllAction = 0;
    selectAllAction = d->stdActionCollection->action("selectAll");
#endif

    if (!actions.isEmpty())
    {
        foreach (DPluginAction* const ac, actions)
        {
            menuExport->addActions(QList<QAction*>() << ac);
        }
    }
    else
    {
        QAction* const notools = new QAction(i18n("No export tool available"), this);
        notools->setEnabled(false);
        menuExport->addAction(notools);
    }

    d->parent->addMenu(menuExport);
}

void ContextMenuHelper::addAlbumActions()
{
    QList<QAction*> albumActions;

    if (!albumActions.isEmpty())
    {
        d->parent->addActions(albumActions);
    }
}

void ContextMenuHelper::addGotoMenu(const imageIds &ids)
{
    if (d->gotoAlbumAction && d->gotoDateAction)
    {
        return;
    }

    setSelectedIds(ids);

    // the currently selected image is always the first item
    ItemInfo item;

    if (!d->selectedIds.isEmpty())
    {
        item = ItemInfo(d->selectedIds.first());
    }

    if (item.isNull())
    {
        return;
    }

    // when more then one item is selected, don't add the menu
    if (d->selectedIds.count() > 1)
    {
        return;
    }

    d->gotoAlbumAction    = new QAction(QIcon::fromTheme(QLatin1String("folder-pictures")),     i18n("Album"), this);
    d->gotoDateAction     = new QAction(QIcon::fromTheme(QLatin1String("view-calendar")), i18n("Date"),  this);
    QMenu* const gotoMenu = new QMenu(d->parent);
    gotoMenu->addAction(d->gotoAlbumAction);
    gotoMenu->addAction(d->gotoDateAction);

    TagsPopupMenu* const gotoTagsPopup = new TagsPopupMenu(d->selectedIds, TagsPopupMenu::DISPLAY, gotoMenu);
    QAction* const gotoTag             = gotoMenu->addMenu(gotoTagsPopup);
    gotoTag->setIcon(QIcon::fromTheme(QLatin1String("tag")));
    gotoTag->setText(i18n("Tag"));

    // Disable the goto Tag popup menu, if there are no tags at all.
    if (!CoreDbAccess().db()->hasTags(d->selectedIds))
    {
        gotoTag->setEnabled(false);
    }

    /**
     * TODO:tags to be ported to multiple selection
     */
    QList<Album*> albumList = AlbumManager::instance()->currentAlbums();
    Album* currentAlbum     = nullptr;

    if (!albumList.isEmpty())
    {
        currentAlbum = albumList.first();
    }
    else
    {
        return;
    }

    if (currentAlbum->type() == Album::PHYSICAL)
    {
        // If the currently selected album is the same as album to
        // which the image belongs, then disable the "Go To" Album.
        // (Note that in recursive album view these can be different).
        if (item.albumId() == currentAlbum->id())
        {
            d->gotoAlbumAction->setEnabled(false);
        }
    }
    else if (currentAlbum->type() == Album::DATE)
    {
        d->gotoDateAction->setEnabled(false);
    }

    QAction* const gotoMenuAction = gotoMenu->menuAction();
    gotoMenuAction->setIcon(QIcon::fromTheme(QLatin1String("go-jump")));
    gotoMenuAction->setText(i18n("Go To"));

    connect(gotoTagsPopup, SIGNAL(signalTagActivated(int)),
            this, SIGNAL(signalGotoTag(int)));

    addAction(gotoMenuAction);
}

void ContextMenuHelper::addQueueManagerMenu()
{
    QMenu* const bqmMenu = new QMenu(i18n("Batch Queue Manager"), d->parent);
    bqmMenu->menuAction()->setIcon(QIcon::fromTheme(QLatin1String("run-build")));
    bqmMenu->addAction(d->stdActionCollection->action(QLatin1String("image_add_to_current_queue")));
    bqmMenu->addAction(d->stdActionCollection->action(QLatin1String("image_add_to_new_queue")));

    // if queue list is empty, do not display the queue submenu
    if (QueueMgrWindow::queueManagerWindowCreated() &&
        !QueueMgrWindow::queueManagerWindow()->queuesMap().isEmpty())
    {
        QueueMgrWindow* const qmw = QueueMgrWindow::queueManagerWindow();
        QMenu* const queueMenu    = new QMenu(i18n("Add to Existing Queue"), bqmMenu);

        // queueActions is used by the exec() method to emit an appropriate signal.
        // Reset the map before filling in the actions.
        if (!d->queueActions.isEmpty())
        {
            d->queueActions.clear();
        }

        QList<QAction*> queueList;

        // get queue list from BQM window, do not access it directly, it might crash
        // when the list is changed
        QMap<int, QString> qmwMap = qmw->queuesMap();

        for (QMap<int, QString>::const_iterator it = qmwMap.constBegin(); it != qmwMap.constEnd(); ++it)
        {
            QAction* const action = new QAction(it.value(), this);
            queueList << action;
            d->queueActions[it.key()] = action;
        }

        queueMenu->addActions(queueList);
        bqmMenu->addMenu(queueMenu);
    }

    d->parent->addMenu(bqmMenu);

    // NOTE: see bug #252130 : we need to disable new items to add on BQM is this one is running.
    bqmMenu->setDisabled(QueueMgrWindow::queueManagerWindow()->isBusy());
}

void ContextMenuHelper::setAlbumModel(AbstractCheckableAlbumModel* model)
{
    d->albumModel = model;
}

void ContextMenuHelper::addAlbumCheckUncheckActions(Album* album)
{
    bool     enabled   = false;
    QString  allString = i18n("All Albums");
    QVariant albumData;

    if (album)
    {
        enabled   = true;
        albumData = QVariant::fromValue(AlbumPointer<>(album));

        if (album->type() == Album::TAG)
            allString = i18n("All Tags");
    }

    QMenu* const selectTagsMenu = new QMenu(i18nc("select tags menu", "Select"));
    addSubMenu(selectTagsMenu);

    selectTagsMenu->addAction(allString, d->albumModel, SLOT(checkAllAlbums()));
    selectTagsMenu->addSeparator();
    QAction* const selectChildrenAction = selectTagsMenu->addAction(i18n("Children"), this, SLOT(slotSelectChildren()));
    QAction* const selectParentsAction  = selectTagsMenu->addAction(i18n("Parents"),  this, SLOT(slotSelectParents()));
    selectChildrenAction->setData(albumData);
    selectParentsAction->setData(albumData);

    QMenu* const deselectTagsMenu = new QMenu(i18nc("deselect tags menu", "Deselect"));
    addSubMenu(deselectTagsMenu);

    deselectTagsMenu->addAction(allString, d->albumModel, SLOT(resetAllCheckedAlbums()));
    deselectTagsMenu->addSeparator();
    QAction* const deselectChildrenAction = deselectTagsMenu->addAction(i18n("Children"), this, SLOT(slotDeselectChildren()));
    QAction* const deselectParentsAction  = deselectTagsMenu->addAction(i18n("Parents"),  this, SLOT(slotDeselectParents()));
    deselectChildrenAction->setData(albumData);
    deselectParentsAction->setData(albumData);

    d->parent->addAction(i18n("Invert Selection"), d->albumModel, SLOT(invertCheckedAlbums()));

    selectChildrenAction->setEnabled(enabled);
    selectParentsAction->setEnabled(enabled);
    deselectChildrenAction->setEnabled(enabled);
    deselectParentsAction->setEnabled(enabled);
}

void ContextMenuHelper::slotSelectChildren()
{
    if (!d->albumModel)
    {
        return;
    }

    d->albumModel->checkAllAlbums(d->indexForAlbumFromAction(sender()));
}

void ContextMenuHelper::slotDeselectChildren()
{
    if (!d->albumModel)
    {
        return;
    }

    d->albumModel->resetCheckedAlbums(d->indexForAlbumFromAction(sender()));
}

void ContextMenuHelper::slotSelectParents()
{
    if (!d->albumModel)
    {
        return;
    }

    d->albumModel->checkAllParentAlbums(d->indexForAlbumFromAction(sender()));
}

void ContextMenuHelper::slotDeselectParents()
{
    if (!d->albumModel)
    {
        return;
    }

    d->albumModel->resetCheckedParentAlbums(d->indexForAlbumFromAction(sender()));
}

void ContextMenuHelper::addGroupMenu(const imageIds &ids, const QList<QAction*>& extraMenuItems)
{
    QList<QAction*> actions = groupMenuActions(ids);

    if (actions.isEmpty() && extraMenuItems.isEmpty())
    {
        return;
    }

    if (!extraMenuItems.isEmpty())
    {
        if (!actions.isEmpty())
        {
            QAction* separator = new QAction(this);
            separator->setSeparator(true);
            actions << separator;
        }

        actions << extraMenuItems;
    }

    QMenu* const menu = new QMenu(i18n("Group"));

    foreach (QAction* const action, actions)
    {
        menu->addAction(action);
    }

    d->parent->addMenu(menu);
}

void ContextMenuHelper::addGroupActions(const imageIds &ids)
{
    foreach (QAction* const action, groupMenuActions(ids))
    {
        d->parent->addAction(action);
    }
}

void ContextMenuHelper::setItemFilterModel(ItemFilterModel* model)
{
    d->imageFilterModel = model;
}

QList<QAction*> ContextMenuHelper::groupMenuActions(const imageIds &ids)
{
    setSelectedIds(ids);

    QList<QAction*> actions;

    if (ids.isEmpty())
    {
        if (d->imageFilterModel)
        {
            if (!d->imageFilterModel->isAllGroupsOpen())
            {
                QAction* const openAction = new QAction(i18nc("@action:inmenu", "Open All Groups"), this);
                connect(openAction, SIGNAL(triggered()), this, SLOT(slotOpenGroups()));
                actions << openAction;
            }
            else
            {
                QAction* const closeAction = new QAction(i18nc("@action:inmenu", "Close All Groups"), this);
                connect(closeAction, SIGNAL(triggered()), this, SLOT(slotCloseGroups()));
                actions << closeAction;
            }
        }

        return actions;
    }

    ItemInfo info(ids.first());

    if (ids.size() == 1)
    {
        if (info.hasGroupedImages())
        {
            if (d->imageFilterModel)
            {
                if (!d->imageFilterModel->isGroupOpen(info.id()))
                {
                    QAction* const action = new QAction(i18nc("@action:inmenu", "Show Grouped Images"), this);
                    connect(action, SIGNAL(triggered()), this, SLOT(slotOpenGroups()));
                    actions << action;
                }
                else
                {
                    QAction* const action = new QAction(i18nc("@action:inmenu", "Hide Grouped Images"), this);
                    connect(action, SIGNAL(triggered()), this, SLOT(slotCloseGroups()));
                    actions << action;
                }
            }

            QAction* const separator = new QAction(this);
            separator->setSeparator(true);
            actions << separator;

            QAction* const clearAction = new QAction(i18nc("@action:inmenu", "Ungroup"), this);
            connect(clearAction, SIGNAL(triggered()), this, SIGNAL(signalUngroup()));
            actions << clearAction;
        }
        else if (info.isGrouped())
        {
            QAction* const action = new QAction(i18nc("@action:inmenu", "Remove From Group"), this);
            connect(action, SIGNAL(triggered()), this, SIGNAL(signalRemoveFromGroup()));
            actions << action;

            // TODO: set as group leader / pick image
        }
    }
    else
    {
        QAction* const closeAction = new QAction(i18nc("@action:inmenu", "Group Selected Here"), this);
        connect(closeAction, SIGNAL(triggered()), this, SIGNAL(signalCreateGroup()));
        actions << closeAction;

        QAction* const closeActionDate = new QAction(i18nc("@action:inmenu", "Group Selected By Time"), this);
        connect(closeActionDate, SIGNAL(triggered()), this, SIGNAL(signalCreateGroupByTime()));
        actions << closeActionDate;

        QAction* const closeActionType = new QAction(i18nc("@action:inmenu", "Group Selected By Filename"), this);
        connect(closeActionType, SIGNAL(triggered()), this, SIGNAL(signalCreateGroupByFilename()));
        actions << closeActionType;

        QAction* const closeActionTimelapse = new QAction(i18nc("@action:inmenu", "Group Selected By Timelapse / Burst"), this);
        connect(closeActionTimelapse, SIGNAL(triggered()), this, SIGNAL(signalCreateGroupByTimelapse()));
        actions << closeActionTimelapse;

        QAction* const separator = new QAction(this);
        separator->setSeparator(true);
        actions << separator;

        if (d->imageFilterModel)
        {
            QAction* const openAction = new QAction(i18nc("@action:inmenu", "Show Grouped Images"), this);
            connect(openAction, SIGNAL(triggered()), this, SLOT(slotOpenGroups()));
            actions << openAction;

            QAction* const closeAction = new QAction(i18nc("@action:inmenu", "Hide Grouped Images"), this);
            connect(closeAction, SIGNAL(triggered()), this, SLOT(slotCloseGroups()));
            actions << closeAction;

            QAction* const separator2 = new QAction(this);
            separator2->setSeparator(true);
            actions << separator2;
        }

        QAction* const removeAction = new QAction(i18nc("@action:inmenu", "Remove Selected From Groups"), this);
        connect(removeAction, SIGNAL(triggered()), this, SIGNAL(signalRemoveFromGroup()));
        actions << removeAction;

        QAction* const clearAction = new QAction(i18nc("@action:inmenu", "Ungroup Selected"), this);
        connect(clearAction, SIGNAL(triggered()), this, SIGNAL(signalUngroup()));
        actions << clearAction;
    }

    return actions;
}

void ContextMenuHelper::setGroupsOpen(bool open)
{
    if (!d->imageFilterModel || d->selectedIds.isEmpty())
    {
        return;
    }

    GroupItemFilterSettings settings = d->imageFilterModel->groupItemFilterSettings();

    foreach (const qlonglong& id, d->selectedIds)
    {
        ItemInfo info(id);

        if (info.hasGroupedImages())
        {
            settings.setOpen(id, open);
        }
    }

    d->imageFilterModel->setGroupItemFilterSettings(settings);
}

void ContextMenuHelper::slotOpenGroups()
{
    setGroupsOpen(true);
}

void ContextMenuHelper::slotCloseGroups()
{
    setGroupsOpen(false);
}

void ContextMenuHelper::slotOpenAllGroups()
{
    if (!d->imageFilterModel)
    {
        return;
    }

    d->imageFilterModel->setAllGroupsOpen(true);
}

void ContextMenuHelper::slotCloseAllGroups()
{
    if (!d->imageFilterModel)
    {
        return;
    }

    d->imageFilterModel->setAllGroupsOpen(false);
}


void ContextMenuHelper::addStandardActionCut(QObject* recv, const char* slot)
{
    QAction* const cut = DXmlGuiWindow::buildStdAction(StdCutAction, recv, slot, d->parent);
    addAction(cut);
}

void ContextMenuHelper::addStandardActionCopy(QObject* recv, const char* slot)
{
    QAction* const copy = DXmlGuiWindow::buildStdAction(StdCopyAction, recv, slot, d->parent);
    addAction(copy);
}

void ContextMenuHelper::addStandardActionPaste(QObject* recv, const char* slot)
{
    QAction* const paste        = DXmlGuiWindow::buildStdAction(StdPasteAction, recv, slot, d->parent);
    const QMimeData* const data = qApp->clipboard()->mimeData(QClipboard::Clipboard);

    if (!data || !data->hasUrls())
    {
        paste->setEnabled(false);
    }

    addAction(paste, true);
}

void ContextMenuHelper::addStandardActionItemDelete(QObject* recv, const char* slot, int quantity)
{
    QAction* const trashAction = new QAction(QIcon::fromTheme(QLatin1String("user-trash-full")), i18ncp("@action:inmenu Pluralized",
                                             "Move to Trash", "Move %1 Files to Trash", quantity), d->parent);
    connect(trashAction, SIGNAL(triggered()),
            recv, slot);

    addAction(trashAction);
}

QAction* ContextMenuHelper::exec(const QPoint& pos, QAction* at)
{
    QAction* const choice = d->parent->exec(pos, at);

    if (choice)
    {
        if (d->selectedIds.count() == 1)
        {
            ItemInfo selectedItem(d->selectedIds.first());

            if (choice == d->gotoAlbumAction)
            {
                emit signalGotoAlbum(selectedItem);
            }
            else if (choice == d->gotoDateAction)
            {
                emit signalGotoDate(selectedItem);
            }
            else if (choice == d->setThumbnailAction)
            {
                emit signalSetThumbnail(selectedItem);
            }
        }

        // check if a BQM action has been triggered
        for (QMap<int, QAction*>::const_iterator it = d->queueActions.constBegin();
             it != d->queueActions.constEnd(); ++it)
        {
            if (choice == it.value())
            {
                emit signalAddToExistingQueue(it.key());
                return choice;
            }
        }
    }

    return choice;
}

void ContextMenuHelper::setSelectedIds(const imageIds &ids)
{
    if (d->selectedIds.isEmpty())
    {
        d->selectedIds = ids;
    }
}

void ContextMenuHelper::setSelectedItems(const QList<QUrl>& urls)
{
    if (d->selectedItems.isEmpty())
    {
        d->selectedItems = urls;
    }
}

} // namespace Digikam
