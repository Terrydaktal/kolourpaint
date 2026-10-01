/*
   SPDX-FileCopyrightText: 2026 Terrydaktal <9lewis9@gmail.com>

   SPDX-License-Identifier: BSD-2-Clause
*/

#include "kpImageFileDrag.h"

#include <QDateTime>
#include <QDir>
#include <QDrag>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QMimeData>
#include <QPixmap>
#include <QStandardPaths>
#include <QString>
#include <QTemporaryFile>
#include <QUrl>

#include <KLocalizedString>
#include <KMessageBox>

namespace {

constexpr qint64 TemporaryDragMaxAgeSeconds = 24 * 60 * 60;

QString temporaryDragDirectoryPath()
{
    QString basePath = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (basePath.isEmpty()) {
        basePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    }

    return QDir(basePath).filePath(QStringLiteral("kolourpaint-drag-exports"));
}

QString createTemporaryDragImage(const QImage &image)
{
    const QString directoryPath = temporaryDragDirectoryPath();
    if (directoryPath.isEmpty() || !QDir().mkpath(directoryPath)) {
        return {};
    }

    kpImageFileDrag::cleanupTemporaryFiles();

    QTemporaryFile temporaryFile(
        QDir(directoryPath).filePath(QStringLiteral("kolourpaint-drag-XXXXXX.png")));
    if (!temporaryFile.open() || !image.save(&temporaryFile, "PNG") || !temporaryFile.flush()) {
        return {};
    }

    const QString filePath = temporaryFile.fileName();
    temporaryFile.setAutoRemove(false);
    temporaryFile.close();
    return filePath;
}

QString shellQuotedPath(QString filePath)
{
    filePath.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QLatin1Char('\'') + filePath + QLatin1Char('\'');
}

} // namespace

void kpImageFileDrag::cleanupTemporaryFiles()
{
    QDir dragDirectory(temporaryDragDirectoryPath());
    if (!dragDirectory.exists()) {
        return;
    }

    const QDateTime now = QDateTime::currentDateTimeUtc();
    const QFileInfoList files =
        dragDirectory.entryInfoList({QStringLiteral("kolourpaint-drag-*.png")}, QDir::Files | QDir::NoSymLinks);
    for (const QFileInfo &file : files) {
        if (file.lastModified().secsTo(now) > TemporaryDragMaxAgeSeconds) {
            QFile::remove(file.absoluteFilePath());
        }
    }
}

Qt::DropAction kpImageFileDrag::start(QObject *source,
                                      QWidget *errorParent,
                                      const QImage &image,
                                      QMimeData *mimeData,
                                      const QString &existingFilePath,
                                      const QPoint &imageHotSpot)
{
    if (!source || image.isNull()) {
        delete mimeData;
        return Qt::IgnoreAction;
    }

    QString filePath = existingFilePath;
    const bool isTemporary = filePath.isEmpty();
    if (isTemporary) {
        filePath = createTemporaryDragImage(image);
        if (filePath.isEmpty()) {
            delete mimeData;
            KMessageBox::error(errorParent, i18n("Could not create a temporary image file for dragging."));
            return Qt::IgnoreAction;
        }
    }

    if (!mimeData) {
        mimeData = new QMimeData();
    }
    mimeData->setUrls({QUrl::fromLocalFile(filePath)});
    mimeData->setText(shellQuotedPath(filePath));
    mimeData->setImageData(image);

    QDrag drag(source);
    drag.setMimeData(mimeData);

    const QImage preview = image.scaled(QSize(128, 128), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    drag.setPixmap(QPixmap::fromImage(preview));
    if (image.rect().contains(imageHotSpot)) {
        drag.setHotSpot(QPoint(imageHotSpot.x() * preview.width() / image.width(),
                               imageHotSpot.y() * preview.height() / image.height()));
    } else {
        drag.setHotSpot(QPoint(preview.width() / 2, preview.height() / 2));
    }

    const Qt::DropAction result = drag.exec(Qt::CopyAction, Qt::CopyAction);
    if (isTemporary && result == Qt::IgnoreAction) {
        QFile::remove(filePath);
    }

    return result;
}
