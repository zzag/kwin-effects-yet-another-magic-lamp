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

class YetAnotherMagicLampEffect : public KWin::Effect {
    Q_OBJECT

public:
    YetAnotherMagicLampEffect();
    ~YetAnotherMagicLampEffect() override;

    void reconfigure(ReconfigureFlags flags) override;

    void prePaintScreen(KWin::ScreenPrePaintData& data, int time) override;
    void prePaintWindow(KWin::EffectWindow* w, KWin::WindowPrePaintData& data, int time) override;
    void paintWindow(KWin::EffectWindow* w, int mask, QRegion region, KWin::WindowPaintData& data) override;
    void postPaintScreen() override;

    bool isActive() const override;
    int requestedEffectChainPosition() const override;

    static bool supported();

private Q_SLOTS:
    void slotWindowMinimized(KWin::EffectWindow* w);
    void slotWindowUnminimized(KWin::EffectWindow* w);
    void slotWindowDeleted(KWin::EffectWindow* w);
    void slotActiveFullScreenEffectChanged();

private:
    struct Animation;
    void paintBumpStage(const Animation& animation, KWin::WindowPaintData& data) const;
    void paintSquashStage(const KWin::EffectWindow* w, const Animation& animation, KWin::WindowPaintData& data) const;
    void paintStretch1Stage(const KWin::EffectWindow* w, const Animation& animation, KWin::WindowPaintData& data) const;
    void paintStretch2Stage(const KWin::EffectWindow* w, const Animation& animation, KWin::WindowPaintData& data) const;

    struct TransformParams {
        qreal stretchProgress;
        qreal squashProgress;
        qreal bumpProgress;
        qreal bumpDistance;
        QRect windowRect;
        QRect iconRect;
    };

    void transformTop(KWin::WindowQuadList& quads, const TransformParams& params) const;
    void transformBottom(KWin::WindowQuadList& quads, const TransformParams& params) const;
    void transformLeft(KWin::WindowQuadList& quads, const TransformParams& params) const;
    void transformRight(KWin::WindowQuadList& quads, const TransformParams& params) const;

    enum class Direction;
    void transformGeneric(KWin::WindowQuadList& quads, Direction direction, const TransformParams& params) const;

    bool updateInAnimationStage(Animation& animation);
    bool updateOutAnimationStage(Animation& animation);

    Direction findDirectionToIcon(const KWin::EffectWindow* w) const;
    int computeBumpDistance(const KWin::EffectWindow* w, Direction direction) const;
    qreal computeStretchFactor(const KWin::EffectWindow* w, Direction direction, int bumpDistance) const;

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
        KWin::TimeLine timeLine;
        Direction direction;
        int bumpDistance;
        qreal stretchFactor;
        bool clip;
    };

    QHash<const KWin::EffectWindow*, Animation> m_animations;
};

inline int YetAnotherMagicLampEffect::requestedEffectChainPosition() const
{
    return 50;
}
