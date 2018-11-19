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

using namespace KWin;

class YetAnotherMagicLampEffect : public Effect {
    Q_OBJECT

public:
    YetAnotherMagicLampEffect();
    ~YetAnotherMagicLampEffect() override;

    void reconfigure(ReconfigureFlags flags) override;

    void prePaintScreen(ScreenPrePaintData& data, int time) override;
    void prePaintWindow(EffectWindow* w, WindowPrePaintData& data, int time) override;
    void paintWindow(EffectWindow* w, int mask, QRegion region, WindowPaintData& data) override;
    void postPaintScreen() override;

    bool isActive() const override;
    int requestedEffectChainPosition() const override;

    static bool supported();

private Q_SLOTS:
    void slotWindowMinimized(EffectWindow* w);
    void slotWindowUnminimized(EffectWindow* w);
    void slotWindowDeleted(EffectWindow* w);
    void slotActiveFullScreenEffectChanged();

private:
    struct Animation;
    void paintBumpStage(const Animation& animation, WindowPaintData& data) const;
    void paintSquashStage(const EffectWindow* w, const Animation& animation, WindowPaintData& data) const;
    void paintStretch1Stage(const EffectWindow* w, const Animation& animation, WindowPaintData& data) const;
    void paintStretch2Stage(const EffectWindow* w, const Animation& animation, WindowPaintData& data) const;

    struct TransformParams {
        qreal stretchProgress;
        qreal squashProgress;
        qreal bumpProgress;
        qreal bumpDistance;
        QRect windowRect;
        QRect iconRect;
    };

    void transformTop(WindowQuadList& quads, const TransformParams& params) const;
    void transformBottom(WindowQuadList& quads, const TransformParams& params) const;
    void transformLeft(WindowQuadList& quads, const TransformParams& params) const;
    void transformRight(WindowQuadList& quads, const TransformParams& params) const;

    enum class Direction;
    void transformGeneric(WindowQuadList& quads, Direction direction, const TransformParams& params) const;

    bool updateInAnimationStage(Animation& animation);
    bool updateOutAnimationStage(Animation& animation);

    Direction findDirectionToIcon(const EffectWindow* w) const;
    int computeBumpDistance(const EffectWindow* w, Direction direction) const;
    qreal computeStretchFactor(const EffectWindow* w, Direction direction, int bumpDistance) const;

private:
    std::chrono::milliseconds m_squashDuration;
    std::chrono::milliseconds m_stretchDuration;
    std::chrono::milliseconds m_bumpDuration;
    int m_gridResolution;
    int m_maxBumpDistance;
    qreal m_stretchFactor; // TODO: Rename to m_shapeFactor.
    QEasingCurve m_shapeCurve;

    enum class AnimationKind {
        In,
        Out
    };

    enum class AnimationStage {
        Bump,
        Stretch1,
        Stretch2,
        Squash
    };

    enum class Direction {
        Top,
        Right,
        Bottom,
        Left
    };

    struct Animation {
        AnimationKind kind;
        AnimationStage stage;
        TimeLine timeLine;
        Direction direction;
        int bumpDistance;
        qreal stretchFactor;
        bool clip;
    };

    QHash<const EffectWindow*, Animation> m_animations;
};

inline int YetAnotherMagicLampEffect::requestedEffectChainPosition() const
{
    return 50;
}
