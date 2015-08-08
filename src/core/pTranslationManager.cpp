/****************************************************************************
**
**      Created using Monkey Studio IDE v1.8.4.0 (1.8.4.0)
** Authors   : Filipe Azevedo aka Nox P@sNox <pasnox@gmail.com>
** Project   : Fresh Library
** FileName  : pTranslationManager.cpp
** Date      : 2011-02-20T00:44:25
** License   : LGPL v3
** Home Page : https://github.com/pasnox/fresh
** Comment   : Fresh Library is a Qt 4 extension library providing set of new core & gui classes.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Leser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This package is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program. If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/
#include "pTranslationManager.h"

#include <QDir>
#include <QCoreApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QDebug>

Q_GLOBAL_STATIC( pTranslationManager, translationManagerInstance );

pTranslationManager* pTranslationManager::instance()
{
    return translationManagerInstance();
}

pTranslationManager::pTranslationManager( QObject* parent )
    : QObject( parent )
{
    mCurrentLocale = systemLocale();
    mSystemTranslationsPaths << QLibraryInfo::location( QLibraryInfo::TranslationsPath );
    mFakeCLocaleEnabled = false;
}

bool pTranslationManager::addTranslator( const QString& filePath, const QLocale& locale )
{
    QTranslator* translator = new QTranslator( QCoreApplication::instance() );

    if ( translator->load( filePath ) ) {
        QCoreApplication::installTranslator( translator );
        mTranslators[ locale ] << translator;
        //qWarning() << "added" << filePath << locale.name();
        return true;
    }

    delete translator;
    return false;
}

void pTranslationManager::clearTranslators()
{
    foreach ( const QLocale& locale, mTranslators.keys() ) {
        foreach ( QTranslator* translator, mTranslators[ locale ] ) {
            QCoreApplication::removeTranslator( translator );
        }

        qDeleteAll( mTranslators[ locale ] );
    }

    mAvailableLocales.clear();
    mTranslators.clear();
}

QStringList pTranslationManager::availableLocales() const
{
    return mAvailableLocales.toList();
}

QList<QLocale> pTranslationManager::availableQLocales() const
{
    QList<QLocale> locales;

    foreach ( const QString& locale, availableLocales() ) {
        locales << QLocale( locale );
    }

    return locales;
}

void pTranslationManager::reloadTranslations()
{
    clearTranslators();

    const QStringList paths = QStringList() << mTranslationsPaths << mSystemTranslationsPaths;
    const QString appDirPath = qApp->applicationDirPath();
    QList<QFileInfo> files;
    QSet<QString> translations;

    foreach ( QString path, paths ) {
        if ( QDir::isRelativePath( path ) ) {
            path = QString( QSL( "%1/%2" ) ).arg( appDirPath ).arg( path );
        }

        files << QDir( path ).entryInfoList( mTranslationsMasks.toList() );
    }

    foreach ( const QFileInfo& file, files ) {
        const QString cfp = file.canonicalFilePath();
        QString fileName = file.fileName();

        if ( translations.contains( cfp )
            || QDir::match( mForbiddenTranslationsMasks.toList(), fileName ) ) {
            continue;
        }

        fileName
            .remove( QSL( ".qm" ), Qt::CaseInsensitive )
            .replace( QSL( "." ), QSL( "_" ) )
            .replace( QSL( "-" ), QSL( "_" ) )
            ;

        const int count = fileName.count( QSL( "_" ) );
        bool added = false;
        bool foundValidLocale = false;
        QLocale locale;

        for ( int i = 1; i < count +1; i++ ) {
            QString part = fileName.section( QL1C( '_' ), i );

            if ( part.toLower() == QSL( "iw" ) || part.toLower().endsWith( QSL( "_iw" ) ) ) {
                part.replace( QSL( "iw" ), QSL( "he" ) );
            }

            locale = QLocale( part );

            //qWarning() << fileName << part << locale.name() << mCurrentLocale.name();

            if ( locale != QLocale::c() ) {
                foundValidLocale = true;
                mAvailableLocales << locale.name();

                if ( locale == mCurrentLocale
                    || locale.language() == mCurrentLocale.language() ) {
                    if ( addTranslator( cfp, locale ) ) {
                        translations << cfp;
                        added = true;
                    }
                }

                break;
            }
        }

        if ( foundValidLocale || added /*|| mCurrentLocale != QLocale::c()*/ ) {
            continue;
        }

        mAvailableLocales << QLocale::c().name();

        if ( mCurrentLocale.language() == QLocale::C
            || mCurrentLocale.language() == QLocale::English ) {
            if ( addTranslator( cfp, QLocale::c() ) ) {
                translations << cfp;
            }
        }
    }

    if ( !mAvailableLocales.contains( QLocale::c().name() ) && mFakeCLocaleEnabled ) {
        mAvailableLocales << QLocale::c().name();
    }
}

QStringList pTranslationManager::translationsMasks() const
{
    return mTranslationsMasks.toList();
}

void pTranslationManager::setTranslationsMasks( const QStringList& masks )
{
    mTranslationsMasks = masks.toSet();
}

void pTranslationManager::addTranslationsMask( const QString& mask )
{
    mTranslationsMasks << mask;
}

void pTranslationManager::removeTranslationsMask( const QString& mask )
{
    mTranslationsMasks.remove( mask );
}

QStringList pTranslationManager::forbiddenTranslationsMasks() const
{
    return mForbiddenTranslationsMasks.toList();
}

void pTranslationManager::setForbiddenTranslationsMasks( const QStringList& masks )
{
    mForbiddenTranslationsMasks = masks.toSet();
}

void pTranslationManager::addForbiddenTranslationsMask( const QString& mask )
{
    mForbiddenTranslationsMasks << mask;
}

void pTranslationManager::removeForbiddenTranslationsMask( const QString& mask )
{
    mForbiddenTranslationsMasks.remove( mask );
}

QLocale pTranslationManager::currentLocale() const
{
    return mCurrentLocale;
}

QLocale pTranslationManager::systemLocale() const
{
    return QLocale::system();
}

void pTranslationManager::setCurrentLocale( const QLocale& locale )
{
    mCurrentLocale = locale;
    QLocale::setDefault( locale );
}

QStringList pTranslationManager::translationsPaths() const
{
    return mTranslationsPaths;
}

void pTranslationManager::setTranslationsPaths( const QStringList& paths )
{
    mTranslationsPaths = paths;
}

QStringList pTranslationManager::systemTranslationsPaths() const
{
    return mSystemTranslationsPaths;
}

void pTranslationManager::setSystemTranslationsPaths( const QStringList& paths )
{
    mSystemTranslationsPaths = paths;
}

void pTranslationManager::setFakeCLocaleEnabled( bool enabled )
{
    mFakeCLocaleEnabled = enabled;
}

bool pTranslationManager::isFakeCLocaleEnabled() const
{
    return mFakeCLocaleEnabled;
}
