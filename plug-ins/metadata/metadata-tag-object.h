/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2024 Niels De Graef
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __METADATA_TAG_OBJECT_H__
#define __METADATA_TAG_OBJECT_H__

#include <glib-object.h>

#define GIMP_TYPE_METADATA_TAG_OBJECT (gimp_metadata_tag_object_get_type ())
G_DECLARE_FINAL_TYPE (GimpMetadataTagObject, gimp_metadata_tag_object,
                      GIMP, METADATA_TAG_OBJECT,
                      GObject);

GimpMetadataTagObject *  gimp_metadata_tag_object_new         (const gchar           *tag,
                                                               const gchar           *value);

const gchar *            gimp_metadata_tag_object_get_tag     (GimpMetadataTagObject *self);
const gchar *            gimp_metadata_tag_object_get_value   (GimpMetadataTagObject *self);

#endif /* __METADATA_TAG_OBJECT_H__ */
