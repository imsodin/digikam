/* ============================================================
 *
 * This file is a part of digiKam project
 * https://www.digikam.org
 *
 * Date        : 2014-09-19
 * Description : slide properties widget
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

#include "slideproperties.h"

// Qt includes

#include <QDateTime>
#include <QPainter>
#include <QPixmap>
#include <QTextDocument>
#include <QDesktopWidget>
#include <QApplication>
#include <QLocale>

// KDE includes

#include <klocalizedstring.h>

// Local includes

#include "digikam_debug.h"
#include "dinfointerface.h"
#include "itempropertiestab.h"

namespace Digikam
{

class Q_DECL_HIDDEN SlideProperties::Private
{
public:

    explicit Private()
        : maxStringLen(80),
         paintEnabled(true)
    {
    }

    const int         maxStringLen;
    bool              paintEnabled;

    QUrl              url;

    SlideShowSettings settings;
};

SlideProperties::SlideProperties(const SlideShowSettings& settings, QWidget* const parent)
    : QWidget(parent),
      d(new Private)
{
    d->settings = settings;
    setFixedSize(QApplication::desktop()->availableGeometry(parentWidget()).size() / 1.5);
    setMouseTracking(true);
}

SlideProperties::~SlideProperties()
{
    delete d;
}

void SlideProperties::setCurrentUrl(const QUrl& url)
{
    d->url  = url;
    update();
}

void SlideProperties::paintEvent(QPaintEvent*)
{
    if (!d->paintEnabled)
    {
        return;
    }

    QPainter p(this);
    p.setFont(d->settings.captionFont);

    DInfoInterface::DInfoMap info = d->settings.iface->itemInfo(d->url);
    DItemInfo item(info);
 
    QString str;
    //PhotoInfoContainer photoInfo = d->info.photoInfo;
    QString comment  = item.comment();
    QString title    = item.title();
    QStringList tags = item.keywords();
    int offset       = 0;

    // Display tag names.

    if (d->settings.printTags)
    {
        printTags(p, offset, tags);
    }

    // Display Titles.

    if (d->settings.printTitle)
    {
        str.clear();

        if (!title.isEmpty())
        {
            str += title;
            printInfoText(p, offset, str);
        }
    }

    // Display Captions if no Titles.

    if (d->settings.printCapIfNoTitle)
    {
        str.clear();

        if (title.isEmpty())
        {
            str += comment;
            printComments(p, offset, str);
        }
    }

    // Display Comments.

    if (d->settings.printComment)
    {
        str = comment;
        printComments(p, offset, str);
    }

    // Display Make and Model.

    if (d->settings.printMakeModel)
    {
        str.clear();

        QString make  = item.make();
        QString model = item.model();

        if (!make.isEmpty())
        {
            ItemPropertiesTab::shortenedMakeInfo(make);
            str = make;
        }

        if (!model.isEmpty())
        {
            if (!make.isEmpty())
            {
                str += QLatin1String(" / ");
            }

            ItemPropertiesTab::shortenedModelInfo(model);
            str += model;
        }

        printInfoText(p, offset, str);
    }

    // Display Exposure and Sensitivity.

    if (d->settings.printExpoSensitivity)
    {
        str.clear();

        QString exposureTime = item.exposureTime();
        QString sensitivity  = item.sensitivity();

        if (!exposureTime.isEmpty())
        {
            str = exposureTime;
        }

        if (!sensitivity.isEmpty())
        {
            if (!exposureTime.isEmpty())
            {
                str += QLatin1String(" / ");
            }

            str += i18n("%1 ISO", sensitivity);
        }

        printInfoText(p, offset, str);
    }

    // Display Aperture and Focal.

    if (d->settings.printApertureFocal)
    {
        str.clear();

        QString aperture        = item.aperture();
        QString focalLength     = item.focalLength();
        QString focalLength35mm = item.focalLength35mm();

        if (!aperture.isEmpty())
        {
            str = aperture;
        }

        if (focalLength35mm.isEmpty())
        {
            if (!focalLength.isEmpty())
            {
                if (!aperture.isEmpty())
                {
                    str += QLatin1String(" / ");
                }

                str += focalLength;
            }
        }
        else
        {
            if (!aperture.isEmpty())
            {
                str += QLatin1String(" / ");
            }

            if (!focalLength.isEmpty())
            {
                str += QString::fromUtf8("%1 (%2)").arg(focalLength).arg(focalLength35mm);
            }
            else
            {
                str += QString::fromUtf8("%1").arg(focalLength35mm);
            }
        }

        printInfoText(p, offset, str);
    }

    // Display Creation Date.

    if (d->settings.printDate)
    {
        QDateTime dateTime = item.dateTime();

        if (dateTime.isValid())
        {
            str = QLocale().toString(dateTime, QLocale::ShortFormat);
            printInfoText(p, offset, str);
        }
    }

    // Display image File Name.

    if (d->settings.printName)
    {
        printInfoText(p, offset, d->url.fileName());
    }
}

void SlideProperties::printInfoText(QPainter& p, int& offset, const QString& str, const QColor& pcol)
{
    if (!str.isEmpty())
    {
        offset += QFontMetrics(p.font()).lineSpacing();
        p.setPen(Qt::black);

        for (int x = -1; x <= 1; ++x)
        {
            for (int y = offset + 1; y >= offset - 1; --y)
            {
                p.drawText(x, p.window().height() - y, str);
            }
        }

        p.setPen(pcol);
        p.drawText(0, p.window().height() - offset, str);
    }
}

void SlideProperties::printComments(QPainter& p, int& offset, const QString& comments)
{
    QStringList commentsByLines;

    uint commentsIndex = 0;     // Comments QString index

    while (commentsIndex < (uint)comments.length())
    {
        QString newLine;
        bool breakLine = false; // End Of Line found
        uint currIndex;         // Comments QString current index

        // Check minimal lines dimension

        uint commentsLinesLengthLocal = d->maxStringLen;

        for (currIndex = commentsIndex ;
             currIndex < (uint)comments.length() && !breakLine ; ++currIndex)
        {
            if (comments.at(currIndex) == QLatin1Char('\n') || comments.at(currIndex).isSpace())
            {
                breakLine = true;
            }
        }

        if (commentsLinesLengthLocal <= (currIndex - commentsIndex))
        {
            commentsLinesLengthLocal = (currIndex - commentsIndex);
        }

        breakLine = false;

        for (currIndex = commentsIndex ;
             currIndex <= commentsIndex + commentsLinesLengthLocal &&
             currIndex < (uint)comments.length() && !breakLine ;
             ++currIndex)
        {
            breakLine = (comments.at(currIndex) == QLatin1Char('\n')) ? true : false;

            if (breakLine)
            {
                newLine.append(QLatin1Char(' '));
            }
            else
            {
                newLine.append(comments.at(currIndex));
            }
        }

        commentsIndex = currIndex; // The line is ended

        if (commentsIndex != (uint)comments.length())
        {
            while (!newLine.endsWith(QLatin1Char(' ')))
            {
                newLine.truncate(newLine.length() - 1);
                --commentsIndex;
            }
        }

        commentsByLines.prepend(newLine.trimmed());
    }

    for (int i = 0 ; i < (int)commentsByLines.count() ; ++i)
    {
        printInfoText(p, offset, commentsByLines.at(i));
    }
}

void SlideProperties::printTags(QPainter& p, int& offset, QStringList& tags)
{
    tags.sort();

    QString str = tags.join(QLatin1String(", "));

    if (!str.isEmpty())
    {
        printInfoText(p, offset, str, qApp->palette().color(QPalette::Link).name());
    }
}

void SlideProperties::togglePaintEnabled()
{
    d->paintEnabled = !d->paintEnabled;
    update();
}

} // namespace Digikam
