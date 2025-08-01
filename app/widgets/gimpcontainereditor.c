/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainereditor.c
 * Copyright (C) 2001-2011 Michael Natterer <mitch@gimp.org>
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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpasyncset.h"
#include "core/gimpcontext.h"
#include "core/gimplist.h"
#include "core/gimpviewable.h"

#include "gimpcontainereditor.h"
#include "gimpcontainericonview.h"
#include "gimpcontainertreeview.h"
#include "gimpcontainerview.h"
#include "gimpdocked.h"
#include "gimpmenufactory.h"
#include "gimpviewrenderer.h"
#include "gimpuimanager.h"


enum
{
  PROP_0,
  PROP_VIEW_TYPE,
  PROP_CONTAINER,
  PROP_CONTEXT,
  PROP_VIEW_SIZE,
  PROP_VIEW_BORDER_WIDTH,
  PROP_MENU_FACTORY,
  PROP_MENU_IDENTIFIER,
  PROP_UI_PATH
};


struct _GimpContainerEditorPrivate
{
  GimpViewType     view_type;
  GimpContainer   *container;
  GimpContext     *context;
  gint             view_size;
  gint             view_border_width;
  GimpMenuFactory *menu_factory;
  gchar           *menu_identifier;
  gchar           *ui_path;
  GtkWidget       *busy_box;
  GBinding        *async_set_binding;
};


static void  gimp_container_editor_docked_iface_init (GimpDockedInterface *iface);

static void   gimp_container_editor_constructed      (GObject             *object);
static void   gimp_container_editor_dispose          (GObject             *object);
static void   gimp_container_editor_set_property     (GObject             *object,
                                                      guint                property_id,
                                                      const GValue        *value,
                                                      GParamSpec          *pspec);
static void   gimp_container_editor_get_property     (GObject             *object,
                                                      guint                property_id,
                                                      GValue              *value,
                                                      GParamSpec          *pspec);

static void   gimp_container_editor_selection_changed(GimpContainerView   *view,
                                                      GimpContainerEditor *editor);
static void   gimp_container_editor_item_activated   (GtkWidget           *widget,
                                                      GimpViewable        *viewable,
                                                      GimpContainerEditor *editor);

static GtkWidget * gimp_container_editor_get_preview (GimpDocked       *docked,
                                                      GimpContext      *context,
                                                      GtkIconSize       size);
static void        gimp_container_editor_set_context (GimpDocked       *docked,
                                                      GimpContext      *context);
static GimpUIManager * gimp_container_editor_get_menu(GimpDocked       *docked,
                                                      const gchar     **ui_path,
                                                      gpointer         *popup_data);

static gboolean  gimp_container_editor_has_button_bar      (GimpDocked *docked);
static void      gimp_container_editor_set_show_button_bar (GimpDocked *docked,
                                                            gboolean    show);
static gboolean  gimp_container_editor_get_show_button_bar (GimpDocked *docked);


G_DEFINE_TYPE_WITH_CODE (GimpContainerEditor, gimp_container_editor,
                         GTK_TYPE_BOX,
                         G_ADD_PRIVATE (GimpContainerEditor)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_container_editor_docked_iface_init))

#define parent_class gimp_container_editor_parent_class


static void
gimp_container_editor_class_init (GimpContainerEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed   = gimp_container_editor_constructed;
  object_class->dispose       = gimp_container_editor_dispose;
  object_class->set_property  = gimp_container_editor_set_property;
  object_class->get_property  = gimp_container_editor_get_property;

  klass->select_item     = NULL;
  klass->activate_item   = NULL;

  g_object_class_install_property (object_class, PROP_VIEW_TYPE,
                                   g_param_spec_enum ("view-type",
                                                      NULL, NULL,
                                                      GIMP_TYPE_VIEW_TYPE,
                                                      GIMP_VIEW_TYPE_LIST,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CONTAINER,
                                   g_param_spec_object ("container",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CONTAINER,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CONTEXT,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_VIEW_SIZE,
                                   g_param_spec_int ("view-size",
                                                     NULL, NULL,
                                                     1, GIMP_VIEWABLE_MAX_PREVIEW_SIZE,
                                                     GIMP_VIEW_SIZE_MEDIUM,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_VIEW_BORDER_WIDTH,
                                   g_param_spec_int ("view-border-width",
                                                     NULL, NULL,
                                                     0,
                                                     GIMP_VIEW_MAX_BORDER_WIDTH,
                                                     1,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_MENU_FACTORY,
                                   g_param_spec_object ("menu-factory",
                                                        NULL, NULL,
                                                        GIMP_TYPE_MENU_FACTORY,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_MENU_IDENTIFIER,
                                   g_param_spec_string ("menu-identifier",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_UI_PATH,
                                   g_param_spec_string ("ui-path",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_container_editor_docked_iface_init (GimpDockedInterface *iface)
{
  iface->get_preview         = gimp_container_editor_get_preview;
  iface->set_context         = gimp_container_editor_set_context;
  iface->get_menu            = gimp_container_editor_get_menu;
  iface->has_button_bar      = gimp_container_editor_has_button_bar;
  iface->set_show_button_bar = gimp_container_editor_set_show_button_bar;
  iface->get_show_button_bar = gimp_container_editor_get_show_button_bar;
}

static void
gimp_container_editor_init (GimpContainerEditor *editor)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (editor),
                                  GTK_ORIENTATION_VERTICAL);

  editor->priv = gimp_container_editor_get_instance_private (editor);
}

static void
gimp_container_editor_constructed (GObject *object)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_CONTAINER (editor->priv->container));
  gimp_assert (GIMP_IS_CONTEXT (editor->priv->context));

  switch (editor->priv->view_type)
    {
    case GIMP_VIEW_TYPE_GRID:
      editor->view =
        GIMP_CONTAINER_VIEW (gimp_container_icon_view_new (editor->priv->container,
                                                           editor->priv->context,
                                                           editor->priv->view_size,
                                                           editor->priv->view_border_width));
      break;

    case GIMP_VIEW_TYPE_LIST:
      editor->view =
        GIMP_CONTAINER_VIEW (gimp_container_tree_view_new (editor->priv->container,
                                                           editor->priv->context,
                                                           editor->priv->view_size,
                                                           editor->priv->view_border_width));
      break;

    default:
      gimp_assert_not_reached ();
    }

  gimp_editor_set_show_name (GIMP_EDITOR (editor->view),
                             (editor->priv->view_type == GIMP_VIEW_TYPE_GRID));

  if (GIMP_IS_LIST (editor->priv->container))
    gimp_container_view_set_reorderable (GIMP_CONTAINER_VIEW (editor->view),
                                         ! gimp_list_get_sort_func (GIMP_LIST (editor->priv->container)));

  if (editor->priv->menu_factory    &&
      editor->priv->menu_identifier &&
      editor->priv->ui_path)
    {
      gimp_editor_create_menu (GIMP_EDITOR (editor->view),
                               editor->priv->menu_factory,
                               editor->priv->menu_identifier,
                               editor->priv->ui_path,
                               editor);
    }

  gtk_box_pack_start (GTK_BOX (editor), GTK_WIDGET (editor->view),
                      TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (editor->view));

  editor->priv->busy_box = gimp_busy_box_new (NULL);
  gtk_box_pack_start (GTK_BOX (editor), editor->priv->busy_box, TRUE, TRUE, 0);

  g_object_bind_property (editor->priv->busy_box, "visible",
                          editor->view,           "visible",
                          G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);

  g_signal_connect_object (editor->view, "selection-changed",
                           G_CALLBACK (gimp_container_editor_selection_changed),
                           editor, 0);

  g_signal_connect_object (editor->view, "item-activated",
                           G_CALLBACK (gimp_container_editor_item_activated),
                           editor, 0);
}

static void
gimp_container_editor_dispose (GObject *object)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (object);

  gimp_container_editor_bind_to_async_set (editor, NULL, NULL);

  g_clear_object (&editor->priv->container);
  g_clear_object (&editor->priv->context);
  g_clear_object (&editor->priv->menu_factory);

  g_clear_pointer (&editor->priv->menu_identifier, g_free);
  g_clear_pointer (&editor->priv->ui_path,         g_free);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_container_editor_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (object);

  switch (property_id)
    {
    case PROP_VIEW_TYPE:
      editor->priv->view_type = g_value_get_enum (value);
      break;

    case PROP_CONTAINER:
      editor->priv->container = g_value_dup_object (value);
      break;

    case PROP_CONTEXT:
      editor->priv->context = g_value_dup_object (value);
      break;

    case PROP_VIEW_SIZE:
      editor->priv->view_size = g_value_get_int (value);
      break;

    case PROP_VIEW_BORDER_WIDTH:
      editor->priv->view_border_width = g_value_get_int (value);
      break;

    case PROP_MENU_FACTORY:
      editor->priv->menu_factory = g_value_dup_object (value);
      break;

    case PROP_MENU_IDENTIFIER:
      editor->priv->menu_identifier = g_value_dup_string (value);
      break;

    case PROP_UI_PATH:
      editor->priv->ui_path = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_container_editor_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (object);

  switch (property_id)
    {
    case PROP_VIEW_TYPE:
      g_value_set_enum (value, editor->priv->view_type);
      break;

    case PROP_CONTAINER:
      g_value_set_object (value, editor->priv->container);
      break;

    case PROP_CONTEXT:
      g_value_set_object (value, editor->priv->context);
      break;

    case PROP_VIEW_SIZE:
      g_value_set_int (value, editor->priv->view_size);
      break;

    case PROP_VIEW_BORDER_WIDTH:
      g_value_set_int (value, editor->priv->view_border_width);
      break;

    case PROP_MENU_FACTORY:
      g_value_set_object (value, editor->priv->menu_factory);
      break;

    case PROP_MENU_IDENTIFIER:
      g_value_set_string (value, editor->priv->menu_identifier);
      break;

    case PROP_UI_PATH:
      g_value_set_string (value, editor->priv->ui_path);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkSelectionMode
gimp_container_editor_get_selection_mode (GimpContainerEditor *editor)
{
  return gimp_container_view_get_selection_mode (GIMP_CONTAINER_VIEW (editor->view));
}

void
gimp_container_editor_set_selection_mode (GimpContainerEditor *editor,
                                          GtkSelectionMode     mode)
{
  gimp_container_view_set_selection_mode (GIMP_CONTAINER_VIEW (editor->view),
                                          mode);
}


/*  private functions  */

static void
gimp_container_editor_selection_changed (GimpContainerView   *view,
                                         GimpContainerEditor *editor)
{
  GimpContainerEditorClass *klass       = GIMP_CONTAINER_EDITOR_GET_CLASS (editor);
  GimpViewable             *viewable    = NULL;
  GimpEditor               *editor_view = NULL;
  GList                    *items;

  gimp_container_view_get_selected (view, &items);

  /* XXX Right now a GimpContainerEditor only supports 1 item selected
   * at once. Let's see later if we want to allow more.
   *
   * g_warn_if_fail (n_items < 2);
   */

  /* The editor view may get destroyed through a chain of signals when
   * changing the context viewables with gimp_context_set_by_type().
   * We make sure it still exists before working on it with a weak
   * pointer.
   */
  g_set_weak_pointer (&editor_view, GIMP_EDITOR (editor->view));

  if (items)
    viewable = items->data;

  if (klass->select_item)
    klass->select_item (editor, viewable);

  if (viewable && editor_view)
    {
      gchar *desc = gimp_viewable_get_description (viewable, NULL);

      gimp_editor_set_name (editor_view, desc);
      g_free (desc);
    }

  if (editor_view && gimp_editor_get_ui_manager (editor_view))
    gimp_ui_manager_update (gimp_editor_get_ui_manager (editor_view),
                            gimp_editor_get_popup_data (editor_view));

  g_clear_weak_pointer (&editor_view);

  g_list_free (items);
}

static void
gimp_container_editor_item_activated (GtkWidget           *widget,
                                      GimpViewable        *viewable,
                                      GimpContainerEditor *editor)
{
  GimpContainerEditorClass *klass = GIMP_CONTAINER_EDITOR_GET_CLASS (editor);

  if (klass->activate_item)
    klass->activate_item (editor, viewable);
}

static GtkWidget *
gimp_container_editor_get_preview (GimpDocked   *docked,
                                   GimpContext  *context,
                                   GtkIconSize   size)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (docked);

  return gimp_docked_get_preview (GIMP_DOCKED (editor->view),
                                  context, size);
}

static void
gimp_container_editor_set_context (GimpDocked  *docked,
                                   GimpContext *context)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (docked);

  gimp_docked_set_context (GIMP_DOCKED (editor->view), context);
}

static GimpUIManager *
gimp_container_editor_get_menu (GimpDocked   *docked,
                                const gchar **ui_path,
                                gpointer     *popup_data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (docked);

  return gimp_docked_get_menu (GIMP_DOCKED (editor->view), ui_path, popup_data);
}

static gboolean
gimp_container_editor_has_button_bar (GimpDocked *docked)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (docked);

  return gimp_docked_has_button_bar (GIMP_DOCKED (editor->view));
}

static void
gimp_container_editor_set_show_button_bar (GimpDocked *docked,
                                           gboolean    show)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (docked);

  gimp_docked_set_show_button_bar (GIMP_DOCKED (editor->view), show);
}

static gboolean
gimp_container_editor_get_show_button_bar (GimpDocked *docked)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (docked);

  return gimp_docked_get_show_button_bar (GIMP_DOCKED (editor->view));
}

void
gimp_container_editor_bind_to_async_set (GimpContainerEditor *editor,
                                         GimpAsyncSet        *async_set,
                                         const gchar         *message)
{
  g_return_if_fail (GIMP_IS_CONTAINER_EDITOR (editor));
  g_return_if_fail (async_set == NULL || GIMP_IS_ASYNC_SET (async_set));
  g_return_if_fail (async_set == NULL || message != NULL);

  if (! async_set && ! editor->priv->async_set_binding)
    return;

  g_clear_object (&editor->priv->async_set_binding);

  if (async_set)
    {
      gimp_busy_box_set_message (GIMP_BUSY_BOX (editor->priv->busy_box),
                                 message);

      editor->priv->async_set_binding = g_object_bind_property (
        async_set,              "empty",
        editor->priv->busy_box, "visible",
        G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);
    }
  else
    {
      gtk_widget_hide (editor->priv->busy_box);
    }
}
