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
#include "Model.h"
#include "WindowQuad.h"

static inline std::chrono::milliseconds durationFraction(std::chrono::milliseconds duration, qreal fraction)
{
    return std::chrono::milliseconds(qMax(qRound(duration.count() * fraction), 1));
}

Model::Model(KWin::EffectWindow* window)
    : m_window(window)
{
}

static KWin::EffectWindow* findDock(const KWin::EffectWindow* client)
{
    const KWin::EffectWindowList windows = KWin::effects->stackingOrder();

    for (KWin::EffectWindow* window : windows) {
        if (!window->isDock())
            continue;

        if (!client->geometry().intersects(window->iconGeometry()))
            continue;

        return window;
    }

    return nullptr;
}

static Direction realizeDirection(const KWin::EffectWindow* window)
{
    const KWin::EffectWindow* dock = findDock(window);

    Direction direction;
    QPoint screenDelta;

    if (dock) {
        const QRect screenRect = KWin::effects->clientArea(KWin::ScreenArea, dock);

        if (dock->width() >= dock->height()) {
            if (dock->y() == screenRect.y())
                direction = Direction::Top;
            else
                direction = Direction::Bottom;
        } else {
            if (dock->x() == screenRect.x())
                direction = Direction::Left;
            else
                direction = Direction::Right;
        }

        screenDelta += screenRect.center();
    } else {
        // Perhaps the dock is hidden, deduce direction to the icon.
        const QRect iconRect = window->iconGeometry();

        const int screen = KWin::effects->screenNumber(iconRect.center());
        const int desktop = KWin::effects->currentDesktop();

        const QRect screenRect = KWin::effects->clientArea(KWin::ScreenArea, screen, desktop);
        const QRect constrainedRect = screenRect.intersected(iconRect);

        if (constrainedRect.left() == screenRect.left())
            direction = Direction::Left;
        else if (constrainedRect.top() == screenRect.top())
            direction = Direction::Top;
        else if (constrainedRect.right() == screenRect.right())
            direction = Direction::Right;
        else
            direction = Direction::Bottom;

        screenDelta += screenRect.center();
    }

    const QRect screenRect = KWin::effects->clientArea(KWin::ScreenArea, window);
    screenDelta -= screenRect.center();

    // Dock and window are on the same screen, no further adjustments are required.
    if (screenDelta.isNull())
        return direction;

    const int safetyMargin = 100;

    switch (direction) {
    case Direction::Top:
    case Direction::Bottom:
        // Approach the icon along horizontal direction.
        if (qAbs(screenDelta.x()) - qAbs(screenDelta.y()) > safetyMargin)
            return direction;

        // Approach the icon from bottom.
        if (screenDelta.y() < 0)
            return Direction::Top;

        // Approach the icon from top.
        return Direction::Bottom;

    case Direction::Left:
    case Direction::Right:
        // Approach the icon along vertical direction.
        if (qAbs(screenDelta.y()) - qAbs(screenDelta.x()) > safetyMargin)
            return direction;

        // Approach the icon from right side.
        if (screenDelta.x() < 0)
            return Direction::Left;

        // Approach the icon from left side.
        return Direction::Right;

    default:
        Q_UNREACHABLE();
    }
}

void Model::start(AnimationKind kind)
{
    m_kind = kind;

    if (m_timeLine.running()) {
        m_timeLine.toggleDirection();
        return;
    }

    m_direction = realizeDirection(m_window);
    m_bumpDistance = computeBumpDistance();
    m_shapeFactor = computeShapeFactor();

    switch (m_kind) {
    case AnimationKind::Minimize:
        if (m_bumpDistance != 0) {
            m_stage = AnimationStage::Bump;
            m_timeLine.reset();
            m_timeLine.setDirection(KWin::TimeLine::Forward);
            m_timeLine.setDuration(m_parameters.bumpDuration);
            m_timeLine.setEasingCurve(QEasingCurve::Linear);
            m_clip = false;
        } else {
            m_stage = AnimationStage::Stretch1;
            m_timeLine.reset();
            m_timeLine.setDirection(KWin::TimeLine::Forward);
            m_timeLine.setDuration(
                durationFraction(m_parameters.stretchDuration, m_shapeFactor));
            m_timeLine.setEasingCurve(QEasingCurve::Linear);
            m_clip = true;
        }
        break;

    case AnimationKind::Unminimize:
        m_stage = AnimationStage::Squash;
        m_timeLine.reset();
        m_timeLine.setDirection(KWin::TimeLine::Backward);
        m_timeLine.setDuration(m_parameters.squashDuration);
        m_timeLine.setEasingCurve(QEasingCurve::Linear);
        m_clip = true;
        break;

    default:
        Q_UNREACHABLE();
    }
}

void Model::step(std::chrono::milliseconds delta)
{
    m_timeLine.update(delta);
    if (!m_timeLine.done()) {
        return;
    }

    switch (m_kind) {
    case AnimationKind::Minimize:
        updateMinimizeStage();
        break;

    case AnimationKind::Unminimize:
        updateUnminimizeStage();
        break;

    default:
        Q_UNREACHABLE();
    }
}

void Model::updateMinimizeStage()
{
    switch (m_stage) {
    case AnimationStage::Bump:
        m_stage = AnimationStage::Stretch1;
        m_timeLine.reset();
        m_timeLine.setDirection(KWin::TimeLine::Forward);
        m_timeLine.setDuration(
            durationFraction(m_parameters.stretchDuration, m_shapeFactor));
        m_timeLine.setEasingCurve(QEasingCurve::Linear);
        m_clip = true;
        m_done = false;
        return;

    case AnimationStage::Stretch1:
    case AnimationStage::Stretch2:
        m_stage = AnimationStage::Squash;
        m_timeLine.reset();
        m_timeLine.setDirection(KWin::TimeLine::Forward);
        m_timeLine.setDuration(m_parameters.squashDuration);
        m_timeLine.setEasingCurve(QEasingCurve::Linear);
        m_clip = true;
        m_done = false;
        return;

    case AnimationStage::Squash:
        m_done = true;
        return;

    default:
        Q_UNREACHABLE();
    }
}

void Model::updateUnminimizeStage()
{
    switch (m_stage) {
    case AnimationStage::Bump:
        m_done = true;
        return;

    case AnimationStage::Stretch1:
        if (m_bumpDistance == 0) {
            m_done = true;
            return;
        }
        m_stage = AnimationStage::Bump;
        m_timeLine.reset();
        m_timeLine.setDirection(KWin::TimeLine::Backward);
        m_timeLine.setDuration(m_parameters.bumpDuration);
        m_timeLine.setEasingCurve(QEasingCurve::Linear);
        m_clip = false;
        m_done = false;
        return;

    case AnimationStage::Stretch2:
        m_done = true;
        return;

    case AnimationStage::Squash:
        m_stage = AnimationStage::Stretch2;
        m_timeLine.reset();
        m_timeLine.setDirection(KWin::TimeLine::Backward);
        m_timeLine.setDuration(
            durationFraction(m_parameters.stretchDuration, m_shapeFactor));
        m_timeLine.setEasingCurve(QEasingCurve::Linear);
        m_clip = false;
        m_done = false;
        return;

    default:
        Q_UNREACHABLE();
    }
}

bool Model::done() const
{
    return m_done;
}

struct TransformParameters {
    QEasingCurve shapeCurve;
    Direction direction;
    qreal stretchProgress;
    qreal squashProgress;
    qreal bumpProgress;
    qreal bumpDistance;
    QRect windowRect;
    QRect iconRect;
};

static inline qreal interpolate(qreal from, qreal to, qreal t)
{
    return from * (1.0 - t) + to * t;
}

template <typename Collection>
static void transformQuadsLeft(const TransformParameters& params, Collection& quads)
{
    // FIXME: Have a generic function that transforms window quads.

    const qreal distance = params.windowRect.right() - params.iconRect.right() + params.bumpDistance;

    for (int i = 0; i < quads.count(); ++i) {
        WindowQuad& quad = quads[i];

        const qreal leftOffset = quad[0].x() - interpolate(0.0, distance, params.squashProgress);
        const qreal rightOffset = quad[2].x() - interpolate(0.0, distance, params.squashProgress);

        const qreal leftScale = params.stretchProgress * params.shapeCurve.valueForProgress((params.windowRect.width() - leftOffset) / distance);
        const qreal rightScale = params.stretchProgress * params.shapeCurve.valueForProgress((params.windowRect.width() - rightOffset) / distance);

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

template <typename Collection>
static void transformQuadsTop(const TransformParameters& params, Collection& quads)
{
    // FIXME: Have a generic function that transforms window quads.

    const qreal distance = params.windowRect.bottom() - params.iconRect.bottom() + params.bumpDistance;

    for (int i = 0; i < quads.count(); ++i) {
        WindowQuad& quad = quads[i];

        const qreal topOffset = quad[0].y() - interpolate(0.0, distance, params.squashProgress);
        const qreal bottomOffset = quad[2].y() - interpolate(0.0, distance, params.squashProgress);

        const qreal topScale = params.stretchProgress * params.shapeCurve.valueForProgress((params.windowRect.height() - topOffset) / distance);
        const qreal bottomScale = params.stretchProgress * params.shapeCurve.valueForProgress((params.windowRect.height() - bottomOffset) / distance);

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

template <typename Collection>
static void transformQuadsRight(const TransformParameters& params, Collection& quads)
{
    // FIXME: Have a generic function that transforms window quads.

    const qreal distance = params.iconRect.left() - params.windowRect.left() + params.bumpDistance;

    for (int i = 0; i < quads.count(); ++i) {
        WindowQuad& quad = quads[i];

        const qreal leftOffset = quad[0].x() + interpolate(0.0, distance, params.squashProgress);
        const qreal rightOffset = quad[2].x() + interpolate(0.0, distance, params.squashProgress);

        const qreal leftScale = params.stretchProgress * params.shapeCurve.valueForProgress(leftOffset / distance);
        const qreal rightScale = params.stretchProgress * params.shapeCurve.valueForProgress(rightOffset / distance);

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

template <typename Collection>
static void transformQuadsBottom(const TransformParameters& params, Collection& quads)
{
    // FIXME: Have a generic function that transforms window quads.

    const qreal distance = params.iconRect.top() - params.windowRect.top() + params.bumpDistance;

    for (int i = 0; i < quads.count(); ++i) {
        WindowQuad& quad = quads[i];

        const qreal topOffset = quad[0].y() + interpolate(0.0, distance, params.squashProgress);
        const qreal bottomOffset = quad[2].y() + interpolate(0.0, distance, params.squashProgress);

        const qreal topScale = params.stretchProgress * params.shapeCurve.valueForProgress(topOffset / distance);
        const qreal bottomScale = params.stretchProgress * params.shapeCurve.valueForProgress(bottomOffset / distance);

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

template <typename Collection>
static void transformQuads(const TransformParameters& params, Collection& quads)
{
    switch (params.direction) {
    case Direction::Left:
        transformQuadsLeft(params, quads);
        break;

    case Direction::Top:
        transformQuadsTop(params, quads);
        break;

    case Direction::Right:
        transformQuadsRight(params, quads);
        break;

    case Direction::Bottom:
        transformQuadsBottom(params, quads);
        break;

    default:
        Q_UNREACHABLE();
    }
}

void Model::apply(QVector<WindowQuad>& quads) const
{
    switch (m_stage) {
    case AnimationStage::Bump:
        applyBump(quads);
        break;

    case AnimationStage::Stretch1:
        applyStretch1(quads);
        break;

    case AnimationStage::Stretch2:
        applyStretch2(quads);
        break;

    case AnimationStage::Squash:
        applySquash(quads);
        break;
    }
}

void Model::applyBump(QVector<WindowQuad>& quads) const
{
    TransformParameters params;
    params.shapeCurve = m_parameters.shapeCurve;
    params.direction = m_direction;
    params.squashProgress = 0.0;
    params.stretchProgress = 0.0;
    params.bumpProgress = m_timeLine.value();
    params.bumpDistance = m_bumpDistance;
    params.windowRect = m_window->geometry();
    params.iconRect = m_window->iconGeometry();
    transformQuads(params, quads);
}

void Model::applyStretch1(QVector<WindowQuad>& quads) const
{
    TransformParameters params;
    params.shapeCurve = m_parameters.shapeCurve;
    params.direction = m_direction;
    params.squashProgress = 0.0;
    params.stretchProgress = m_shapeFactor * m_timeLine.value();
    params.bumpProgress = 1.0;
    params.bumpDistance = m_bumpDistance;
    params.windowRect = m_window->geometry();
    params.iconRect = m_window->iconGeometry();
    transformQuads(params, quads);
}

void Model::applyStretch2(QVector<WindowQuad>& quads) const
{
    TransformParameters params;
    params.shapeCurve = m_parameters.shapeCurve;
    params.direction = m_direction;
    params.squashProgress = 0.0;
    params.stretchProgress = m_shapeFactor * m_timeLine.value();
    params.bumpProgress = params.stretchProgress;
    params.bumpDistance = m_bumpDistance;
    params.windowRect = m_window->geometry();
    params.iconRect = m_window->iconGeometry();
    transformQuads(params, quads);
}

void Model::applySquash(QVector<WindowQuad>& quads) const
{
    TransformParameters params;
    params.shapeCurve = m_parameters.shapeCurve;
    params.direction = m_direction;
    params.squashProgress = m_timeLine.value();
    params.stretchProgress = qMin(m_shapeFactor + params.squashProgress, 1.0);
    params.bumpProgress = 1.0;
    params.bumpDistance = m_bumpDistance;
    params.windowRect = m_window->geometry();
    params.iconRect = m_window->iconGeometry();
    transformQuads(params, quads);
}

Model::Parameters Model::parameters() const
{
    return m_parameters;
}

void Model::setParameters(const Parameters& parameters)
{
    m_parameters = parameters;
}

KWin::EffectWindow* Model::window() const
{
    return m_window;
}

void Model::setWindow(KWin::EffectWindow* window)
{
    m_window = window;
}

bool Model::needsClip() const
{
    return m_clip;
}

QRegion Model::clipRegion() const
{
    const QRect iconRect = m_window->iconGeometry();
    QRect clipRect = m_window->expandedGeometry();

    switch (m_direction) {
    case Direction::Top:
        clipRect.translate(0, m_bumpDistance);
        clipRect.setTop(iconRect.top());
        clipRect.setLeft(qMin(iconRect.left(), clipRect.left()));
        clipRect.setRight(qMax(iconRect.right(), clipRect.right()));
        break;

    case Direction::Right:
        clipRect.translate(-m_bumpDistance, 0);
        clipRect.setRight(iconRect.right());
        clipRect.setTop(qMin(iconRect.top(), clipRect.top()));
        clipRect.setBottom(qMax(iconRect.bottom(), clipRect.bottom()));
        break;

    case Direction::Bottom:
        clipRect.translate(0, -m_bumpDistance);
        clipRect.setBottom(iconRect.bottom());
        clipRect.setLeft(qMin(iconRect.left(), clipRect.left()));
        clipRect.setRight(qMax(iconRect.right(), clipRect.right()));
        break;

    case Direction::Left:
        clipRect.translate(m_bumpDistance, 0);
        clipRect.setLeft(iconRect.left());
        clipRect.setTop(qMin(iconRect.top(), clipRect.top()));
        clipRect.setBottom(qMax(iconRect.bottom(), clipRect.bottom()));
        break;

    default:
        Q_UNREACHABLE();
    }

    return clipRect;
}

int Model::computeBumpDistance() const
{
    const QRect windowRect = m_window->geometry();
    const QRect iconRect = m_window->iconGeometry();

    int bumpDistance = 0;
    switch (m_direction) {
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

    bumpDistance += qMin(bumpDistance, m_parameters.bumpDistance);

    return bumpDistance;
}

qreal Model::computeShapeFactor() const
{
    const QRect windowRect = m_window->geometry();
    const QRect iconRect = m_window->iconGeometry();

    int movingExtent = 0;
    int distanceToIcon = 0;
    switch (m_direction) {
    case Direction::Top:
        movingExtent = windowRect.height();
        distanceToIcon = windowRect.bottom() - iconRect.bottom() + m_bumpDistance;
        break;

    case Direction::Right:
        movingExtent = windowRect.width();
        distanceToIcon = iconRect.left() - windowRect.left() + m_bumpDistance;
        break;

    case Direction::Bottom:
        movingExtent = windowRect.height();
        distanceToIcon = iconRect.top() - windowRect.top() + m_bumpDistance;
        break;

    case Direction::Left:
        movingExtent = windowRect.width();
        distanceToIcon = windowRect.right() - iconRect.right() + m_bumpDistance;
        break;

    default:
        Q_UNREACHABLE();
    }

    const qreal minimumShapeFactor = static_cast<qreal>(movingExtent) / distanceToIcon;
    return qMax(m_parameters.shapeFactor, minimumShapeFactor);
}
