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

// Own
#include "OffscreenRenderer.h"

// std
#include <cmath>

OffscreenRenderer::OffscreenRenderer(QObject* parent)
    : QObject(parent)
{
    connect(KWin::effects, &KWin::EffectsHandler::windowGeometryShapeChanged,
            this, &OffscreenRenderer::slotWindowGeometryShapeChanged);
    connect(KWin::effects, &KWin::EffectsHandler::windowDeleted,
            this, &OffscreenRenderer::slotWindowDeleted);
    connect(KWin::effects, &KWin::EffectsHandler::windowDamaged,
            this, &OffscreenRenderer::slotWindowDamaged);
}

OffscreenRenderer::~OffscreenRenderer()
{
    unregisterAllWindows();
}

void OffscreenRenderer::registerWindow(KWin::EffectWindow* w)
{
    if (m_renderResources.contains(w)) {
        return;
    }

    RenderResources resources = allocateRenderResources(w);
    if (resources.isValid()) {
        m_renderResources[w] = resources;
    }
}

void OffscreenRenderer::unregisterWindow(KWin::EffectWindow* w)
{
    auto it = m_renderResources.find(w);
    if (it == m_renderResources.end()) {
        return;
    }

    freeRenderResources(*it);
    m_renderResources.erase(it);
}

void OffscreenRenderer::unregisterAllWindows()
{
    const auto windows = m_renderResources.keys();
    for (KWin::EffectWindow* window : windows) {
        unregisterWindow(window);
    }
}

KWin::GLTexture* OffscreenRenderer::render(KWin::EffectWindow* w)
{
    auto it = m_renderResources.find(w);
    if (it == m_renderResources.end())
        return nullptr;

    if (!it->isDirty)
        return it->texture;

    KWin::GLRenderTarget::pushRenderTarget(it->renderTarget);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    const int mask = KWin::Effect::PAINT_WINDOW_TRANSFORMED | KWin::Effect::PAINT_WINDOW_TRANSLUCENT;

    KWin::WindowPaintData data(w);

    QMatrix4x4 projectionMatrix;
    projectionMatrix.ortho(QRect(0, 0, it->texture->width(), it->texture->height()));
    data.setProjectionMatrix(projectionMatrix);

    data.setXTranslation(-w->expandedGeometry().x());
    data.setYTranslation(-w->expandedGeometry().y());

    KWin::effects->drawWindow(w, mask, KWin::infiniteRegion(), data);

    KWin::GLRenderTarget::popRenderTarget();

    it->isDirty = false;

    return it->texture;
}

void OffscreenRenderer::slotWindowGeometryShapeChanged(KWin::EffectWindow* w, const QRect& old)
{
    Q_UNUSED(old)

    auto it = m_renderResources.find(w);
    if (it == m_renderResources.end()) {
        return;
    }

    KWin::effects->makeOpenGLContextCurrent();
    freeRenderResources(*it);
    RenderResources resources = allocateRenderResources(w);
    if (resources.isValid()) {
        *it = resources;
    } else {
        m_renderResources.erase(it);
    }
    KWin::effects->doneOpenGLContextCurrent();
}

void OffscreenRenderer::slotWindowDeleted(KWin::EffectWindow* w)
{
    KWin::effects->makeOpenGLContextCurrent();
    unregisterWindow(w);
    KWin::effects->doneOpenGLContextCurrent();
}

void OffscreenRenderer::slotWindowDamaged(KWin::EffectWindow* w)
{
    auto it = m_renderResources.find(w);
    if (it != m_renderResources.end())
        it->isDirty = true;
}

OffscreenRenderer::RenderResources
OffscreenRenderer::allocateRenderResources(KWin::EffectWindow* w)
{
    const QRect geometry = w->expandedGeometry();

    const int levels = std::floor(std::log2(std::min(geometry.width(), geometry.height()))) + 1;
    QScopedPointer<KWin::GLTexture> texture;
    texture.reset(new KWin::GLTexture(GL_RGBA8, geometry.width(), geometry.height(), levels));
    texture->setFilter(GL_LINEAR_MIPMAP_LINEAR);
    texture->setWrapMode(GL_CLAMP_TO_EDGE);

    QScopedPointer<KWin::GLRenderTarget> renderTarget;
    renderTarget.reset(new KWin::GLRenderTarget(*texture));

    if (!renderTarget->valid()) {
        return {};
    }

    RenderResources resources = {};
    resources.texture = texture.take();
    resources.renderTarget = renderTarget.take();
    resources.isDirty = true;

    return resources;
}

void OffscreenRenderer::freeRenderResources(RenderResources& resources)
{
    delete resources.renderTarget;
    delete resources.texture;
}
