/* ============================================================
 *
 * This file is a part of digiKam project
 * https://www.digikam.org
 *
 * Date        : 2005-06-14
 * Description : A QImage loader for DImg framework.
 *
 * Copyright (C) 2005      by Renchi Raju <renchi dot raju at gmail dot com>
 * Copyright (C) 2006-2019 by Caulier Gilles <caulier dot gilles at gmail dot com>
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

#ifndef DIGIKAM_QIMAGE_LOADER_H
#define DIGIKAM_QIMAGE_LOADER_H

// Local includes

#include "dimgloader.h"
#include "digikam_export.h"
#include "digikam_config.h"

namespace Digikam
{
class DImg;

class DIGIKAM_EXPORT QImageLoader : public DImgLoader
{
public:

    explicit QImageLoader(DImg* const image);

    virtual bool load(const QString& filePath, DImgLoaderObserver* const observer) override;
    virtual bool save(const QString& filePath, DImgLoaderObserver* const observer) override;

    virtual bool hasAlpha()   const override;
    virtual bool sixteenBit() const override;
    virtual bool isReadOnly() const override;

private:

    bool m_hasAlpha;
};

} // namespace Digikam

#endif // DIGIKAM_QIMAGE_LOADER_H
