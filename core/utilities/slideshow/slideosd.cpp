/* ============================================================
 *
 * This file is a part of digiKam project
 * https://www.digikam.org
 *
 * Date        : 2014-09-18
 * Description : slideshow OSD widget
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

#include "slideosd.h"

// Qt includes

#include <QTimer>
#include <QLayout>
#include <QDesktopWidget>
#include <QEvent>
#include <QStyle>
#include <QApplication>
#include <QProgressBar>

// Windows includes

#ifdef Q_OS_WIN
#   include <windows.h>
#endif

// Local includes

#include "digikam_debug.h"
#include "slideshow.h"
#include "slidetoolbar.h"
#include "slideproperties.h"
#include "ratingwidget.h"
#include "colorlabelwidget.h"
#include "picklabelwidget.h"
#include "dinfointerface.h"

namespace Digikam
{

class Q_DECL_HIDDEN SlideOSD::Private
{
public:

    explicit Private()
      : paused(false),
        video(false),
        blink(false),
        refresh(1000),       // Progress bar refresh in ms
        progressBar(nullptr),
        progressTimer(nullptr),
        labelsBox(nullptr),
        progressBox(nullptr),
        parent(nullptr),
        slideProps(nullptr),
        toolBar(nullptr),
        ratingWidget(nullptr),
        clWidget(nullptr),
        plWidget(nullptr)
    {
    }

    bool                paused;
    bool                video;
    bool                blink;
    int const           refresh;

    QProgressBar*       progressBar;
    QTimer*             progressTimer;

    DHBox*              labelsBox;
    DHBox*              progressBox;

    SlideShow*          parent;
    SlideProperties*    slideProps;
    SlideToolBar*       toolBar;
    RatingWidget*       ratingWidget;
    ColorLabelSelector* clWidget;
    PickLabelSelector*  plWidget;
    SlideShowSettings   settings;
};

SlideOSD::SlideOSD(const SlideShowSettings& settings, SlideShow* const parent)
    : QWidget(parent),
      d(new Private)
{
    Qt::WindowFlags flags = Qt::FramelessWindowHint  |
                            Qt::WindowStaysOnTopHint |
                            Qt::X11BypassWindowManagerHint;

    setWindowFlags(flags);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setMouseTracking(true);

    d->settings   = settings;
    d->parent     = parent;
    d->slideProps = new SlideProperties(d->settings, this);
    d->slideProps->installEventFilter(d->parent);

    // ---------------------------------------------------------------

    d->labelsBox    = new DHBox(this);

    d->clWidget     = new ColorLabelSelector(d->labelsBox);
    d->clWidget->installEventFilter(this);
    d->clWidget->installEventFilter(d->parent);
    d->clWidget->colorLabelWidget()->installEventFilter(this);
    d->clWidget->setFocusPolicy(Qt::NoFocus);
    d->clWidget->setMouseTracking(true);

    d->plWidget     = new PickLabelSelector(d->labelsBox);
    d->plWidget->installEventFilter(this);
    d->plWidget->installEventFilter(d->parent);
    d->plWidget->setFocusPolicy(Qt::NoFocus);
    d->plWidget->pickLabelWidget()->installEventFilter(this);
    d->plWidget->setMouseTracking(true);

    d->ratingWidget = new RatingWidget(d->labelsBox);
    d->ratingWidget->setTracking(false);
    d->ratingWidget->setFading(false);
    d->ratingWidget->installEventFilter(this);
    d->ratingWidget->installEventFilter(d->parent);
    d->ratingWidget->setFocusPolicy(Qt::NoFocus);
    d->ratingWidget->setMouseTracking(true);

    d->labelsBox->layout()->setAlignment(d->ratingWidget, Qt::AlignVCenter | Qt::AlignLeft);
    d->labelsBox->installEventFilter(d->parent);
    d->labelsBox->setMouseTracking(true);

    d->labelsBox->setVisible(d->settings.printLabels || d->settings.printRating);
    d->ratingWidget->setVisible(d->settings.printRating);
    d->clWidget->setVisible(d->settings.printLabels);
    d->plWidget->setVisible(d->settings.printLabels);

    connect(d->ratingWidget, SIGNAL(signalRatingChanged(int)),
            parent, SLOT(slotAssignRating(int)));

    connect(d->clWidget, SIGNAL(signalColorLabelChanged(int)),
            parent, SLOT(slotAssignColorLabel(int)));

    connect(d->plWidget, SIGNAL(signalPickLabelChanged(int)),
            parent, SLOT(slotAssignPickLabel(int)));

    // ---------------------------------------------------------------

    d->progressBox   = new DHBox(this);
    d->progressBox->setVisible(d->settings.showProgressIndicator);
    d->progressBox->installEventFilter(d->parent);
    d->progressBox->setMouseTracking(true);

    d->progressBar   = new QProgressBar(d->progressBox);
    d->progressBar->setMinimum(0);
    d->progressBar->setMaximum(d->settings.delay);
    d->progressBar->setFocusPolicy(Qt::NoFocus);
    d->progressBar->installEventFilter(d->parent);
    d->progressBar->setMouseTracking(true);

    d->toolBar       = new SlideToolBar(d->settings, d->progressBox);
    d->toolBar->installEventFilter(this);
    d->toolBar->installEventFilter(d->parent);

    connect(d->toolBar, SIGNAL(signalPause()),
            d->parent, SLOT(slotPause()));

    connect(d->toolBar, SIGNAL(signalPlay()),
            d->parent, SLOT(slotPlay()));

    connect(d->toolBar, SIGNAL(signalNext()),
            d->parent, SLOT(slotLoadNextItem()));

    connect(d->toolBar, SIGNAL(signalPrev()),
            d->parent, SLOT(slotLoadPrevItem()));

    connect(d->toolBar, SIGNAL(signalClose()),
            d->parent, SLOT(close()));

    connect(d->toolBar, SIGNAL(signalScreenSelected(int)),
            d->parent, SLOT(slotScreenSelected(int)));

    // ---------------------------------------------------------------

    QGridLayout* const grid = new QGridLayout(this);
    grid->addWidget(d->slideProps,  1, 0, 1, 2);
    grid->addWidget(d->labelsBox,   2, 0, 1, 1);
    grid->addWidget(d->progressBox, 3, 0, 1, 1);
    grid->setRowStretch(0, 10);
    grid->setColumnStretch(1, 10);
    grid->setContentsMargins(QMargins());
    grid->setSpacing(QApplication::style()->pixelMetric(QStyle::PM_DefaultLayoutSpacing));

    // ---------------------------------------------------------------

    d->progressTimer = new QTimer(this);
    d->progressTimer->setSingleShot(false);

    connect(d->progressTimer, SIGNAL(timeout()),
            this, SLOT(slotProgressTimer()));

    QTimer::singleShot(500, this, SLOT(slotStart()));
}

SlideOSD::~SlideOSD()
{
    d->progressTimer->stop();

    delete d;
}

void SlideOSD::slotStart()
{
    d->parent->slotLoadNextItem();
    d->progressTimer->start(d->refresh);
    pause(!d->settings.autoPlayEnabled);
}

SlideToolBar* SlideOSD::toolBar() const
{
    return d->toolBar;
}

void SlideOSD::setCurrentUrl(const QUrl& url)
{
    DInfoInterface::DInfoMap info = d->settings.iface->itemInfo(url);
    DItemInfo item(info);

    // Update info text.

    d->slideProps->setCurrentUrl(url);

    // Display Labels.

    if (d->settings.printLabels)
    {
        d->clWidget->blockSignals(true);
        d->plWidget->blockSignals(true);
        d->clWidget->setColorLabel((ColorLabel)item.colorLabel());
        d->plWidget->setPickLabel((PickLabel)item.pickLabel());
        d->clWidget->blockSignals(false);
        d->plWidget->blockSignals(false);
    }

    if (d->settings.printRating)
    {
        d->ratingWidget->blockSignals(true);
        d->ratingWidget->setRating(item.rating());
        d->ratingWidget->blockSignals(false);
    }

    // Make the OSD the proper size
    layout()->activate();
    resize(sizeHint());

    QRect geometry(QApplication::desktop()->availableGeometry(parentWidget()));
    move(10, geometry.bottom() - height());
    show();
    raise();
}

bool SlideOSD::eventFilter(QObject* obj, QEvent* ev)
{
    if (obj == d->labelsBox                    ||
        obj == d->ratingWidget                 ||
        obj == d->clWidget                     ||
        obj == d->plWidget                     ||
        obj == d->clWidget->colorLabelWidget() ||
        obj == d->plWidget->pickLabelWidget())
    {
        if (ev->type() == QEvent::Enter)
        {
            d->paused = isPaused();
            d->parent->slotPause();
            return false;
        }

        if (ev->type() == QEvent::Leave)
        {
            if (!d->paused)
            {
                d->parent->slotPlay();
            }

            return false;
        }
    }

    // pass the event on to the parent class
    return QWidget::eventFilter(obj, ev);
}

void SlideOSD::slotProgressTimer()
{
    QString str = QString::fromUtf8("(%1/%2)")
                    .arg(QString::number(d->settings.fileList.indexOf(d->parent->currentItem()) + 1))
                    .arg(QString::number(d->settings.fileList.count()));

    if (isPaused())
    {
        d->blink = !d->blink;

        if (d->blink)
        {
            str = QString();
        }

        d->progressBar->setFormat(str);
    }
    else if (d->video)
    {
        d->progressBar->setFormat(str);
        return;
    }
    else
    {
        d->progressBar->setFormat(str);
        d->progressBar->setValue(d->progressBar->value()+1);

        if (d->progressBar->value() == d->settings.delay)
        {
            d->parent->slotLoadNextItem();
        }
    }
}

void SlideOSD::pause(bool b)
{
    d->toolBar->pause(b);

    if (!b)
    {
        d->progressBar->setValue(0);
    }
}

void SlideOSD::video(bool b)
{
    d->video = b;
}

bool SlideOSD::isPaused() const
{
    return d->toolBar->isPaused();
}

bool SlideOSD::isUnderMouse() const
{
    return (d->ratingWidget->underMouse() ||
            d->progressBar->underMouse()  ||
            d->clWidget->underMouse()     ||
            d->plWidget->underMouse()     ||
            d->toolBar->underMouse());
}

void SlideOSD::toggleProperties()
{
    d->slideProps->togglePaintEnabled();
}

} // namespace Digikam
