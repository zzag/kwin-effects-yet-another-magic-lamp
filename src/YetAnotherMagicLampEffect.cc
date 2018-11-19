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

// Auto-generated
#include "YetAnotherMagicLampConfig.h"

// std
#include <cmath>

static inline std::chrono::milliseconds durationFraction(std::chrono::milliseconds duration, qreal fraction)
{
    return std::chrono::milliseconds(qMax(qRound(duration.count() * fraction), 1));
}

YetAnotherMagicLampEffect::YetAnotherMagicLampEffect()
{
    reconfigure(ReconfigureAll);

    m_shapeCurve.setType(QEasingCurve::InOutSine);

    connect(effects, &EffectsHandler::windowMinimized,
        this, &YetAnotherMagicLampEffect::slotWindowMinimized);
    connect(effects, &EffectsHandler::windowUnminimized,
        this, &YetAnotherMagicLampEffect::slotWindowUnminimized);
    connect(effects, &EffectsHandler::windowDeleted,
        this, &YetAnotherMagicLampEffect::slotWindowDeleted);
    connect(effects, &EffectsHandler::activeFullScreenEffectChanged,
        this, &YetAnotherMagicLampEffect::slotActiveFullScreenEffectChanged);
}

YetAnotherMagicLampEffect::~YetAnotherMagicLampEffect()
{
}

void YetAnotherMagicLampEffect::reconfigure(ReconfigureFlags flags)
{
    Q_UNUSED(flags)

    YetAnotherMagicLampConfig::self()->read();

    const int baseDuration = animationTime<YetAnotherMagicLampConfig>(250);
    m_squashDuration = std::chrono::milliseconds(baseDuration);
    m_stretchDuration = std::chrono::milliseconds(qMax(qRound(baseDuration * 0.4), 1));
    m_bumpDuration = std::chrono::milliseconds(qMax(qRound(baseDuration * 0.9), 1));

    m_gridResolution = YetAnotherMagicLampConfig::gridResolution();
    m_maxBumpDistance = YetAnotherMagicLampConfig::maxBumpDistance();
    m_stretchFactor = YetAnotherMagicLampConfig::initialShapeFactor();
}

void YetAnotherMagicLampEffect::prePaintScreen(ScreenPrePaintData& data, int time)
{
    const std::chrono::milliseconds delta(time);

    auto animationIt = m_animations.begin();
    while (animationIt != m_animations.end()) {
        (*animationIt).timeLine.update(delta);
        ++animationIt;
    }

    data.mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS;

    effects->prePaintScreen(data, time);
}

void YetAnotherMagicLampEffect::prePaintWindow(EffectWindow* w, WindowPrePaintData& data, int time)
{
    auto animationIt = m_animations.constFind(w);
    if (animationIt != m_animations.constEnd()) {
        data.setTransformed();
        w->enablePainting(EffectWindow::PAINT_DISABLED_BY_MINIMIZE);

        // Windows are transformed in Stretch1, Stretch2, and Squash stages.
        if ((*animationIt).stage != AnimationStage::Bump) {
            data.quads = data.quads.makeGrid(m_gridResolution);
        }
    }

    effects->prePaintWindow(w, data, time);
}

void YetAnotherMagicLampEffect::paintBumpStage(const Animation& animation, WindowPaintData& data) const
{
    const qreal t = animation.timeLine.value();

    switch (animation.direction) {
    case Direction::Top:
        data.translate(0.0, interpolate(0.0, animation.bumpDistance, t));
        break;

    case Direction::Right:
        data.translate(-interpolate(0.0, animation.bumpDistance, t), 0.0);
        break;

    case Direction::Bottom:
        data.translate(0.0, -interpolate(0.0, animation.bumpDistance, t));
        break;

    case Direction::Left:
        data.translate(interpolate(0.0, animation.bumpDistance, t), 0.0);
        break;

    default:
        Q_UNREACHABLE();
    }
}

void YetAnotherMagicLampEffect::transformTop(WindowQuadList& quads, const TransformParams& params) const
{
    const qreal distance = params.windowRect.bottom() - params.iconRect.bottom() + params.bumpDistance;

    for (int i = 0; i < quads.count(); ++i) {
        WindowQuad& quad = quads[i];

        const qreal topOffset = quad[0].y() - interpolate(0.0, distance, params.squashProgress);
        const qreal bottomOffset = quad[2].y() - interpolate(0.0, distance, params.squashProgress);

        const qreal topScale = params.stretchProgress * m_shapeCurve.valueForProgress((params.windowRect.height() - topOffset) / distance);
        const qreal bottomScale = params.stretchProgress * m_shapeCurve.valueForProgress((params.windowRect.height() - bottomOffset) / distance);

        const qreal targetTopLeftX = params.iconRect.x() + params.iconRect.width() * quad[0].x() / params.windowRect.width();
        const qreal targetTopRightX = params.iconRect.x() + params.iconRect.width() * quad[1].x() / params.windowRect.width();
        const qreal targetBottomRightX = params.iconRect.x() + params.iconRect.width() * quad[2].x() / params.windowRect.width();
        const qreal targetBottomLeftX = params.iconRect.x() + params.iconRect.width() * quad[3].x() / params.windowRect.width();

        quad[0].setX(quad[0].x() + topScale * (targetTopLeftX - (params.windowRect.x() + quad[0].x())));
        quad[1].setX(quad[1].x() + topScale * (targetTopRightX - (params.windowRect.x() + quad[1].x())));
        quad[2].setX(quad[2].x() + bottomScale * (targetBottomRightX - (params.windowRect.x() + quad[2].x())));
        quad[3].setX(quad[3].x() + bottomScale * (targetBottomLeftX - (params.windowRect.x() + quad[3].x())));

        const qreal targetTopOffset = topOffset + params.bumpDistance * params.bumpProgress;
        const qreal targetBottomOffset = bottomOffset + params.bumpDistance * params.bumpProgress;

        quad[0].setY(targetTopOffset);
        quad[1].setY(targetTopOffset);
        quad[2].setY(targetBottomOffset);
        quad[3].setY(targetBottomOffset);
    }
}

void YetAnotherMagicLampEffect::transformBottom(WindowQuadList& quads, const TransformParams& params) const
{
    const qreal distance = params.iconRect.top() - params.windowRect.top() + params.bumpDistance;

    for (int i = 0; i < quads.count(); ++i) {
        WindowQuad& quad = quads[i];

        const qreal topOffset = quad[0].y() + interpolate(0.0, distance, params.squashProgress);
        const qreal bottomOffset = quad[2].y() + interpolate(0.0, distance, params.squashProgress);

        const qreal topScale = params.stretchProgress * m_shapeCurve.valueForProgress(topOffset / distance);
        const qreal bottomScale = params.stretchProgress * m_shapeCurve.valueForProgress(bottomOffset / distance);

        const qreal targetTopLeftX = params.iconRect.x() + params.iconRect.width() * quad[0].x() / params.windowRect.width();
        const qreal targetTopRightX = params.iconRect.x() + params.iconRect.width() * quad[1].x() / params.windowRect.width();
        const qreal targetBottomRightX = params.iconRect.x() + params.iconRect.width() * quad[2].x() / params.windowRect.width();
        const qreal targetBottomLeftX = params.iconRect.x() + params.iconRect.width() * quad[3].x() / params.windowRect.width();

        quad[0].setX(quad[0].x() + topScale * (targetTopLeftX - (params.windowRect.x() + quad[0].x())));
        quad[1].setX(quad[1].x() + topScale * (targetTopRightX - (params.windowRect.x() + quad[1].x())));
        quad[2].setX(quad[2].x() + bottomScale * (targetBottomRightX - (params.windowRect.x() + quad[2].x())));
        quad[3].setX(quad[3].x() + bottomScale * (targetBottomLeftX - (params.windowRect.x() + quad[3].x())));

        const qreal targetTopOffset = topOffset - params.bumpDistance * params.bumpProgress;
        const qreal targetBottomOffset = bottomOffset - params.bumpDistance * params.bumpProgress;

        quad[0].setY(targetTopOffset);
        quad[1].setY(targetTopOffset);
        quad[2].setY(targetBottomOffset);
        quad[3].setY(targetBottomOffset);
    }
}

void YetAnotherMagicLampEffect::transformLeft(WindowQuadList& quads, const TransformParams& params) const
{
    const qreal distance = params.windowRect.right() - params.iconRect.right() + params.bumpDistance;

    for (int i = 0; i < quads.count(); ++i) {
        WindowQuad& quad = quads[i];

        const qreal leftOffset = quad[0].x() - interpolate(0.0, distance, params.squashProgress);
        const qreal rightOffset = quad[2].x() - interpolate(0.0, distance, params.squashProgress);

        const qreal leftScale = params.stretchProgress * m_shapeCurve.valueForProgress((params.windowRect.width() - leftOffset) / distance);
        const qreal rightScale = params.stretchProgress * m_shapeCurve.valueForProgress((params.windowRect.width() - rightOffset) / distance);

        const qreal targetTopLeftY = params.iconRect.y() + params.iconRect.height() * quad[0].y() / params.windowRect.height();
        const qreal targetTopRightY = params.iconRect.y() + params.iconRect.height() * quad[1].y() / params.windowRect.height();
        const qreal targetBottomRightY = params.iconRect.y() + params.iconRect.height() * quad[2].y() / params.windowRect.height();
        const qreal targetBottomLeftY = params.iconRect.y() + params.iconRect.height() * quad[3].y() / params.windowRect.height();

        quad[0].setY(quad[0].y() + leftScale * (targetTopLeftY - (params.windowRect.y() + quad[0].y())));
        quad[3].setY(quad[3].y() + leftScale * (targetBottomLeftY - (params.windowRect.y() + quad[3].y())));
        quad[1].setY(quad[1].y() + rightScale * (targetTopRightY - (params.windowRect.y() + quad[1].y())));
        quad[2].setY(quad[2].y() + rightScale * (targetBottomRightY - (params.windowRect.y() + quad[2].y())));

        const qreal targetLeftOffset = leftOffset + params.bumpDistance * params.bumpProgress;
        const qreal targetRightOffset = rightOffset + params.bumpDistance * params.bumpProgress;

        quad[0].setX(targetLeftOffset);
        quad[3].setX(targetLeftOffset);
        quad[1].setX(targetRightOffset);
        quad[2].setX(targetRightOffset);
    }
}

void YetAnotherMagicLampEffect::transformRight(WindowQuadList& quads, const TransformParams& params) const
{
    const qreal distance = params.iconRect.left() - params.windowRect.left() + params.bumpDistance;

    for (int i = 0; i < quads.count(); ++i) {
        WindowQuad& quad = quads[i];

        const qreal leftOffset = quad[0].x() + interpolate(0.0, distance, params.squashProgress);
        const qreal rightOffset = quad[2].x() + interpolate(0.0, distance, params.squashProgress);

        const qreal leftScale = params.stretchProgress * m_shapeCurve.valueForProgress(leftOffset / distance);
        const qreal rightScale = params.stretchProgress * m_shapeCurve.valueForProgress(rightOffset / distance);

        const qreal targetTopLeftY = params.iconRect.y() + params.iconRect.height() * quad[0].y() / params.windowRect.height();
        const qreal targetTopRightY = params.iconRect.y() + params.iconRect.height() * quad[1].y() / params.windowRect.height();
        const qreal targetBottomRightY = params.iconRect.y() + params.iconRect.height() * quad[2].y() / params.windowRect.height();
        const qreal targetBottomLeftY = params.iconRect.y() + params.iconRect.height() * quad[3].y() / params.windowRect.height();

        quad[0].setY(quad[0].y() + leftScale * (targetTopLeftY - (params.windowRect.y() + quad[0].y())));
        quad[3].setY(quad[3].y() + leftScale * (targetBottomLeftY - (params.windowRect.y() + quad[3].y())));
        quad[1].setY(quad[1].y() + rightScale * (targetTopRightY - (params.windowRect.y() + quad[1].y())));
        quad[2].setY(quad[2].y() + rightScale * (targetBottomRightY - (params.windowRect.y() + quad[2].y())));

        const qreal targetLeftOffset = leftOffset - params.bumpDistance * params.bumpProgress;
        const qreal targetRightOffset = rightOffset - params.bumpDistance * params.bumpProgress;

        quad[0].setX(targetLeftOffset);
        quad[3].setX(targetLeftOffset);
        quad[1].setX(targetRightOffset);
        quad[2].setX(targetRightOffset);
    }
}

void YetAnotherMagicLampEffect::transformGeneric(WindowQuadList& quads, Direction direction, const TransformParams& params) const
{
    switch (direction) {
    case Direction::Top:
        transformTop(quads, params);
        break;

    case Direction::Right:
        transformRight(quads, params);
        break;

    case Direction::Bottom:
        transformBottom(quads, params);
        break;

    case Direction::Left:
        transformLeft(quads, params);
        break;

    default:
        Q_UNREACHABLE();
    }
}

void YetAnotherMagicLampEffect::paintSquashStage(const EffectWindow* w, const Animation& animation, WindowPaintData& data) const
{
    TransformParams params;
    params.squashProgress = animation.timeLine.value();
    params.stretchProgress = qMin(animation.stretchFactor + params.squashProgress, 1.0);
    params.bumpProgress = 1.0;
    params.bumpDistance = animation.bumpDistance;
    params.windowRect = w->geometry();
    params.iconRect = w->iconGeometry();

    transformGeneric(data.quads, animation.direction, params);
}

void YetAnotherMagicLampEffect::paintStretch1Stage(const EffectWindow* w, const Animation& animation, WindowPaintData& data) const
{
    TransformParams params;
    params.squashProgress = 0.0;
    params.stretchProgress = animation.stretchFactor * animation.timeLine.value();
    params.bumpProgress = 1.0;
    params.bumpDistance = animation.bumpDistance;
    params.windowRect = w->geometry();
    params.iconRect = w->iconGeometry();

    transformGeneric(data.quads, animation.direction, params);
}

void YetAnotherMagicLampEffect::paintStretch2Stage(const EffectWindow* w, const Animation& animation, WindowPaintData& data) const
{
    TransformParams params;
    params.squashProgress = 0.0;
    params.stretchProgress = animation.stretchFactor * animation.timeLine.value();
    params.bumpProgress = params.stretchProgress;
    params.bumpDistance = animation.bumpDistance;
    params.windowRect = w->geometry();
    params.iconRect = w->iconGeometry();

    transformGeneric(data.quads, animation.direction, params);
}

void YetAnotherMagicLampEffect::paintWindow(EffectWindow* w, int mask, QRegion region, WindowPaintData& data)
{
    auto animationIt = m_animations.constFind(w);
    if (animationIt == m_animations.constEnd()) {
        effects->paintWindow(w, mask, region, data);
        return;
    }

    switch ((*animationIt).stage) {
    case AnimationStage::Bump:
        paintBumpStage(*animationIt, data);
        break;

    case AnimationStage::Stretch1:
        paintStretch1Stage(w, *animationIt, data);
        break;

    case AnimationStage::Stretch2:
        paintStretch2Stage(w, *animationIt, data);
        break;

    case AnimationStage::Squash:
        paintSquashStage(w, *animationIt, data);
        break;

    default:
        Q_UNREACHABLE();
    }

    if ((*animationIt).clip) {
        const QRect iconRect = w->iconGeometry();
        QRect clipRect = w->expandedGeometry();
        switch ((*animationIt).direction) {
        case Direction::Top:
            clipRect.translate(0, (*animationIt).bumpDistance);
            clipRect.setTop(iconRect.top());
            clipRect.setLeft(qMin(iconRect.left(), clipRect.left()));
            clipRect.setRight(qMax(iconRect.right(), clipRect.right()));
            break;

        case Direction::Right:
            clipRect.translate(-(*animationIt).bumpDistance, 0);
            clipRect.setRight(iconRect.right());
            clipRect.setTop(qMin(iconRect.top(), clipRect.top()));
            clipRect.setBottom(qMax(iconRect.bottom(), clipRect.bottom()));
            break;

        case Direction::Bottom:
            clipRect.translate(0, -(*animationIt).bumpDistance);
            clipRect.setBottom(iconRect.bottom());
            clipRect.setLeft(qMin(iconRect.left(), clipRect.left()));
            clipRect.setRight(qMax(iconRect.right(), clipRect.right()));
            break;

        case Direction::Left:
            clipRect.translate((*animationIt).bumpDistance, 0);
            clipRect.setLeft(iconRect.left());
            clipRect.setTop(qMin(iconRect.top(), clipRect.top()));
            clipRect.setBottom(qMax(iconRect.bottom(), clipRect.bottom()));
            break;

        default:
            Q_UNREACHABLE();
        }

        region = QRegion(clipRect);
    }

    effects->paintWindow(w, mask, region, data);
}

bool YetAnotherMagicLampEffect::updateInAnimationStage(Animation& animation)
{
    switch (animation.stage) {
    case AnimationStage::Bump:
        animation.stage = AnimationStage::Stretch1;
        animation.timeLine.reset();
        animation.timeLine.setDirection(TimeLine::Forward);
        animation.timeLine.setDuration(
            durationFraction(m_stretchDuration, animation.stretchFactor));
        animation.timeLine.setEasingCurve(QEasingCurve::Linear);
        animation.clip = true;
        return false;

    case AnimationStage::Stretch1:
    case AnimationStage::Stretch2:
        animation.stage = AnimationStage::Squash;
        animation.timeLine.reset();
        animation.timeLine.setDirection(TimeLine::Forward);
        animation.timeLine.setDuration(m_squashDuration);
        animation.timeLine.setEasingCurve(QEasingCurve::Linear);
        animation.clip = true;
        return false;

    case AnimationStage::Squash:
        return true;

    default:
        Q_UNREACHABLE();
    }
}

bool YetAnotherMagicLampEffect::updateOutAnimationStage(Animation& animation)
{
    switch (animation.stage) {
    case AnimationStage::Bump:
        return true;

    case AnimationStage::Stretch1:
        if (animation.bumpDistance == 0) {
            return true;
        }
        animation.stage = AnimationStage::Bump;
        animation.timeLine.reset();
        animation.timeLine.setDirection(TimeLine::Backward);
        animation.timeLine.setDuration(m_bumpDuration);
        animation.timeLine.setEasingCurve(QEasingCurve::Linear);
        animation.clip = false;
        return false;

    case AnimationStage::Stretch2:
        return true;

    case AnimationStage::Squash:
        animation.stage = AnimationStage::Stretch2;
        animation.timeLine.reset();
        animation.timeLine.setDirection(TimeLine::Backward);
        animation.timeLine.setDuration(
            durationFraction(m_stretchDuration, animation.stretchFactor));
        animation.timeLine.setEasingCurve(QEasingCurve::Linear);
        animation.clip = false;
        return false;

    default:
        Q_UNREACHABLE();
    }
}

void YetAnotherMagicLampEffect::postPaintScreen()
{
    auto animationIt = m_animations.begin();
    while (animationIt != m_animations.end()) {
        if (!(*animationIt).timeLine.done()) {
            ++animationIt;
            continue;
        }

        bool done;
        switch ((*animationIt).kind) {
        case AnimationKind::In:
            done = updateInAnimationStage(*animationIt);
            break;

        case AnimationKind::Out:
            done = updateOutAnimationStage(*animationIt);
            break;

        default:
            Q_UNREACHABLE();
        }

        if (done) {
            animationIt = m_animations.erase(animationIt);
        } else {
            ++animationIt;
        }
    }

    // TODO: Don't do full repaints.
    effects->addRepaintFull();

    effects->postPaintScreen();
}

bool YetAnotherMagicLampEffect::isActive() const
{
    return !m_animations.isEmpty();
}

bool YetAnotherMagicLampEffect::supported()
{
    if (!effects->animationsSupported()) {
        return false;
    }
    if (effects->isOpenGLCompositing()) {
        return true;
    }
    return false;
}

void YetAnotherMagicLampEffect::slotWindowMinimized(EffectWindow* w)
{
    if (effects->activeFullScreenEffect()) {
        return;
    }

    const QRect iconRect = w->iconGeometry();
    if (!iconRect.isValid()) {
        return;
    }

    Animation& animation = m_animations[w];
    animation.kind = AnimationKind::In;

    if (animation.timeLine.running()) {
        animation.timeLine.toggleDirection();
        // Don't need to schedule repaint because it's already scheduled.
        return;
    }

    animation.direction = findDirectionToIcon(w);
    animation.bumpDistance = computeBumpDistance(w, animation.direction);
    animation.stretchFactor = computeStretchFactor(w, animation.direction, animation.bumpDistance);

    if (animation.bumpDistance != 0) {
        animation.stage = AnimationStage::Bump;
        animation.timeLine.reset();
        animation.timeLine.setDirection(TimeLine::Forward);
        animation.timeLine.setDuration(m_bumpDuration);
        animation.timeLine.setEasingCurve(QEasingCurve::Linear);
        animation.clip = false;
    } else {
        animation.stage = AnimationStage::Stretch1;
        animation.timeLine.reset();
        animation.timeLine.setDirection(TimeLine::Forward);
        animation.timeLine.setDuration(
            durationFraction(m_stretchDuration, animation.stretchFactor));
        animation.timeLine.setEasingCurve(QEasingCurve::Linear);
        animation.clip = true;
    }

    effects->addRepaintFull();
}

void YetAnotherMagicLampEffect::slotWindowUnminimized(EffectWindow* w)
{
    if (effects->activeFullScreenEffect()) {
        return;
    }

    const QRect iconRect = w->iconGeometry();
    if (!iconRect.isValid()) {
        return;
    }

    Animation& animation = m_animations[w];
    animation.kind = AnimationKind::Out;

    if (animation.timeLine.running()) {
        animation.timeLine.toggleDirection();
        // Don't need to schedule repaint because it's already scheduled.
        return;
    }

    animation.direction = findDirectionToIcon(w);
    animation.bumpDistance = computeBumpDistance(w, animation.direction);
    animation.stretchFactor = computeStretchFactor(w, animation.direction, animation.bumpDistance);

    animation.stage = AnimationStage::Squash;
    animation.timeLine.reset();
    animation.timeLine.setDirection(TimeLine::Backward);
    animation.timeLine.setDuration(m_squashDuration);
    animation.timeLine.setEasingCurve(QEasingCurve::Linear);
    animation.clip = true;

    effects->addRepaintFull();
}

void YetAnotherMagicLampEffect::slotWindowDeleted(EffectWindow* w)
{
    m_animations.remove(w);
}

void YetAnotherMagicLampEffect::slotActiveFullScreenEffectChanged()
{
    if (effects->activeFullScreenEffect() == nullptr) {
        return;
    }

    m_animations.clear();
}

YetAnotherMagicLampEffect::Direction
YetAnotherMagicLampEffect::findDirectionToIcon(const EffectWindow* w) const
{
    const QRect iconRect = w->iconGeometry();

    const EffectWindowList windows = effects->stackingOrder();
    auto panelIt = std::find_if(windows.constBegin(), windows.constEnd(),
        [&iconRect](const EffectWindow* w) {
            if (!w->isDock()) {
                return false;
            }
            return w->geometry().intersects(iconRect);
        });
    const EffectWindow* panel = (panelIt != windows.constEnd())
        ? (*panelIt)
        : nullptr;

    Direction direction;
    if (panel != nullptr) {
        const QRect panelScreen = effects->clientArea(ScreenArea, (*panelIt));
        if (panel->width() >= panel->height()) {
            direction = (panel->y() == panelScreen.y())
                ? Direction::Top
                : Direction::Bottom;
        } else {
            direction = (panel->x() == panelScreen.x())
                ? Direction::Left
                : Direction::Right;
        }
    } else {
        const QRect iconScreen = effects->clientArea(ScreenArea, iconRect.center(), effects->currentDesktop());
        const QRect rect = iconScreen.intersected(iconRect);

        // TODO: Explain why this is in some sense wrong.
        if (rect.left() == iconScreen.left()) {
            direction = Direction::Left;
        } else if (rect.top() == iconScreen.top()) {
            direction = Direction::Top;
        } else if (rect.right() == iconScreen.right()) {
            direction = Direction::Right;
        } else {
            direction = Direction::Bottom;
        }
    }

    if (panel != nullptr && panel->screen() == w->screen()) {
        return direction;
    }

    const QRect windowRect = w->geometry();

    if (direction == Direction::Top && windowRect.top() < iconRect.top()) {
        direction = Direction::Bottom;
    } else if (direction == Direction::Right && iconRect.right() < windowRect.right()) {
        direction = Direction::Left;
    } else if (direction == Direction::Bottom && iconRect.bottom() < windowRect.bottom()) {
        direction = Direction::Top;
    } else if (direction == Direction::Left && windowRect.left() < iconRect.left()) {
        direction = Direction::Right;
    }

    return direction;
}

int YetAnotherMagicLampEffect::computeBumpDistance(const EffectWindow* w, Direction direction) const
{
    const QRect windowRect = w->geometry();
    const QRect iconRect = w->iconGeometry();

    int bumpDistance = 0;
    switch (direction) {
    case Direction::Top:
        bumpDistance = qMax(0, iconRect.y() + iconRect.height() - windowRect.y());
        break;

    case Direction::Right:
        bumpDistance = qMax(0, windowRect.x() + windowRect.width() - iconRect.x());
        break;

    case Direction::Bottom:
        bumpDistance = qMax(0, windowRect.y() + windowRect.height() - iconRect.y());
        break;

    case Direction::Left:
        bumpDistance = qMax(0, iconRect.x() + iconRect.width() - windowRect.x());
        break;

    default:
        Q_UNREACHABLE();
    }

    bumpDistance += qMin(bumpDistance, m_maxBumpDistance);

    return bumpDistance;
}

qreal YetAnotherMagicLampEffect::computeStretchFactor(const EffectWindow* w, Direction direction, int bumpDistance) const
{
    const QRect windowRect = w->geometry();
    const QRect iconRect = w->iconGeometry();

    int movingExtent = 0;
    int distanceToIcon = 0;
    switch (direction) {
    case Direction::Top:
        movingExtent = windowRect.height();
        distanceToIcon = windowRect.bottom() - iconRect.bottom() + bumpDistance;
        break;

    case Direction::Right:
        movingExtent = windowRect.width();
        distanceToIcon = iconRect.left() - windowRect.left() + bumpDistance;
        break;

    case Direction::Bottom:
        movingExtent = windowRect.height();
        distanceToIcon = iconRect.top() - windowRect.top() + bumpDistance;
        break;

    case Direction::Left:
        movingExtent = windowRect.width();
        distanceToIcon = windowRect.right() - iconRect.right() + bumpDistance;
        break;

    default:
        Q_UNREACHABLE();
    }

    const qreal minimumStretchFactor = static_cast<qreal>(movingExtent) / distanceToIcon;
    return qMax(m_stretchFactor, minimumStretchFactor);
}
