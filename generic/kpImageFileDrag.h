/*
   SPDX-FileCopyrightText: 2026 Terrydaktal <9lewis9@gmail.com>

   SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef kpImageFileDrag_H
#define kpImageFileDrag_H

#include <QPoint>
#include <QString>
#include <QtCore/Qt>

class QImage;
class QMimeData;
class QObject;
class QWidget;

class kpImageFileDrag
{
public:
    static void cleanupTemporaryFiles();

    // Takes ownership of mimeData when it is not null.
    static Qt::DropAction start(QObject *source,
                                QWidget *errorParent,
                                const QImage &image,
                                QMimeData *mimeData = nullptr,
                                const QString &existingFilePath = QString(),
                                const QPoint &imageHotSpot = QPoint(-1, -1));
};

#endif // kpImageFileDrag_H
