/* ============================================================
 *
 * This file is a part of digiKam project
 * https://www.digikam.org
 *
 * Date        : 2004-12-23
 * Description : a tool to shear an image
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

#ifndef DIGIKAM_EDITOR_SHEAR_TOOL_H
#define DIGIKAM_EDITOR_SHEAR_TOOL_H

// Local includes

#include "editortool.h"

using namespace Digikam;

namespace DigikamEditorShearToolPlugin
{

class ShearTool : public EditorToolThreaded
{
    Q_OBJECT

public:

    explicit ShearTool(QObject* const parent);
    ~ShearTool();

private Q_SLOTS:

    void slotResetSettings();
    void slotColorGuideChanged();

private:

    void readSettings();
    void writeSettings();
    void preparePreview();
    void prepareFinal();
    void setPreviewImage();
    void setFinalImage();

private:

    class Private;
    Private* const d;
};

} // namespace DigikamEditorShearToolPlugin

#endif // DIGIKAM_EDITOR_SHEAR_TOOL_H
