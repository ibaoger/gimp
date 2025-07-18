/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimptextlayer_pdb.h
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

/* NOTE: This file is auto-generated by pdbgen.pl */

#pragma once

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


GimpTextLayer*        gimp_text_layer_new                (GimpImage              *image,
                                                          const gchar            *text,
                                                          GimpFont               *font,
                                                          gdouble                 size,
                                                          GimpUnit               *unit);
gchar*                gimp_text_layer_get_text           (GimpTextLayer          *layer);
gboolean              gimp_text_layer_set_text           (GimpTextLayer          *layer,
                                                          const gchar            *text);
gchar*                gimp_text_layer_get_markup         (GimpTextLayer          *layer);
gboolean              gimp_text_layer_set_markup         (GimpTextLayer          *layer,
                                                          const gchar            *markup);
GimpFont*             gimp_text_layer_get_font           (GimpTextLayer          *layer);
gboolean              gimp_text_layer_set_font           (GimpTextLayer          *layer,
                                                          GimpFont               *font);
gdouble               gimp_text_layer_get_font_size      (GimpTextLayer          *layer,
                                                          GimpUnit              **unit);
gboolean              gimp_text_layer_set_font_size      (GimpTextLayer          *layer,
                                                          gdouble                 font_size,
                                                          GimpUnit               *unit);
gboolean              gimp_text_layer_get_antialias      (GimpTextLayer          *layer);
gboolean              gimp_text_layer_set_antialias      (GimpTextLayer          *layer,
                                                          gboolean                antialias);
GimpTextHintStyle     gimp_text_layer_get_hint_style     (GimpTextLayer          *layer);
gboolean              gimp_text_layer_set_hint_style     (GimpTextLayer          *layer,
                                                          GimpTextHintStyle       style);
gboolean              gimp_text_layer_get_kerning        (GimpTextLayer          *layer);
gboolean              gimp_text_layer_set_kerning        (GimpTextLayer          *layer,
                                                          gboolean                kerning);
gchar*                gimp_text_layer_get_language       (GimpTextLayer          *layer);
gboolean              gimp_text_layer_set_language       (GimpTextLayer          *layer,
                                                          const gchar            *language);
GimpTextDirection     gimp_text_layer_get_base_direction (GimpTextLayer          *layer);
gboolean              gimp_text_layer_set_base_direction (GimpTextLayer          *layer,
                                                          GimpTextDirection       direction);
GimpTextJustification gimp_text_layer_get_justification  (GimpTextLayer          *layer);
gboolean              gimp_text_layer_set_justification  (GimpTextLayer          *layer,
                                                          GimpTextJustification   justify);
GeglColor*            gimp_text_layer_get_color          (GimpTextLayer          *layer);
gboolean              gimp_text_layer_set_color          (GimpTextLayer          *layer,
                                                          GeglColor              *color);
gdouble               gimp_text_layer_get_indent         (GimpTextLayer          *layer);
gboolean              gimp_text_layer_set_indent         (GimpTextLayer          *layer,
                                                          gdouble                 indent);
gdouble               gimp_text_layer_get_line_spacing   (GimpTextLayer          *layer);
gboolean              gimp_text_layer_set_line_spacing   (GimpTextLayer          *layer,
                                                          gdouble                 line_spacing);
gdouble               gimp_text_layer_get_letter_spacing (GimpTextLayer          *layer);
gboolean              gimp_text_layer_set_letter_spacing (GimpTextLayer          *layer,
                                                          gdouble                 letter_spacing);
gboolean              gimp_text_layer_resize             (GimpTextLayer          *layer,
                                                          gdouble                 width,
                                                          gdouble                 height);

G_END_DECLS
