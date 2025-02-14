/* ============================================================
 *
 * This file is a part of digiKam project
 * https://www.digikam.org
 *
 * Date        : 2004-11-17
 * Description : item properties side bar (without support of digiKam database).
 *
 * Copyright (C) 2004-2019 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#include "itempropertiessidebar.h"

// Qt includes

#include <QRect>
#include <QSplitter>
#include <QFileInfo>
#include <QApplication>
#include <QIcon>
#include <QLocale>
#include <QMimeDatabase>
#include <QMimeType>

// KDE includes

#include <klocalizedstring.h>
#include <kconfiggroup.h>

// Local includes

#include "drawdecoder.h"
#include "digikam_config.h"
#include "digikam_debug.h"
#include "dimg.h"
#include "dmetadata.h"
#include "itempropertiestab.h"
#include "itempropertiesmetadatatab.h"
#include "itempropertiescolorstab.h"
#include "itempropertiesversionstab.h"

#ifdef HAVE_MARBLE
#   include "itempropertiesgpstab.h"
#endif // HAVE_MARBLE

namespace Digikam
{

ItemPropertiesSideBar::ItemPropertiesSideBar(QWidget* const parent,
                                             SidebarSplitter* const splitter,
                                             Qt::Edge side,
                                             bool mimimizedDefault)
    : Sidebar(parent, splitter, side, mimimizedDefault)
{
    m_image              = nullptr;
    m_currentRect        = QRect();
    m_dirtyPropertiesTab = false;
    m_dirtyMetadataTab   = false;
    m_dirtyColorTab      = false;
    m_dirtyGpsTab        = false;
    m_dirtyHistoryTab    = false;

    m_propertiesTab      = new ItemPropertiesTab(parent);
    m_metadataTab        = new ItemPropertiesMetadataTab(parent);
    m_colorTab           = new ItemPropertiesColorsTab(parent);

    // NOTE: Special case with Showfoto which will only be able to load image, not video.
    if (QApplication::applicationName() != QLatin1String("digikam"))
        m_propertiesTab->setVideoInfoDisable(true);

    appendTab(m_propertiesTab, QIcon::fromTheme(QLatin1String("configure")),        i18n("Properties"));
    appendTab(m_metadataTab,   QIcon::fromTheme(QLatin1String("format-text-code")), i18n("Metadata")); // krazy:exclude=iconnames
    appendTab(m_colorTab,      QIcon::fromTheme(QLatin1String("fill-color")),       i18n("Colors"));

#ifdef HAVE_MARBLE
    m_gpsTab = new ItemPropertiesGPSTab(parent);
    appendTab(m_gpsTab,        QIcon::fromTheme(QLatin1String("globe")),            i18n("Map"));
#endif // HAVE_MARBLE

    connect(this, SIGNAL(signalChangedTab(QWidget*)),
            this, SLOT(slotChangedTab(QWidget*)));

    connect(m_metadataTab, SIGNAL(signalSetupMetadataFilters(int)),
            this, SIGNAL(signalSetupMetadataFilters(int)));
}

ItemPropertiesSideBar::~ItemPropertiesSideBar()
{
}

void ItemPropertiesSideBar::itemChanged(const QUrl& url, const QRect& rect, DImg* const img)
{
    if (!url.isValid())
    {
        return;
    }

    m_currentURL         = url;
    m_currentRect        = rect;
    m_image              = img;
    m_dirtyPropertiesTab = false;
    m_dirtyMetadataTab   = false;
    m_dirtyColorTab      = false;
    m_dirtyGpsTab        = false;
    m_dirtyHistoryTab    = false;

    slotChangedTab( getActiveTab() );
}

void ItemPropertiesSideBar::slotNoCurrentItem()
{
    m_currentURL = QUrl();

    m_propertiesTab->setCurrentURL();
    m_metadataTab->setCurrentURL();
    m_colorTab->setData();

#ifdef HAVE_MARBLE
    m_gpsTab->setCurrentURL();
#endif // HAVE_MARBLE

    m_dirtyPropertiesTab = false;
    m_dirtyMetadataTab   = false;
    m_dirtyColorTab      = false;
    m_dirtyGpsTab        = false;
    m_dirtyHistoryTab    = false;
}

void ItemPropertiesSideBar::slotImageSelectionChanged(const QRect& rect)
{
    m_currentRect = rect;

    if (m_dirtyColorTab)
    {
        m_colorTab->setSelection(rect);
    }
    else
    {
        slotChangedTab(m_colorTab);
    }
}

void ItemPropertiesSideBar::slotChangedTab(QWidget* tab)
{
    if (!m_currentURL.isValid())
    {
#ifdef HAVE_MARBLE
        m_gpsTab->setActive(tab == m_gpsTab);
#endif // HAVE_MARBLE
        return;
    }

    setCursor(Qt::WaitCursor);

    if (tab == m_propertiesTab && !m_dirtyPropertiesTab)
    {
        m_propertiesTab->setCurrentURL(m_currentURL);
        setImagePropertiesInformation(m_currentURL);
        m_dirtyPropertiesTab = true;
    }
    else if (tab == m_metadataTab && !m_dirtyMetadataTab)
    {
        m_metadataTab->setCurrentURL(m_currentURL);
        m_dirtyMetadataTab = true;
    }
    else if (tab == m_colorTab && !m_dirtyColorTab)
    {
        m_colorTab->setData(m_currentURL, m_currentRect, m_image);
        m_dirtyColorTab = true;
    }
#ifdef HAVE_MARBLE
    else if (tab == m_gpsTab && !m_dirtyGpsTab)
    {
        m_gpsTab->setCurrentURL(m_currentURL);
        m_dirtyGpsTab = true;
    }

    m_gpsTab->setActive(tab == m_gpsTab);
#endif // HAVE_MARBLE

    unsetCursor();
}

void ItemPropertiesSideBar::setImagePropertiesInformation(const QUrl& url)
{
    if (!url.isValid())
    {
        return;
    }

    QString str;
    QString unavailable(i18n("<i>unavailable</i>"));
    QFileInfo fileInfo(url.toLocalFile());
    DMetadata metaData(url.toLocalFile());

    // -- File system information -----------------------------------------

    QDateTime modifiedDate = fileInfo.lastModified();
    str = QLocale().toString(modifiedDate, QLocale::ShortFormat);
    m_propertiesTab->setFileModifiedDate(str);

    str = QString::fromUtf8("%1 (%2)").arg(ItemPropertiesTab::humanReadableBytesCount(fileInfo.size()))
                                      .arg(QLocale().toString(fileInfo.size()));
    m_propertiesTab->setFileSize(str);
    m_propertiesTab->setFileOwner(QString::fromUtf8("%1 - %2").arg(fileInfo.owner()).arg(fileInfo.group()));
    m_propertiesTab->setFilePermissions(ItemPropertiesTab::permissionsString(fileInfo));

    // -- Image Properties --------------------------------------------------

    QSize   dims;
    QString bitDepth, colorMode;
    QString rawFilesExt = QLatin1String(DRawDecoder::rawFiles());
    QString ext         = fileInfo.suffix().toUpper();

    if (!ext.isEmpty() && rawFilesExt.toUpper().contains(ext))
    {
        m_propertiesTab->setImageMime(i18n("RAW Image"));
        bitDepth    = QLatin1String("48");
        dims        = metaData.getItemDimensions();
        colorMode   = i18n("Uncalibrated");
    }
    else
    {
        m_propertiesTab->setImageMime(QMimeDatabase().mimeTypeForFile(fileInfo).comment());

        dims = metaData.getPixelSize();

        DImg img;
        img.loadItemInfo(url.toLocalFile(), false, false, false, false);
        bitDepth.number(img.originalBitDepth());
        colorMode = DImg::colorModelToString(img.originalColorModel());
    }

    QString mpixels;
    mpixels.setNum(dims.width()*dims.height()/1000000.0, 'f', 2);
    str = (!dims.isValid()) ? i18n("Unknown") : i18n("%1x%2 (%3Mpx)",
            dims.width(), dims.height(), mpixels);
    m_propertiesTab->setItemDimensions(str);

    if (!dims.isValid()) str = i18n("Unknown");
    else m_propertiesTab->aspectRatioToString(dims.width(), dims.height(), str);

    m_propertiesTab->setImageRatio(str);
    m_propertiesTab->setImageBitDepth(bitDepth.isEmpty()   ? unavailable : i18n("%1 bpp", bitDepth));
    m_propertiesTab->setImageColorMode(colorMode.isEmpty() ? unavailable : colorMode);

    // -- Photograph information ------------------------------------------

    PhotoInfoContainer photoInfo = metaData.getPhotographInformation();

    m_propertiesTab->setPhotoInfoDisable(photoInfo.isEmpty());
    ItemPropertiesTab::shortenedMakeInfo(photoInfo.make);
    ItemPropertiesTab::shortenedModelInfo(photoInfo.model);
    m_propertiesTab->setPhotoMake(photoInfo.make.isEmpty()   ? unavailable : photoInfo.make);
    m_propertiesTab->setPhotoModel(photoInfo.model.isEmpty() ? unavailable : photoInfo.model);

    if (photoInfo.dateTime.isValid())
    {
        str = QLocale().toString(photoInfo.dateTime, QLocale::ShortFormat);
        m_propertiesTab->setPhotoDateTime(str);
    }
    else
    {
        m_propertiesTab->setPhotoDateTime(unavailable);
    }

    m_propertiesTab->setPhotoLens(photoInfo.lens.isEmpty()         ? unavailable : photoInfo.lens);
    m_propertiesTab->setPhotoAperture(photoInfo.aperture.isEmpty() ? unavailable : photoInfo.aperture);

    if (photoInfo.focalLength35mm.isEmpty())
    {
        m_propertiesTab->setPhotoFocalLength(photoInfo.focalLength.isEmpty() ? unavailable : photoInfo.focalLength);
    }
    else
    {
        str = i18n("%1 (%2)", photoInfo.focalLength, photoInfo.focalLength35mm);
        m_propertiesTab->setPhotoFocalLength(str);
    }

    m_propertiesTab->setPhotoExposureTime(photoInfo.exposureTime.isEmpty() ? unavailable : photoInfo.exposureTime);
    m_propertiesTab->setPhotoSensitivity(photoInfo.sensitivity.isEmpty()   ? unavailable : i18n("%1 ISO", photoInfo.sensitivity));

    if (photoInfo.exposureMode.isEmpty() && photoInfo.exposureProgram.isEmpty())
    {
        m_propertiesTab->setPhotoExposureMode(unavailable);
    }
    else if (!photoInfo.exposureMode.isEmpty() && photoInfo.exposureProgram.isEmpty())
    {
        m_propertiesTab->setPhotoExposureMode(photoInfo.exposureMode);
    }
    else if (photoInfo.exposureMode.isEmpty() && !photoInfo.exposureProgram.isEmpty())
    {
        m_propertiesTab->setPhotoExposureMode(photoInfo.exposureProgram);
    }
    else
    {
        str = QString::fromUtf8("%1 / %2").arg(photoInfo.exposureMode).arg(photoInfo.exposureProgram);
        m_propertiesTab->setPhotoExposureMode(str);
    }

    m_propertiesTab->setPhotoFlash(photoInfo.flash.isEmpty()               ? unavailable : photoInfo.flash);
    m_propertiesTab->setPhotoWhiteBalance(photoInfo.whiteBalance.isEmpty() ? unavailable : photoInfo.whiteBalance);

    // -- Audio/Video information ------------------------------------------

    VideoInfoContainer videoInfo = metaData.getVideoInformation();

    m_propertiesTab->setVideoInfoDisable(videoInfo.isEmpty());

    m_propertiesTab->setVideoAspectRatio(videoInfo.aspectRatio.isEmpty()           ? unavailable : videoInfo.aspectRatio);
    m_propertiesTab->setVideoDuration(videoInfo.duration.isEmpty()                 ? unavailable : videoInfo.duration);
    m_propertiesTab->setVideoFrameRate(videoInfo.frameRate.isEmpty()               ? unavailable : videoInfo.frameRate);
    m_propertiesTab->setVideoVideoCodec(videoInfo.videoCodec.isEmpty()             ? unavailable : videoInfo.videoCodec);
    m_propertiesTab->setVideoAudioBitRate(videoInfo.audioBitRate.isEmpty()         ? unavailable : videoInfo.audioBitRate);
    m_propertiesTab->setVideoAudioChannelType(videoInfo.audioChannelType.isEmpty() ? unavailable : videoInfo.audioChannelType);
    m_propertiesTab->setVideoAudioCodec(videoInfo.audioCodec.isEmpty()             ? unavailable : videoInfo.audioCodec);

    // -- Caption, ratings, tag information ---------------------

    CaptionsMap captions = metaData.getItemComments();
    QString caption;

    if (captions.contains(QLatin1String("x-default")))
        caption = captions.value(QLatin1String("x-default")).caption;
    else if (!captions.isEmpty())
        caption = captions.begin().value().caption;

    m_propertiesTab->setCaption(caption);

    m_propertiesTab->setRating(metaData.getItemRating());

    QStringList tagPaths;
    metaData.getItemTagsPath(tagPaths);
    m_propertiesTab->setTags(tagPaths);
    m_propertiesTab->showOrHideCaptionAndTags();
}

void ItemPropertiesSideBar::doLoadState()
{
    Sidebar::doLoadState();

    /// @todo m_propertiesTab should load its settings from our group
    m_propertiesTab->setObjectName(QLatin1String("Image Properties SideBar Expander"));

    KConfigGroup group = getConfigGroup();

    m_propertiesTab->readSettings(group);

#ifdef HAVE_MARBLE
    const KConfigGroup groupGPSTab      = KConfigGroup(&group, entryName(QLatin1String("GPS Properties Tab")));
    m_gpsTab->readSettings(groupGPSTab);
#endif // HAVE_MARBLE

    const KConfigGroup groupColorTab    = KConfigGroup(&group, entryName(QLatin1String("Color Properties Tab")));
    m_colorTab->readSettings(groupColorTab);

    const KConfigGroup groupMetadataTab = KConfigGroup(&group, entryName(QLatin1String("Metadata Properties Tab")));
    m_metadataTab->readSettings(groupMetadataTab);
}

void ItemPropertiesSideBar::doSaveState()
{
    Sidebar::doSaveState();

    KConfigGroup group = getConfigGroup();

    m_propertiesTab->writeSettings(group);

#ifdef HAVE_MARBLE
    KConfigGroup groupGPSTab      = KConfigGroup(&group, entryName(QLatin1String("GPS Properties Tab")));
    m_gpsTab->writeSettings(groupGPSTab);
#endif // HAVE_MARBLE

    KConfigGroup groupColorTab    = KConfigGroup(&group, entryName(QLatin1String("Color Properties Tab")));
    m_colorTab->writeSettings(groupColorTab);

    KConfigGroup groupMetadataTab = KConfigGroup(&group, entryName(QLatin1String("Metadata Properties Tab")));
    m_metadataTab->writeSettings(groupMetadataTab);
}

void ItemPropertiesSideBar::slotLoadMetadataFilters()
{
    m_metadataTab->loadFilters();
}

} // namespace Digikam
