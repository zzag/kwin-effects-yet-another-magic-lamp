/*
 * Copyright (C) 2018 Vlad Zagorodniy <vladzzag@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

// kwineffects
#include <kwineffects.h>
#include <kwingltexture.h>
#include <kwinglutils.h>

// Qt
#include <QMap>
#include <QObject>

/**
 * Helper class to render windows into offscreen textures.
 **/
class OffscreenRenderer : public QObject {
    Q_OBJECT

public:
    explicit OffscreenRenderer(QObject* parent = nullptr);
    ~OffscreenRenderer() override;

    /**
     * Allocates necessary rendering resources.
     **/
    void registerWindow(KWin::EffectWindow* w);

    /**
     * Frees rendering resources.
     **/
    void unregisterWindow(KWin::EffectWindow* w);

    /**
     * Frees rendering resources for all windows.
     **/
    void unregisterAllWindows();

    /**
     * Renders the given window into an offscreen texture.
     **/
    KWin::GLTexture* render(KWin::EffectWindow* w);

private Q_SLOTS:
    void slotWindowGeometryShapeChanged(KWin::EffectWindow* w, const QRect& old);
    void slotWindowDeleted(KWin::EffectWindow* w);
    void slotWindowDamaged(KWin::EffectWindow* w);

private:
    struct RenderResources {
        bool isValid() const
        {
            return texture && renderTarget;
        }

        KWin::GLTexture* texture = nullptr;
        KWin::GLRenderTarget* renderTarget = nullptr;
        bool isDirty = false;
    };

    RenderResources allocateRenderResources(KWin::EffectWindow* w);
    void freeRenderResources(RenderResources& resources);

    QMap<KWin::EffectWindow*, RenderResources> m_renderResources;

    Q_DISABLE_COPY(OffscreenRenderer)
};
