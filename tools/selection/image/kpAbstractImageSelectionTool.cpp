
/*
   SPDX-FileCopyrightText: 2003-2007 Clarence Dang <dang@kde.org>

   SPDX-License-Identifier: BSD-2-Clause
*/

#include "kpAbstractImageSelectionTool.h"

#include <QHash>
#include <QImage>
#include <QRegion>

#include <KLocalizedString>

#include "commands/tools/selection/kpToolSelectionPullFromDocumentCommand.h"
#include "document/kpDocument.h"
#include "environments/tools/selection/kpToolSelectionEnvironment.h"
#include "layers/selections/image/kpAbstractImageSelection.h"

//---------------------------------------------------------------------

namespace {

kpColor InferSelectionBackgroundColor(const QImage &image,
                                      const kpAbstractImageSelection &selection,
                                      const kpColor &fallback)
{
    const QRegion selectionRegion = selection.shapeRegion();
    const QRegion surroundingRegion = selectionRegion.translated(-1, 0)
        .united(selectionRegion.translated(1, 0))
        .united(selectionRegion.translated(0, -1))
        .united(selectionRegion.translated(0, 1))
        .subtracted(selectionRegion)
        .intersected(image.rect());

    QHash<QRgb, int> colorCounts;
    int sampleCount = 0;
    for (const QRect &sampleRect : surroundingRegion) {
        for (int y = sampleRect.top(); y <= sampleRect.bottom(); y++) {
            for (int x = sampleRect.left(); x <= sampleRect.right(); x++) {
                colorCounts[image.pixel(x, y)]++;
                sampleCount++;
            }
        }
    }

    if (sampleCount == 0) {
        return fallback;
    }

    QRgb dominantColor = 0;
    int dominantCount = 0;
    for (auto it = colorCounts.cbegin(); it != colorCounts.cend(); ++it) {
        if (it.value() > dominantCount) {
            dominantColor = it.key();
            dominantCount = it.value();
        }
    }

    constexpr double minimumDominantColorFraction = 0.6;
    if (static_cast<double>(dominantCount) / sampleCount < minimumDominantColorFraction) {
        return fallback;
    }

    return kpColor(dominantColor);
}

} // namespace

//---------------------------------------------------------------------

kpAbstractImageSelectionTool::kpAbstractImageSelectionTool(const QString &text,
                                                           const QString &description,
                                                           int key,
                                                           kpToolSelectionEnvironment *environ,
                                                           QObject *parent,
                                                           const QString &name)
    : kpAbstractSelectionTool(text, description, key, environ, parent, name)
{
}

//---------------------------------------------------------------------

// protected virtual [kpAbstractSelectionTool]
kpAbstractSelectionContentCommand *kpAbstractImageSelectionTool::newGiveContentCommand() const
{
    kpAbstractImageSelection *imageSel = document()->imageSelection();
    Q_ASSERT(imageSel && !imageSel->hasContent());

    if (imageSel->transparency().isTransparent()) {
        environ()->flashColorSimilarityToolBarItem();
    }

    const kpColor backgroundColor =
        InferSelectionBackgroundColor(document()->image(), *imageSel, environ()->backgroundColor());

    return new kpToolSelectionPullFromDocumentCommand(*imageSel,
                                                      backgroundColor,
                                                      QString() /*uninteresting child of macro cmd*/,
                                                      environ()->commandEnvironment());
}

//---------------------------------------------------------------------
// protected virtual [kpAbstractSelectionTool]

QString kpAbstractImageSelectionTool::nameOfCreateCommand() const
{
    return i18n("Selection: Create");
}

//---------------------------------------------------------------------
// protected virtual [kpAbstractSelectionTool]

QString kpAbstractImageSelectionTool::haventBegunDrawUserMessageCreate() const
{
    // TODO: This is wrong because you can still use RMB.
    return i18n("Left drag to create selection.");
}

//---------------------------------------------------------------------
// protected virtual [kpAbstractSelectionTool]

QString kpAbstractImageSelectionTool::haventBegunDrawUserMessageMove() const
{
    return i18n("Left drag to move selection.");
}

//---------------------------------------------------------------------
// protected virtual [kpAbstractSelectionTool]

QString kpAbstractImageSelectionTool::haventBegunDrawUserMessageResizeScale() const
{
    return i18n("Left drag to scale selection.");
}

//---------------------------------------------------------------------

#include "moc_kpAbstractImageSelectionTool.cpp"
