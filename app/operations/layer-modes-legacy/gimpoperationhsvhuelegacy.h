/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationhsvhuelegacy.h
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
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

#pragma once

#include "operations/layer-modes/gimpoperationlayermode.h"


#define GIMP_TYPE_OPERATION_HSV_HUE_LEGACY            (gimp_operation_hsv_hue_legacy_get_type ())
#define GIMP_OPERATION_HSV_HUE_LEGACY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_HSV_HUE_LEGACY, GimpOperationHsvHueLegacy))
#define GIMP_OPERATION_HSV_HUE_LEGACY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_HSV_HUE_LEGACY, GimpOperationHsvHueLegacyClass))
#define GIMP_IS_OPERATION_HSV_HUE_LEGACY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_HSV_HUE_LEGACY))
#define GIMP_IS_OPERATION_HSV_HUE_LEGACY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_HSV_HUE_LEGACY))
#define GIMP_OPERATION_HSV_HUE_LEGACY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_HSV_HUE_LEGACY, GimpOperationHsvHueLegacyClass))


typedef struct _GimpOperationHsvHueLegacy      GimpOperationHsvHueLegacy;
typedef struct _GimpOperationHsvHueLegacyClass GimpOperationHsvHueLegacyClass;

struct _GimpOperationHsvHueLegacy
{
  GimpOperationLayerMode  parent_instance;
};

struct _GimpOperationHsvHueLegacyClass
{
  GimpOperationLayerModeClass  parent_class;
};


GType   gimp_operation_hsv_hue_legacy_get_type (void) G_GNUC_CONST;
