/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpimagecolorprofile.h
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#pragma once

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


GimpColorProfile * gimp_image_get_color_profile           (GimpImage                 *image);
gboolean           gimp_image_set_color_profile           (GimpImage                 *image,
                                                           GimpColorProfile          *profile);

GimpColorProfile * gimp_image_get_simulation_profile      (GimpImage                 *image);
gboolean           gimp_image_set_simulation_profile      (GimpImage                 *image,
                                                           GimpColorProfile          *profile);

GimpColorProfile * gimp_image_get_effective_color_profile (GimpImage                 *image);

gboolean           gimp_image_convert_color_profile       (GimpImage                 *image,
                                                           GimpColorProfile          *profile,
                                                           GimpColorRenderingIntent   intent,
                                                           gboolean                   bpc);

G_END_DECLS
