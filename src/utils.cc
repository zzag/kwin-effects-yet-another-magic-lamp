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

// own
#include "utils.h"

namespace utils {

QImage textureToImage(KWin::GLTexture* texture)
{
    QImage image(texture->size(), QImage::Format_ARGB32_Premultiplied);

    texture->bind();
    glReadPixels(0, 0, texture->width(), texture->height(), GL_BGRA,
        GL_UNSIGNED_INT_8_8_8_8_REV, image.bits());
    texture->unbind();

    return image.mirrored(false, true);
}

} // namespace utils
