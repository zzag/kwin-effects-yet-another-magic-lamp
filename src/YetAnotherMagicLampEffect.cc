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
#include "YetAnotherMagicLampEffect.h"
#include "OffscreenRenderer.h"
#include "WindowMeshRenderer.h"

// Auto-generated
#include "YetAnotherMagicLampConfig.h"

// std
#include <cmath>

enum ShapeCurve {
    Linear = 0,
    Quad = 1,
    Cubic = 2,
    Quart = 3,
    Quint = 4,
    Sine = 5,
    Circ = 6,
    Bounce = 7,
    Bezier = 8
};

YetAnotherMagicLampEffect::YetAnotherMagicLampEffect()
{
    reconfigure(ReconfigureAll);

    connect(KWin::effects, &KWin::EffectsHandler::windowMinimized,
        this, &YetAnotherMagicLampEffect::slotWindowMinimized);
    connect(KWin::effects, &KWin::EffectsHandler::windowUnminimized,
        this, &YetAnotherMagicLampEffect::slotWindowUnminimized);
    connect(KWin::effects, &KWin::EffectsHandler::windowDeleted,
        this, &YetAnotherMagicLampEffect::slotWindowDeleted);
    connect(KWin::effects, &KWin::EffectsHandler::activeFullScreenEffectChanged,
        this, &YetAnotherMagicLampEffect::slotActiveFullScreenEffectChanged);

    m_offscreenRenderer = new OffscreenRenderer(this);
    m_meshRenderer = new WindowMeshRenderer(this);
}

YetAnotherMagicLampEffect::~YetAnotherMagicLampEffect()
{
}

void YetAnotherMagicLampEffect::reconfigure(ReconfigureFlags flags)
{
    Q_UNUSED(flags)

    YetAnotherMagicLampConfig::self()->read();

    QEasingCurve curve;
    const auto shapeCurve = static_cast<ShapeCurve>(YetAnotherMagicLampConfig::shapeCurve());
    switch (shapeCurve) {
    case ShapeCurve::Linear:
        curve.setType(QEasingCurve::Linear);
        break;

    case ShapeCurve::Quad:
        curve.setType(QEasingCurve::InOutQuad);
        break;

    case ShapeCurve::Cubic:
        curve.setType(QEasingCurve::InOutCubic);
        break;

    case ShapeCurve::Quart:
        curve.setType(QEasingCurve::InOutQuart);
        break;

    case ShapeCurve::Quint:
        curve.setType(QEasingCurve::InOutQuint);
        break;

    case ShapeCurve::Sine:
        curve.setType(QEasingCurve::InOutSine);
        break;

    case ShapeCurve::Circ:
        curve.setType(QEasingCurve::InOutCirc);
        break;

    case ShapeCurve::Bounce:
        curve.setType(QEasingCurve::InOutBounce);
        break;

    case ShapeCurve::Bezier: {
        curve.setType(QEasingCurve::BezierSpline);
        const QPointF c1(0.5, 0.0);
        const QPointF c2(0.5, 1.0);
        const QPointF end(1.0, 1.0);
        curve.addCubicBezierSegment(c1, c2, end);
        break;
    }

    default:
        // Fallback to the sine curve.
        curve.setType(QEasingCurve::InOutSine);
        break;
    }
    m_modelParameters.shapeCurve = curve;

    const int baseDuration = animationTime<YetAnotherMagicLampConfig>(250);
    m_modelParameters.squashDuration = std::chrono::milliseconds(baseDuration);
    m_modelParameters.stretchDuration = std::chrono::milliseconds(qMax(qRound(baseDuration * 0.4), 1));
    m_modelParameters.bumpDuration = std::chrono::milliseconds(qMax(qRound(baseDuration * 0.9), 1));
    m_modelParameters.shapeFactor = YetAnotherMagicLampConfig::initialShapeFactor();
    m_modelParameters.bumpDistance = YetAnotherMagicLampConfig::maxBumpDistance();

    m_gridResolution = YetAnotherMagicLampConfig::gridResolution();
}

void YetAnotherMagicLampEffect::prePaintScreen(KWin::ScreenPrePaintData& data, int time)
{
    const std::chrono::milliseconds delta(time);

    auto modelIt = m_models.begin();
    while (modelIt != m_models.end()) {
        (*modelIt).step(delta);
        ++modelIt;
    }

    data.mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS;

    KWin::effects->prePaintScreen(data, time);
}

void YetAnotherMagicLampEffect::prePaintWindow(KWin::EffectWindow* w, KWin::WindowPrePaintData& data, int time)
{
    auto modelIt = m_models.constFind(w);
    if (modelIt != m_models.constEnd()) {
        w->enablePainting(KWin::EffectWindow::PAINT_DISABLED_BY_MINIMIZE);
    }

    KWin::effects->prePaintWindow(w, data, time);
}

void YetAnotherMagicLampEffect::paintWindow(KWin::EffectWindow* w, int mask, QRegion region, KWin::WindowPaintData& data)
{
    auto modelIt = m_models.constFind(w);
    if (modelIt == m_models.constEnd()) {
        KWin::effects->paintWindow(w, mask, region, data);
        return;
    }

    QVector<WindowQuad> quads = m_meshRenderer->makeGrid(w, m_gridResolution);
    (*modelIt).apply(quads);

    if ((*modelIt).needsClip()) {
        region = (*modelIt).clipRegion();
    }

    KWin::GLTexture* texture = m_offscreenRenderer->render(w);
    m_meshRenderer->render(w, quads, texture, region);
}

void YetAnotherMagicLampEffect::postPaintScreen()
{
    auto modelIt = m_models.begin();
    while (modelIt != m_models.end()) {
        if ((*modelIt).done()) {
            m_offscreenRenderer->unregisterWindow(modelIt.key());
            modelIt = m_models.erase(modelIt);
        } else {
            ++modelIt;
        }
    }

    // TODO: Don't do full repaints.
    KWin::effects->addRepaintFull();

    KWin::effects->postPaintScreen();
}

bool YetAnotherMagicLampEffect::isActive() const
{
    return !m_models.isEmpty();
}

bool YetAnotherMagicLampEffect::supported()
{
    if (!KWin::effects->animationsSupported()) {
        return false;
    }
    if (KWin::effects->isOpenGLCompositing()) {
        return true;
    }
    return false;
}

void YetAnotherMagicLampEffect::slotWindowMinimized(KWin::EffectWindow* w)
{
    if (KWin::effects->activeFullScreenEffect()) {
        return;
    }

    const QRect iconRect = w->iconGeometry();
    if (!iconRect.isValid()) {
        return;
    }

    Model& model = m_models[w];
    model.setWindow(w);
    model.setParameters(m_modelParameters);
    model.start(Model::AnimationKind::Minimize);

    m_offscreenRenderer->registerWindow(w);

    KWin::effects->addRepaintFull();
}

void YetAnotherMagicLampEffect::slotWindowUnminimized(KWin::EffectWindow* w)
{
    if (KWin::effects->activeFullScreenEffect()) {
        return;
    }

    const QRect iconRect = w->iconGeometry();
    if (!iconRect.isValid()) {
        return;
    }

    Model& model = m_models[w];
    model.setWindow(w);
    model.setParameters(m_modelParameters);
    model.start(Model::AnimationKind::Unminimize);

    m_offscreenRenderer->registerWindow(w);

    KWin::effects->addRepaintFull();
}

void YetAnotherMagicLampEffect::slotWindowDeleted(KWin::EffectWindow* w)
{
    m_models.remove(w);
}

void YetAnotherMagicLampEffect::slotActiveFullScreenEffectChanged()
{
    if (KWin::effects->activeFullScreenEffect() == nullptr) {
        return;
    }

    m_models.clear();
}
