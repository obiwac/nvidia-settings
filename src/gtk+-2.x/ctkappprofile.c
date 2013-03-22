/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2013 NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include "ctkutils.h"
#include "ctkbanner.h"
#include "ctkhelp.h"
#include "ctkappprofile.h"
#include "common-utils.h"
#include "msg.h"

#define UPDATE_RULE_LABEL "Update Rule"
#define UPDATE_PROFILE_LABEL "Update Profile"

#define STATUSBAR_UPDATE_WARNING "This will take effect after changes are saved."

enum {
    PROFILE_SETTING_GL_FSAA_MODE,
    PROFILE_SETTING_GL_LOG_MAX_ANISO,
    PROFILE_SETTING_GL_NO_DSO_FINALIZER,
    PROFILE_SETTING_GL_SINGLE_THREADED,
    PROFILE_SETTING_GL_SYNC_DISPLAY_DEVICE,
    PROFILE_SETTING_GL_SYNC_TO_VBLANK,
    PROFILE_SETTING_GL_SORT_FBCONFIGS,
    PROFILE_SETTING_GL_ALLOW_UNOFFICIAL_PROTOCOL,
    PROFILE_SETTING_GL_SELINUX_BOOLEANS,
    PROFILE_SETTING_GL_SHADER_DISK_CACHE,
    PROFILE_SETTING_GL_SHADER_DISK_CACHE_PATH,
    PROFILE_SETTING_GL_YIELD,
    PROFILE_SETTING_GL_THREADED_OPTIMIZATIONS,
    PROFILE_SETTING_GL_DOOM3,
    PROFILE_SETTING_GL_EXTENSION_STRING_VERSION,
    NUM_PROFILE_SETTINGS
};

static const char *profile_setting_keys[] = {
    "GLFSAAMode",                   // PROFILE_SETTING_GL_FSAA_MODE
    "GLLogMaxAniso",                // PROFILE_SETTING_GL_LOG_MAX_ANISO
    "GLNoDsoFinalizer",             // PROFILE_SETTING_GL_NO_DSO_FINALIZER
    "GLSingleThreaded",             // PROFILE_SETTING_GL_SINGLE_THREADED
    "GLSyncDisplayDevice",          // PROFILE_SETTING_GL_SYNC_DISPLAY_DEVICE
    "GLSyncToVblank",               // PROFILE_SETTING_GL_SYNC_TO_VBLANK
    "GLSortFbconfigs",              // PROFILE_SETTING_GL_SORT_FBCONFIGS
    "GLAllowUnofficialProtocol",    // PROFILE_SETTING_GL_ALLOW_UNOFFICIAL_PROTOCOL
    "GLSELinuxBooleans",            // PROFILE_SETTING_GL_SELINUX_BOOLEANS
    "GLShaderDiskCache",            // PROFILE_SETTING_GL_SHADER_DISK_CACHE
    "GLShaderDiskCachePath",        // PROFILE_SETTING_GL_SHADER_DISK_CACHE_PATH
    "GLYield",                      // PROFILE_SETTING_GL_YIELD
    "GLThreadedOptimizations",      // PROFILE_SETTING_GL_THREADED_OPTIMIZATIONS
    "GLDoom3",                      // PROFILE_SETTING_GL_DOOM3
    "GLExtensionStringVersion"      // PROFILE_SETTING_GL_EXTENSION_STRING_VERSION
};

/*
 * XXX: it might be a good idea to generate some of these descriptions
 * dynamically based on other tables used by nvidia-settings
 */
static const char *profile_setting_descriptions[] = {
    // PROFILE_SETTING_GL_FSAA_MODE
    "This setting enables full-scene antialiasing in a process using OpenGL. This expects "
    "the same integer value that can be used to configure FSAA through nvidia-settings "
    "and the NV-CONTROL X extension. To see available FSAA values, run:\n\n"
    "\tnvidia-settings --query=fsaa --verbose",
    // PROFILE_SETTING_GL_LOG_MAX_ANISO
    "This enables anisotropic texture filtering. The possible values are:\n\n"
    "\t0\tNo anisotropic filtering\n"
    "\t1\t2x anisotropic filtering\n"
    "\t2\t4x anisotropic filtering\n"
    "\t3\t8x anisotropic filtering\n"
    "\t4\t16x anisotropic filtering",
    // PROFILE_SETTING_GL_NO_DSO_FINALIZER
    "This works around problems with certain multithreaded applications in which "
    "one thread exits while others are executing OpenGL code. This may be set to true or false.",
    // PROFILE_SETTING_GL_SINGLE_THREADED
    "This works around some legacy dynamic loaders which can cause applications linked against pthreads "
    "which dlopen() libGL multiple times to crash. This may be set to true or false. ",
    // PROFILE_SETTING_GL_SYNC_DISPLAY_DEVICE
    "This allows an application to specify target a display device to sync with if sync to vblank is enabled. "
    "This should be set to a string containing a valid display device name (for example, \"CRT-1\").",
    // PROFILE_SETTING_GL_SYNC_TO_VBLANK
    "This enables sync to vblank for an application. This may be set to true or false. ",
    // PROFILE_SETTING_GL_SORT_FBCONFIGS
    "By default the NVIDIA GLX implementation will sort FBConfigs as described by the specification. This "
    "may be set to false to disable this behavior.",
    // PROFILE_SETTING_GL_ALLOW_UNOFFICIAL_PROTOCOL
    "Setting this to true will allow the client-side NVIDIA GLX implementation to send \"incomplete\" GLX protocol.",
    // PROFILE_SETTING_GL_SELINUX_BOOLEANS
    "This allows the user to override driver detection of specific SELinux policy booleans, which may "
    "work around problems when running the driver under SELinux in permissive mode. This should be set to a "
    "string value; see __GL_SELINUX_BOOLEANS in the README for a description of legal string formats.",
    // PROFILE_SETTING_GL_SHADER_DISK_CACHE
    "This enables the shader disk cache for direct rendering. This value may be set to true or false.",
    // PROFILE_SETTING_GL_SHADER_DISK_CACHE_PATH
    "This setting affects where shader caches are stored on disk for a given application. "
    "This value should be set to a string containing a valid pathname.",
    // PROFILE_SETTING_GL_YIELD
    "This controls how the NVIDIA graphics driver will perform a yield. This may be set to one of the following strings:\n\n"
    "\t\"USLEEP\"\tOpenGL will call usleep(0) to yield\n"
    "\t\"NOTHING\"\tOpenGL will never yield\n"
    "\t<any other value>\tOpenGL will call sched_yield() to yield (default)",
    // PROFILE_SETTING_GL_THREADED_OPTIMIZATIONS
    "This setting enables multi-threaded optimizations in the OpenGL driver which may improve application performance. "
    "This may be set to true or false.",
    // PROFILE_SETTING_GL_DOOM3
    "This enables optimal SLI and Multi-GPU settings for games such as Doom 3 and Quake 4. "
    "This may be set to true or false.",
    // PROFILE_SETTING_GL_EXTENSION_STRING_VERSION
    "This forces the extension string returned by glXQueryExtensionsString() to one that appeared in an earlier "
    "version of the NVIDIA graphics driver. This may work around bugs in certain applications which expect an extension "
    "string to be smaller than a certain size. This value should be set to a string value or integer containing the "
    "desired version number (e.g. \"17700\" to force the extension string in the 177.* driver series).",
};

enum {
   RULE_FEATURE_PROCNAME,
   RULE_FEATURE_DSO,
   RULE_FEATURE_TRUE,
   NUM_RULE_FEATURES
};

static const char *rule_feature_label_strings[] = {
    "Process Name (procname)",      // RULE_FEATURE_PROCNAME
    "Shared Object Name (dso)",     // RULE_FEATURE_DSO
    "Always Applies (true)"         // RULE_FEATURE_TRUE
};

static const char *rule_feature_identifiers[] = {
    "procname",                     // RULE_FEATURE_PROCNAME
    "dso",                          // RULE_FEATURE_DSO
    "true"                          // RULE_FEATURE_TRUE
};

#define MATCHES_INPUT_DESCRIPTION "\"Matches this string...\" text entry box"

static const char *rule_feature_help_text[] = {
    "Patterns using this feature compare the string provided by the " MATCHES_INPUT_DESCRIPTION " "
    "against the pathname of the current process with the leading directory components removed, "
    "and match if they are equal.", // RULE_FEATURE_PROCNAME
    "Patterns using this feature compare the string provided by the " MATCHES_INPUT_DESCRIPTION " "
    "against the list of currently loaded libraries in the current process, and match if "
    "the string matches one of the entries in the list (with leading directory components removed).", // RULE_FEATURE_DSO
    "Patterns using this feature will always match the process, regardless of the "
    "contents of the string specified in the " MATCHES_INPUT_DESCRIPTION ".", // RULE_FEATURE_TRUE
};

enum {
    SETTING_LIST_STORE_COL_SETTING,
    SETTING_LIST_STORE_NUM_COLS,
};

/*
 * Struct containing metadata on widgets created via
 * populate_{toolbar,tree_view}().
 */
typedef struct _WidgetDataItem {
    gchar *label;
    GtkWidget *widget;
} WidgetDataItem;

/*
 * Template used to construct toolbar buttons and generate help text with populate_toolbar().
 */
typedef struct _ToolbarItemTemplate {
    const gchar *text;
    const gchar *icon_id;
    GCallback callback;
    gpointer user_data;

    const gchar *help_text;
    const gchar *extended_help_text;
} ToolbarItemTemplate;

/*
 * Template used to construct tree view columns and generate help text with
 * populate_tree_view().
 */
typedef struct _TreeViewColumnTemplate {
    const gchar *title;

    GtkTreeCellDataFunc renderer_func;
    gpointer func_data;

    const gchar *attribute;
    gint attr_col;
    gint min_width;

    gboolean sortable;
    gint sort_column_id;

    gboolean editable;
    GCallback edit_callback;

    const gchar *help_text;
    const gchar *extended_help_text;
} TreeViewColumnTemplate;

#if JSON_INTEGER_IS_LONG_LONG
# define JSON_INTEGER_HEX_FORMAT "llx"
#else
# define JSON_INTEGER_HEX_FORMAT "lx"
#endif

/*
 * Function prototypes
 */
static void app_profile_class_init(CtkAppProfileClass *ctk_object_class);
static void app_profile_finalize(GObject *object);
static void edit_rule_dialog_destroy(EditRuleDialog *dialog);
static void edit_profile_dialog_destroy(EditProfileDialog *dialog);
static void save_app_profile_changes_dialog_destroy(SaveAppProfileChangesDialog *dialog);

/*
 * Get a UTF8 bullet string suitable for printing
 */
static const char *get_bullet(void)
{
    gint len;
    static gboolean initialized = FALSE;
    static gchar bullet[8];

    // Convert unicode "bullet" character into a UTF8 string
    if (!initialized) {
        len = g_unichar_to_utf8(0x2022, bullet);
        bullet[len] = '\0';
        initialized = TRUE;
    }
    return bullet;
}

static char *vmarkup_string(const char *s, gboolean add_markup, const char *tag, va_list ap)
{
    char *escaped_s;
    GString *tagged_str;
    char *tagged_s_return;
    char *attrib, *attrib_val;

    if (!add_markup) {
        return strdup(s);
    }

    tagged_str = g_string_new("");
    escaped_s = g_markup_escape_text(s, -1);

    g_string_append_printf(tagged_str, "<%s ", tag);
    do {
        attrib = va_arg(ap, char *);
        if (attrib) {
            attrib_val = va_arg(ap, char *);
            g_string_append_printf(tagged_str, "%s=\"%s\"", attrib, attrib_val);
        }
    } while (attrib);
    g_string_append_printf(tagged_str, ">%s</%s>", escaped_s, tag);

    tagged_s_return = strdup(tagged_str->str);
    g_string_free(tagged_str, TRUE);
    free(escaped_s);

    return tagged_s_return;
}

static char *markup_string(const char *s, gboolean add_markup, const char *tag, ...)
{
    char *tagged_s;
    va_list ap;
    va_start(ap, tag);
    tagged_s = vmarkup_string(s, add_markup, tag, ap);
    va_end(ap);

    return tagged_s;
}


GType ctk_app_profile_get_type(void)
{
    static GType ctk_app_profile_type = 0;

    if (!ctk_app_profile_type) {
        static const GTypeInfo ctk_app_profile_info = {
            sizeof (CtkAppProfileClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) app_profile_class_init, /* constructor */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkAppProfile),
            0,    /* n_preallocs */
            NULL, /* instance_init */
            NULL  /* value_table */
        };

        ctk_app_profile_type =
            g_type_register_static(GTK_TYPE_VBOX, "CtkAppProfile",
                                   &ctk_app_profile_info, 0);
    }

    return ctk_app_profile_type;

} /* ctk_app_profile_get_type() */

static void app_profile_class_init(CtkAppProfileClass *ctk_object_class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(ctk_object_class);
    gobject_class->finalize = app_profile_finalize;
}

static void app_profile_finalize(GObject *object)
{
    CtkAppProfile *ctk_app_profile = CTK_APP_PROFILE(object);

    edit_rule_dialog_destroy(ctk_app_profile->edit_rule_dialog);
    edit_profile_dialog_destroy(ctk_app_profile->edit_profile_dialog);
    save_app_profile_changes_dialog_destroy(ctk_app_profile->save_app_profile_changes_dialog);

    ctk_help_data_list_free_full(ctk_app_profile->global_settings_help_data);
    ctk_help_data_list_free_full(ctk_app_profile->rules_help_data);
    ctk_help_data_list_free_full(ctk_app_profile->rules_columns_help_data);
    ctk_help_data_list_free_full(ctk_app_profile->profiles_help_data);
    ctk_help_data_list_free_full(ctk_app_profile->profiles_columns_help_data);
    ctk_help_data_list_free_full(ctk_app_profile->save_reload_help_data);
}

static void button_set_label_and_stock_icon(GtkButton *button, const gchar *label_text, const gchar *icon_id)
{
    GtkWidget *hbox;
    GtkWidget *icon;
    GtkWidget *label;
    GtkWidget *button_child;
    hbox = gtk_hbox_new(FALSE, 0);
    icon = gtk_image_new_from_stock(icon_id, GTK_ICON_SIZE_SMALL_TOOLBAR);
    label = gtk_label_new(label_text);
    gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
    button_child = gtk_bin_get_child(GTK_BIN(button));
    if (button_child) {
        gtk_container_remove(GTK_CONTAINER(button), button_child);
    }
    gtk_widget_show_all(GTK_WIDGET(hbox));
    gtk_container_add(GTK_CONTAINER(button), hbox);
}

static gint find_widget_in_widget_data_list_cb(gconstpointer a, gconstpointer b)
{
    const WidgetDataItem *item = (const WidgetDataItem *)a;
    const char *label = (const char *)b;

    return strcmp(item->label, label);
}

static GtkWidget *find_widget_in_widget_data_list(GList *list, const char *label)
{
    GList *list_item;
    WidgetDataItem *item;

    list_item = g_list_find_custom(list, (gconstpointer)label, find_widget_in_widget_data_list_cb);
    assert(list_item);

    item = (WidgetDataItem *)list_item->data;
    return item->widget;
}

static void widget_data_list_free_cb(gpointer data, gpointer user_data)
{
    WidgetDataItem *item = (WidgetDataItem *)data;
    free(item->label);
    // Don't free item->widget; it's owned by its parent widget
    free(item);
}

static void widget_data_list_free_full(GList *list)
{
    g_list_foreach(list, widget_data_list_free_cb, NULL);
    g_list_free(list);
}


/* Simple helper function to fill a toolbar with buttons from a table */
static void populate_toolbar(GtkToolbar *toolbar,
                             const ToolbarItemTemplate *item,
                             size_t num_items,
                             GList **help_data,
                             GList **widget_data)
{
    WidgetDataItem *widget_data_item;
    GtkWidget *widget;
    GtkWidget *icon;

    if (help_data) {
        *help_data = NULL;
    }
    if (widget_data) {
        *widget_data = NULL;
    }

    while (num_items--) {
        if (item->icon_id) {
            icon = gtk_image_new_from_stock(item->icon_id, GTK_ICON_SIZE_SMALL_TOOLBAR);
        } else {
            icon = NULL;
        }
        widget = gtk_toolbar_append_item(toolbar,
                                         item->text,
                                         item->help_text,
                                         NULL,
                                         icon,
                                         item->callback,
                                         item->user_data);
        if (help_data) {
            ctk_help_data_list_prepend(help_data,
                                       item->text,
                                       item->help_text,
                                       item->extended_help_text);
        }
        if (widget_data) {
            widget_data_item = malloc(sizeof(WidgetDataItem));
            widget_data_item->label = strdup(item->text);
            widget_data_item->widget = widget;
            *widget_data = g_list_prepend(*widget_data, widget_data_item);
        }
        item++;
    }

    if (help_data) {
        *help_data = g_list_reverse(*help_data);
    }
    if (widget_data) {
        *widget_data = g_list_reverse(*widget_data);
    }
}

typedef struct CellRendererRegisterKeyDataRec {
    GtkTreeView *tree_view;
} CellRendererRegisterKeyData;

static void destroy_cell_renderer_register_key_data(gpointer data, GClosure *closure)
{
    free((CellRendererRegisterKeyData *)data);
}

static void tree_view_get_cursor_path_and_column_idx(GtkTreeView *tree_view,
                                                     GtkTreePath **path,
                                                     gint *column_idx,
                                                     gint *column_count)
{
    GtkTreeViewColumn *focus_column;
    GList *column_list, *column_in_list;

    column_list = gtk_tree_view_get_columns(tree_view);
    gtk_tree_view_get_cursor(tree_view, path, &focus_column);

    // Lame...
    column_in_list = g_list_find(column_list, focus_column);
    *column_idx = g_list_position(column_list, column_in_list);

    *column_count = g_list_length(column_list);

    g_list_free(column_list);
}

static void cell_renderer_editable(gpointer data, gpointer user_data)
{
    gboolean this_editable;
    GtkCellRenderer *renderer = GTK_CELL_RENDERER(data);
    gboolean *editable = (gboolean *)user_data;

    if (*editable) {
        return;
    }

    g_object_get(G_OBJECT(renderer), "editable", &this_editable, NULL);

    if (this_editable) {
        *editable = TRUE;
    }
}

static gboolean tree_view_column_is_editable(GtkTreeViewColumn *tree_column)
{
    gboolean editable = FALSE;
    GList *renderers = gtk_tree_view_column_get_cell_renderers(tree_column);
    g_list_foreach(renderers, cell_renderer_editable, &editable);
    return editable;
}

static gboolean cell_renderer_widget_key_press_event(GtkWidget *widget,
                                                      GdkEvent *event,
                                                      gpointer user_data)
{
    CellRendererRegisterKeyData *data = (CellRendererRegisterKeyData *)user_data;
    GtkTreeView *tree_view;
    GtkTreeModel *tree_model;
    GdkEventKey *key_event;
    gint column_idx, column_count;
    GtkTreeViewColumn *column;
    GtkTreePath *path;
    gint depth, *indices, row_idx;
    gint row_count;
    gint dx, dy;

    if (event->type == GDK_KEY_PRESS) {
        key_event = (GdkEventKey *)event;
        dx = dy = 0;

        if ((key_event->keyval == GDK_Tab) ||
            (key_event->keyval == GDK_ISO_Left_Tab)) {
            dx = (key_event->state & GDK_SHIFT_MASK) ? -1 : 1;
        } else if (key_event->keyval == GDK_Up) {
            dy = -1;
        } else if ((key_event->keyval == GDK_Down) ||
                   (key_event->keyval == GDK_Return)) {
            dy = 1;
        }

        if (dx || dy) {
            assert(!dx || !dy);
            tree_view = data->tree_view;
            tree_model = gtk_tree_view_get_model(tree_view);

            row_count = gtk_tree_model_iter_n_children(tree_model, NULL);

            // Done editing this cell
            gtk_cell_editable_editing_done(GTK_CELL_EDITABLE(widget));
            gtk_cell_editable_remove_widget(GTK_CELL_EDITABLE(widget));

            // Get currently highlighted row
            tree_view_get_cursor_path_and_column_idx(tree_view,
                                                     &path,
                                                     &column_idx,
                                                     &column_count);

            depth = gtk_tree_path_get_depth(path);
            assert(depth == 1);
            (void)depth;

            indices = gtk_tree_path_get_indices(path);
            row_idx = indices[0];

            gtk_tree_path_free(path);

            if (dx) {
                do {
                    column_idx += dx;

                    assert(column_count >= 1);

                    if (column_idx < 0) {
                        // go to previous row, if possible
                        row_idx--;
                        column_idx = column_count - 1;
                    } else if (column_idx >= column_count) {
                        // go to next row, if possible
                        row_idx++;
                        column_idx = 0;
                    }

                    column = gtk_tree_view_get_column(tree_view, column_idx);
                } while (!tree_view_column_is_editable(column) &&
                         (row_idx >= 0) && (row_idx < row_count));
            } else {
                row_idx += dy;
                column = gtk_tree_view_get_column(tree_view, column_idx);
            }

            if ((row_idx >= 0) && (row_idx < row_count)) {
                path = gtk_tree_path_new();
                gtk_tree_path_append_index(path, row_idx);

                gtk_tree_view_set_cursor(tree_view, path, column, TRUE);

                gtk_tree_path_free(path);
            }

            return TRUE;
        }
    }

    // Use default handlers
    return FALSE;
}

static gboolean cell_renderer_widget_focus_out_event(GtkWidget *widget,
                                                     GdkEvent *event,
                                                     gpointer user_data)
{
    // Done editing this cell
    gtk_cell_editable_editing_done(GTK_CELL_EDITABLE(widget));
    gtk_cell_editable_remove_widget(GTK_CELL_EDITABLE(widget));

    // Use default handlers
    return FALSE;
}


static void cell_renderer_register_key_shortcuts(GtkCellRenderer *renderer,
                                                 GtkCellEditable *editable,
                                                 gchar *path,
                                                 gpointer user_data)
{
    if (GTK_IS_WIDGET(editable)) {
        GtkWidget *widget = GTK_WIDGET(editable);
        g_signal_connect(G_OBJECT(widget), "key-press-event",
                         G_CALLBACK(cell_renderer_widget_key_press_event),
                         user_data);
        g_signal_connect(G_OBJECT(widget), "focus-out-event",
                         G_CALLBACK(cell_renderer_widget_focus_out_event),
                         user_data);
    } else {
        // XXX implement other types?
    }
}

/* Simple helper function to fill a tree view with text columns */
static void populate_tree_view(GtkTreeView *tree_view,
                               const TreeViewColumnTemplate *column_template,
                               CtkAppProfile *ctk_app_profile,
                               size_t num_columns,
                               GList **help_data)
{
    GtkCellRenderer *cell_renderer;
    GtkTreeViewColumn *tree_view_column;
    GtkWidget *label;

    if (help_data) {
        *help_data = NULL;
    }

    while (num_columns--) {
        cell_renderer = gtk_cell_renderer_text_new();
        tree_view_column = gtk_tree_view_column_new();

        label = gtk_label_new(column_template->title);
        if (column_template->help_text) {
            ctk_config_set_tooltip(ctk_app_profile->ctk_config,
                                   label,
                                   column_template->help_text);
        }
        // Necessary since the label isn't part of the CtkAppProfile's hierarchy
        gtk_widget_show(label);

        gtk_tree_view_column_set_widget(tree_view_column, label);


        gtk_tree_view_column_pack_start(tree_view_column, cell_renderer, FALSE);

        if (column_template->renderer_func) {
            assert(!column_template->attribute);
            gtk_tree_view_column_set_cell_data_func(tree_view_column,
                                                    cell_renderer,
                                                    column_template->renderer_func,
                                                    column_template->func_data,
                                                    NULL);
        } else {
            gtk_tree_view_column_add_attribute(tree_view_column,
                                               cell_renderer,
                                               column_template->attribute,
                                               column_template->attr_col);
            assert(column_template->attribute);
        }

        if (column_template->min_width > 0) {
            gtk_tree_view_column_set_min_width(tree_view_column, column_template->min_width);
        }

        if (column_template->sortable) {
            gtk_tree_view_column_set_sort_column_id(tree_view_column, column_template->sort_column_id);
        }

        if (column_template->editable) {
            CellRendererRegisterKeyData *rk_data = malloc(sizeof(CellRendererRegisterKeyData));
            rk_data->tree_view = tree_view;
            g_object_set(G_OBJECT(cell_renderer), "editable", TRUE, NULL);
            g_signal_connect(G_OBJECT(cell_renderer), "edited", column_template->edit_callback,
                             column_template->func_data);

            // Generic code to implement navigating between fields with tab/shift-tab
            g_signal_connect_data(G_OBJECT(cell_renderer), "editing-started",
                                  G_CALLBACK(cell_renderer_register_key_shortcuts),
                                  (gpointer)rk_data, destroy_cell_renderer_register_key_data,
                                  0);
        }

        if (help_data) {
            ctk_help_data_list_prepend(help_data,
                                       column_template->title,
                                       column_template->help_text,
                                       column_template->extended_help_text);
        }


        gtk_tree_view_append_column(tree_view, tree_view_column);
        column_template++;
    }

    if (help_data) {
        *help_data = g_list_reverse(*help_data);
    }
}

static void rule_order_renderer_func(GtkTreeViewColumn *tree_column,
                                     GtkCellRenderer   *cell,
                                     GtkTreeModel      *model,
                                     GtkTreeIter       *iter,
                                     gpointer           data)
{
    char *markup;
    GtkTreePath *path;
    gint *indices, depth;

    path = gtk_tree_model_get_path(model, iter);
    indices = gtk_tree_path_get_indices(path);
    depth = gtk_tree_path_get_depth(path);

    assert(depth == 1);
    (void)depth;

    markup = nvasprintf("%d", indices[0] + 1);

    g_object_set(cell, "markup", markup, NULL);

    free(markup);
    gtk_tree_path_free(path);
}

static void rule_pattern_renderer_func(GtkTreeViewColumn *tree_column,
                                       GtkCellRenderer   *cell,
                                       GtkTreeModel      *model,
                                       GtkTreeIter       *iter,
                                       gpointer           data)
{
    char *feature, *matches;
    char *feature_plain;
    char *feature_markup;
    char *matches_markup;
    char *markup;

    gtk_tree_model_get(model, iter,
                       CTK_APC_RULE_MODEL_COL_FEATURE, &feature,
                       CTK_APC_RULE_MODEL_COL_MATCHES, &matches,
                       -1);

    feature_plain = nvasprintf("[%s]", feature);
    feature_markup = markup_string(feature_plain, TRUE, "span", "color", "#444411",
                                   "style", "italic", NULL);
    matches_markup = g_markup_escape_text(matches, -1);

    markup = nvstrcat(feature_markup, " ", matches_markup, NULL);

    g_object_set(cell, "markup", markup, NULL);

    free(markup);
    free(feature_markup);
    free(matches_markup);
    free(feature_plain);
    free(feature);
    free(matches);
}


static inline void setting_get_key_value(const json_t *setting,
                                         char **key,
                                         char **value,
                                         gboolean add_markup)
{
    json_t *json_value;
    const char *plain_key;
    char *plain_value;

    if (key) {
        plain_key = json_string_value(json_object_get(setting, "key"));
        *key = markup_string(plain_key, add_markup, "span", "color", "#000033", NULL);
    }

    if (value) {
        json_value = json_object_get(setting, "value");
        switch(json_typeof(json_value)) {
        case JSON_STRING:
        case JSON_TRUE:
        case JSON_FALSE:
        case JSON_REAL:
            plain_value = json_dumps(json_value, JSON_ENCODE_ANY);
            break;
        case JSON_INTEGER:
            // Prefer hex to integer values
            plain_value = nvasprintf("0x%" JSON_INTEGER_HEX_FORMAT, json_integer_value(json_value));
            break;
        default:
            plain_value = strdup("?");
            assert(0);
        }
        *value = markup_string(plain_value, add_markup, "span", "color", "#003300", NULL);
        free(plain_value);
    }
}

char *serialize_settings(const json_t *settings, gboolean add_markup)
{
    char *old_markup, *markup;
    char *one_setting, *value;
    char *key;
    json_t *setting;
    size_t i, size;

    if (!settings) {
        return markup_string("(no such profile)", add_markup,
                             "span", "color", "#555555", NULL);
    }

    old_markup = strdup("");
    for (i = 0, size = json_array_size(settings); i < size; i++) {
        if (i != 0) {
            markup = nvstrcat(old_markup, ", ", NULL);
            free(old_markup);
            old_markup = markup;
        }
        setting = json_array_get(settings, i);
        setting_get_key_value(setting, &key, &value, add_markup);
        one_setting = nvasprintf("%s=%s", key, value);

        markup = nvstrcat(old_markup, one_setting, NULL);
        free(one_setting);
        free(key);
        free(value);
        free(old_markup);
        old_markup = markup;
    }

    return old_markup;
}

static void rule_profile_settings_renderer_func(GtkTreeViewColumn *tree_column,
                                                GtkCellRenderer   *cell,
                                                GtkTreeModel      *model,
                                                GtkTreeIter       *iter,
                                                gpointer           data)
{
    CtkAppProfile *ctk_app_profile = (CtkAppProfile *)data;
    char *settings_string;
    const json_t *profile;
    char *profile_name;
    json_t *settings;

    gtk_tree_model_get(model, iter, CTK_APC_RULE_MODEL_COL_PROFILE_NAME, &profile_name, -1);

    profile = ctk_apc_profile_model_get_profile(ctk_app_profile->apc_profile_model,
                                                profile_name);
    settings = json_object_get(profile, "settings");

    settings_string = serialize_settings(settings, TRUE);

    g_object_set(cell, "markup", settings_string, NULL);

    free(settings_string);
    free(profile_name);
}

static void increase_rule_priority_callback(GtkWidget *widget, gpointer user_data)
{
    GtkTreeViewColumn *focus_column;
    GtkTreeIter iter;
    GtkTreePath *path;
    CtkAppProfile *ctk_app_profile = (CtkAppProfile *)user_data;
    GValue id = G_VALUE_INIT;

    // Get currently highlighted row
    gtk_tree_view_get_cursor(ctk_app_profile->main_rule_view,
                             &path, &focus_column);
    if (!path) {
        return;
    }

    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(ctk_app_profile->apc_rule_model),
                                 &iter, path)) {
        return;
    }

    gtk_tree_model_get_value(GTK_TREE_MODEL(ctk_app_profile->apc_rule_model),
                             &iter, CTK_APC_RULE_MODEL_COL_ID, &id);

    // Increment the row
    ctk_apc_rule_model_change_rule_priority(ctk_app_profile->apc_rule_model,
                                            g_value_get_int(&id),
                                            -1);

    ctk_config_statusbar_message(ctk_app_profile->ctk_config,
                                 "Priority of rule increased. %s",
                                 STATUSBAR_UPDATE_WARNING);

    gtk_tree_path_free(path);
    g_value_unset(&id);
}

static void decrease_rule_priority_callback(GtkWidget *widget, gpointer user_data)
{
    GtkTreeViewColumn *focus_column;
    GtkTreeIter iter;
    GtkTreePath *path;
    CtkAppProfile *ctk_app_profile = (CtkAppProfile *)user_data;
    GValue id = G_VALUE_INIT;

    // Get currently highlighted row
    gtk_tree_view_get_cursor(ctk_app_profile->main_rule_view,
                             &path, &focus_column);
    if (!path) {
        return;
    }

    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(ctk_app_profile->apc_rule_model),
                                 &iter, path)) {
        return;
    }
    gtk_tree_model_get_value(GTK_TREE_MODEL(ctk_app_profile->apc_rule_model),
                             &iter, CTK_APC_RULE_MODEL_COL_ID, &id);

    // Decrement the row
    ctk_apc_rule_model_change_rule_priority(ctk_app_profile->apc_rule_model,
                                            g_value_get_int(&id),
                                            1);

    ctk_config_statusbar_message(ctk_app_profile->ctk_config,
                                 "Priority of rule decreased. %s",
                                 STATUSBAR_UPDATE_WARNING);

    gtk_tree_path_free(path);
    g_value_unset(&id);
}

static void string_list_free_cb(gpointer data, gpointer user_data)
{
    free(data);
}

static void string_list_free_full(GList *list)
{
    g_list_foreach(list, string_list_free_cb, NULL);
    g_list_free(list);
}

static GList *get_source_filenames(CtkAppProfile *ctk_app_profile)
{
    size_t i, size;
    json_t *json_filename, *json_filenames;
    GList *filenames = NULL;

    json_filenames = nv_app_profile_config_get_source_filenames(ctk_app_profile->cur_config);

    for (i = 0, size = json_array_size(json_filenames); i < size; i++) {
        json_filename = json_array_get(json_filenames, i);
        filenames = g_list_prepend(filenames, strdup(json_string_value(json_filename)));
    }

    filenames = g_list_reverse(filenames);
    json_decref(json_filenames);

    return filenames;
}

static gboolean append_profile_name(GtkTreeModel *model,
                                    GtkTreePath *path,
                                    GtkTreeIter *iter,
                                    gpointer data)
{
    GList **profile_names = (GList **)data;
    gchar *profile_name;

    gtk_tree_model_get(model, iter, CTK_APC_PROFILE_MODEL_COL_NAME, &profile_name, -1);

    *profile_names = g_list_prepend(*profile_names, profile_name);

    return FALSE;
}

static GList *get_profile_names(CtkAppProfile *ctk_app_profile)
{
    GList *profile_names = NULL;

    gtk_tree_model_foreach(GTK_TREE_MODEL(ctk_app_profile->apc_profile_model),
                           append_profile_name,
                           (gpointer)&profile_names);

    profile_names = g_list_reverse(profile_names);
    return profile_names;
}

static gboolean unref_setting_object(GtkTreeModel *model,
                                     GtkTreePath *path,
                                     GtkTreeIter *iter,
                                     gpointer data)
{
    json_t *setting;

    gtk_tree_model_get(model, iter, SETTING_LIST_STORE_COL_SETTING, &setting, -1);

    json_decref(setting);

    return FALSE;
}

static void load_settings_from_profile(CtkAppProfile *ctk_app_profile,
                                       GtkListStore *list_store,
                                       const char *profile_name)
{
    GtkTreeIter iter;
    size_t i, size;
    const json_t *profile;
    const json_t *settings;
    json_t *setting;

    gtk_tree_model_foreach(GTK_TREE_MODEL(list_store), unref_setting_object, NULL);
    gtk_list_store_clear(list_store);

    profile = ctk_apc_profile_model_get_profile(ctk_app_profile->apc_profile_model,
                                                profile_name);
    if (!profile) {
        return;
    }

    settings = json_object_get(profile, "settings");
    if (!settings) {
        return;
    }
    for (i = 0, size = json_array_size(settings); i < size; i++) {
        setting = json_deep_copy(json_array_get(settings, i));
        gtk_list_store_append(list_store, &iter);
        gtk_list_store_set(list_store, &iter, SETTING_LIST_STORE_COL_SETTING, setting, -1);
    }
}

static void edit_rule_dialog_load_profile(EditRuleDialog *dialog,
                                          const char *profile_name)
{
    GList *strings;
    GtkCombo *combo;

    // profile name
    combo = GTK_COMBO(dialog->profile_name_combo);
    strings = get_profile_names(CTK_APP_PROFILE(dialog->parent));
    gtk_combo_set_popdown_strings(combo, strings);

    if (!profile_name) {
        if (g_list_length(strings)) {
            // Choose first string in the list
            g_string_assign(dialog->profile_name, (gchar *)strings->data);
        } else {
            g_string_assign(dialog->profile_name, "");
        }
    } else {
        g_string_assign(dialog->profile_name, profile_name);
    }

    gtk_entry_set_text(GTK_ENTRY(combo->entry), dialog->profile_name->str);

    string_list_free_full(strings);

    // profile settings
    load_settings_from_profile(CTK_APP_PROFILE(dialog->parent),
                               dialog->profile_settings_store,
                               dialog->profile_name->str);
}

static void edit_rule_dialog_load_values(EditRuleDialog *dialog)
{
    GList *strings;
    GtkCombo *combo;
    char *profile_name_copy;

    // window title
    gtk_window_set_title(GTK_WINDOW(dialog->top_window),
                         dialog->new_rule ? "Add new rule" : "Edit existing rule");

    // add/edit button
    button_set_label_and_stock_icon(GTK_BUTTON(dialog->add_edit_rule_button),
                                    "Update Rule",
                                    dialog->new_rule ? GTK_STOCK_ADD : GTK_STOCK_PREFERENCES);

    // source file
    combo = GTK_COMBO(dialog->source_file_combo);
    strings = get_source_filenames(CTK_APP_PROFILE(dialog->parent));

    gtk_combo_set_popdown_strings(combo, strings);

    if (dialog->new_rule) {
        if (g_list_length(strings)) {
            // Choose first string in the list
            g_string_assign(dialog->source_file, (gchar *)strings->data);
        } else {
            g_string_assign(dialog->source_file, "");
        }
    }

    gtk_entry_set_text(GTK_ENTRY(combo->entry), dialog->source_file->str);

    string_list_free_full(strings);

    // feature and matches
    ctk_drop_down_menu_set_current_value(CTK_DROP_DOWN_MENU(dialog->feature_menu),
                                         dialog->feature);
    gtk_entry_set_text(dialog->matches_entry, dialog->matches->str);


    // profile name and settings
    profile_name_copy = dialog->new_rule ? NULL : strdup(dialog->profile_name->str);
    edit_rule_dialog_load_profile(dialog, profile_name_copy);
    free(profile_name_copy);

}

static void edit_rule_dialog_show(EditRuleDialog *dialog)
{
    // Temporarily disable the "changed" signal to prevent races between
    // the update below and callbacks which fire when the window opens
    g_signal_handler_block(G_OBJECT(dialog->feature_menu),
                           dialog->feature_changed_signal);
    g_signal_handler_block(G_OBJECT(GTK_COMBO(dialog->profile_name_combo)->entry),
                           dialog->rule_profile_name_changed_signal);

    edit_rule_dialog_load_values(dialog);
    gtk_widget_show_all(dialog->top_window);
    g_signal_handler_unblock(G_OBJECT(dialog->feature_menu),
                             dialog->feature_changed_signal);
    g_signal_handler_unblock(G_OBJECT(GTK_COMBO(dialog->profile_name_combo)->entry),
                             dialog->rule_profile_name_changed_signal);

    // disable focusing to main window until this window closed
    gtk_window_set_transient_for(GTK_WINDOW(dialog->top_window),
                                 GTK_WINDOW(gtk_widget_get_toplevel(dialog->parent)));
    gtk_widget_set_sensitive(dialog->parent, FALSE);
}

static void add_rule_callback(GtkWidget *widget, gpointer user_data)
{
    CtkAppProfile *ctk_app_profile = (CtkAppProfile *)user_data;
    EditRuleDialog *dialog = ctk_app_profile->edit_rule_dialog;

    dialog->new_rule = TRUE;
    dialog->rule_id = -1;

    g_string_assign(dialog->source_file, "");
    dialog->feature = RULE_FEATURE_PROCNAME;
    g_string_assign(dialog->matches, "");
    g_string_assign(dialog->profile_name, "");

    edit_rule_dialog_show(dialog);

}

static gint parse_feature(const char *feature_str)
{
    size_t i;
    for (i = 0; i < ARRAY_LEN(rule_feature_identifiers); i++) {
        if (!strcmp(feature_str, rule_feature_identifiers[i])) {
            return i;
        }
    }
    return 0;
}

static void edit_rule_callbacks_common(CtkAppProfile *ctk_app_profile,
                                       GtkTreePath *path)
{
    EditRuleDialog *dialog = ctk_app_profile->edit_rule_dialog;
    gint id;
    gchar *feature, *matches, *profile_name, *filename;
    GtkTreeIter iter;

    if (!path) {
        return;
    }

    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(ctk_app_profile->apc_rule_model),
                                 &iter, path)) {
        return;
    }

    gtk_tree_model_get(GTK_TREE_MODEL(ctk_app_profile->apc_rule_model),
                       &iter,
                       CTK_APC_RULE_MODEL_COL_ID, &id,
                       CTK_APC_RULE_MODEL_COL_FEATURE, &feature,
                       CTK_APC_RULE_MODEL_COL_MATCHES, &matches,
                       CTK_APC_RULE_MODEL_COL_PROFILE_NAME, &profile_name,
                       CTK_APC_RULE_MODEL_COL_FILENAME, &filename,
                       -1);

    dialog->new_rule = FALSE;
    dialog->rule_id = id;
    g_string_assign(dialog->source_file, filename);
    dialog->feature = parse_feature(feature);
    g_string_assign(dialog->matches, matches);
    g_string_assign(dialog->profile_name, profile_name);

    edit_rule_dialog_show(dialog);


    free(filename);
    free(feature);
    free(matches);
    free(profile_name);
}

static void edit_rule_callback(GtkWidget *widget, gpointer user_data)
{
    GtkTreeViewColumn *focus_column;
    GtkTreePath *path;
    CtkAppProfile *ctk_app_profile = (CtkAppProfile *)user_data;

    // Get currently highlighted row
    gtk_tree_view_get_cursor(ctk_app_profile->main_rule_view,
                             &path, &focus_column);

    edit_rule_callbacks_common(ctk_app_profile, path);

    gtk_tree_path_free(path);
}

static void choose_next_row_in_list_view(GtkTreeView *tree_view,
                                         GtkTreeModel *tree_model,
                                         GtkTreePath **path)
{
    gint num_rows;
    gint depth, *indices;

    num_rows = gtk_tree_model_iter_n_children(tree_model, NULL);
    depth = gtk_tree_path_get_depth(*path);
    indices = gtk_tree_path_get_indices(*path);

    assert(depth == 1);
    (void)depth;

    if ((num_rows > 0) && (indices[0] == num_rows)) {
        // Choose the previous row instead of the current one
        gtk_tree_path_free(*path);
        *path = gtk_tree_path_new();
        gtk_tree_path_append_index(*path, num_rows - 1);
    }
}

static void delete_rule_callback_common(CtkAppProfile *ctk_app_profile)
{
    GtkTreeViewColumn *focus_column;
    GtkTreeIter iter;
    GtkTreePath *path;
    gint id;

    // Get currently highlighted row
    gtk_tree_view_get_cursor(ctk_app_profile->main_rule_view,
                             &path, &focus_column);
    if (!path) {
        return;
    }

    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(ctk_app_profile->apc_rule_model),
                                 &iter, path)) {
        return;
    }

    gtk_tree_model_get(GTK_TREE_MODEL(ctk_app_profile->apc_rule_model),
                       &iter, CTK_APC_RULE_MODEL_COL_ID, &id, -1);

    // Delete the row
    ctk_apc_rule_model_delete_rule(ctk_app_profile->apc_rule_model, id);

    // Select next rule in the list, if available
    choose_next_row_in_list_view(ctk_app_profile->main_rule_view,
                                 GTK_TREE_MODEL(ctk_app_profile->apc_rule_model),
                                 &path);
    gtk_tree_view_set_cursor(ctk_app_profile->main_rule_view,
                             path, NULL, FALSE);

    ctk_config_statusbar_message(ctk_app_profile->ctk_config,
                                 "Rule deleted. %s",
                                 STATUSBAR_UPDATE_WARNING);

    gtk_tree_path_free(path);
}

static void delete_rule_callback(GtkWidget *widget, gpointer user_data)
{
    CtkAppProfile *ctk_app_profile = (CtkAppProfile *)user_data;
    delete_rule_callback_common(ctk_app_profile);
}

static gboolean rules_tree_view_key_press_event(GtkWidget *widget,
                                                   GdkEvent *event,
                                                   gpointer user_data)
{
    CtkAppProfile *ctk_app_profile = (CtkAppProfile *)user_data;
    GdkEventKey *key_event;

    if (event->type == GDK_KEY_PRESS) {
        key_event = (GdkEventKey *)event;
        if (key_event->keyval == GDK_Delete) {
            delete_rule_callback_common(ctk_app_profile);
            return TRUE;
        }
    }

    // Use default handlers
    return FALSE;
}

static gboolean rule_browse_button_clicked(GtkWidget *widget, gpointer user_data)
{
    EditRuleDialog *dialog = (EditRuleDialog *)user_data;
    const gchar *filename = dialog->source_file->str;
    gint result;

    gtk_window_set_transient_for(GTK_WINDOW(dialog->file_sel),
                                 GTK_WINDOW(dialog->top_window));

    gtk_file_selection_set_filename(GTK_FILE_SELECTION(dialog->file_sel), filename);

    result = gtk_dialog_run(GTK_DIALOG(dialog->file_sel));

    switch (result) {
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
        filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(dialog->file_sel));
        gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(dialog->source_file_combo)->entry), filename);
    default:
        break;
    }

    gtk_widget_hide(dialog->file_sel);
    return FALSE;
}

static gboolean profile_browse_button_clicked(GtkWidget *widget, gpointer user_data)
{
    EditProfileDialog *dialog = (EditProfileDialog *)user_data;
    const gchar *filename = dialog->source_file->str;
    gint result;

    gtk_window_set_transient_for(GTK_WINDOW(dialog->file_sel),
                                 GTK_WINDOW(dialog->top_window));

    gtk_file_selection_set_filename(GTK_FILE_SELECTION(dialog->file_sel), filename);

    result = gtk_dialog_run(GTK_DIALOG(dialog->file_sel));

    switch (result) {
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
        filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(dialog->file_sel));
        gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(dialog->source_file_combo)->entry), filename);
    default:
        break;
    }

    gtk_widget_hide(dialog->file_sel);
    return FALSE;
}

static const char __rule_pattern_help[] =
    "In this section, you write the pattern that will be used to determine whether "
    "the settings in this rule will apply to a given application.";

static const char __rule_pattern_extended_help[] =
    "A pattern is comprised of two parts: a feature of the "
    "process which will be retrieved by the driver at runtime, and a string against "
    "which the driver will compare the feature and determine if there is a match. "
    "If the pattern matches, then the settings determined by the rule's associated "
    "profile will be applied to the process, assuming they don't conflict with "
    "settings determined by other matching rules with higher priority.\n\n"
    "See the \"Supported Features\" help section for a list of supported features.";

static const char __rule_profile_help[] =
    "In this section, you choose the profile that will be applied if the rule's pattern "
    "matches a given process.";

static const char __rule_profile_extended_help[] =
    "This section contains a drop-down box for choosing a profile name, and convenience "
    "buttons for modifying an existing profile or creating a new profile to be used by "
    "the rule. This section also has a table which lets you preview the settings that "
    "will be applied by the given profile. The table is read-only: to modify individual "
    "settings, click the \"Edit Profile\" button.";

static void config_create_source_file_entry(CtkConfig *ctk_config,
                                            GtkWidget **pcontainer,
                                            GtkWidget **psource_file_combo,
                                            GList **help_data_list,
                                            const char *name,
                                            GCallback browse_button_clicked_callback,
                                            gpointer user_data)
{
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *combo_box;
    GtkWidget *browse_button;

    GString *help_string;

    help_string = g_string_new("");

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_set_spacing(GTK_BOX(hbox), 4);

    label = gtk_label_new("Source File");

    g_string_printf(help_string, "You can specify the source file where the %s is defined in this drop-down box.", name);

    ctk_config_set_tooltip_and_add_help_data(ctk_config,
                                         label,
                                         help_data_list,
                                         "Source File",
                                         help_string->str,
                                         NULL);

    combo_box = gtk_combo_new();
    browse_button = gtk_button_new();

    button_set_label_and_stock_icon(GTK_BUTTON(browse_button),
                                    "Browse...", GTK_STOCK_OPEN);

    g_string_printf(help_string, "Clicking this button opens a file selection dialog box which allows you to choose an "
                                 "appropriate configuration file for the %s.", name);

    ctk_config_set_tooltip_and_add_help_data(ctk_config,
                                         browse_button,
                                         help_data_list,
                                         "Browse...",
                                         help_string->str,
                                         NULL);

    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), combo_box, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), browse_button, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(browse_button), "clicked",
                     browse_button_clicked_callback,
                     user_data);

    *pcontainer = hbox;
    *psource_file_combo = combo_box;

    g_string_free(help_string, TRUE);
}


static void feature_changed(GtkWidget *widget, gpointer user_data)
{
    EditRuleDialog *dialog = (EditRuleDialog *)user_data;
    dialog->feature =
        ctk_drop_down_menu_get_current_value(CTK_DROP_DOWN_MENU(dialog->feature_menu));
}

static GtkWidget *create_feature_menu(EditRuleDialog *dialog)
{
    size_t i;

    dialog->feature_menu = ctk_drop_down_menu_new(CTK_DROP_DOWN_MENU_FLAG_COMBO);

    for (i = 0; i < NUM_RULE_FEATURES; i++) {
        ctk_drop_down_menu_append_item(CTK_DROP_DOWN_MENU(dialog->feature_menu),
                                       rule_feature_label_strings[i], i);
    }

    dialog->feature_changed_signal =
        g_signal_connect(G_OBJECT(dialog->feature_menu),
                         "changed", G_CALLBACK(feature_changed), (gpointer)dialog);

    return dialog->feature_menu;
}

static void rule_profile_name_changed(GtkWidget *widget, gpointer user_data)
{
    const char *profile_name;
    EditRuleDialog *dialog;
    dialog = (EditRuleDialog *)user_data;
    profile_name = gtk_entry_get_text(GTK_ENTRY(widget));
    g_string_assign(dialog->profile_name, profile_name);

    load_settings_from_profile(CTK_APP_PROFILE(dialog->parent),
                               dialog->profile_settings_store,
                               profile_name);
}

typedef struct FindPathOfProfileParamsRec {
    const char *profile_name; // in
    GtkTreePath *path;  // out
} FindPathOfProfileParams;

/*
 * find_path_of_profile() is a callback to gtk_tree_model_foreach which looks
 * through the CtkApcProfileModel for a profile with the given profile name.
 * For this to work properly, find_path_of_profile_params should be initialized
 * as follows before calling gtk_tree_model_foreach():
 *   profile_name: the name of the profile
 *   path: NULL
 */
static gboolean find_path_of_profile(GtkTreeModel *model,
                                     GtkTreePath *path,
                                     GtkTreeIter *iter,
                                     gpointer data)
{
    char *profile_name;
    FindPathOfProfileParams *find_path_of_profile_params =
        (FindPathOfProfileParams *)data;

    gtk_tree_model_get(model, iter, CTK_APC_PROFILE_MODEL_COL_NAME, &profile_name, -1);

    if (!strcmp(profile_name, find_path_of_profile_params->profile_name)) {
        find_path_of_profile_params->path =
            gtk_tree_path_copy(path);
        return TRUE; // Done
    } else {
        return FALSE; // Keep going
    }
}

static void edit_profile_callbacks_common(CtkAppProfile *ctk_app_profile,
                                          GtkTreePath *path,
                                          GtkWidget *caller);

static void edit_profile_dialog_show(EditProfileDialog *dialog);

static gboolean rule_profile_entry_edit_profile_button_clicked(GtkWidget *widget,
                                                               gpointer user_data)
{
    EditRuleDialog *rule_dialog = (EditRuleDialog *)user_data;
    CtkAppProfile *ctk_app_profile = CTK_APP_PROFILE(rule_dialog->parent);
    FindPathOfProfileParams find_path_of_profile_params;

    memset(&find_path_of_profile_params, 0, sizeof(FindPathOfProfileParams));

    find_path_of_profile_params.profile_name = rule_dialog->profile_name->str;
    find_path_of_profile_params.path = NULL;

    gtk_tree_model_foreach(GTK_TREE_MODEL(ctk_app_profile->apc_profile_model),
                           find_path_of_profile,
                           &find_path_of_profile_params);

    edit_profile_callbacks_common(ctk_app_profile,
                                  find_path_of_profile_params.path,
                                  rule_dialog->top_window);

    gtk_tree_path_free(find_path_of_profile_params.path);

    return FALSE;
}

static void add_profile_callbacks_common(CtkAppProfile *ctk_app_profile,
                                         GtkWidget *caller);

static gboolean rule_profile_entry_new_profile_button_clicked(GtkWidget *widget,
                                                              gpointer user_data)
{
    EditRuleDialog *rule_dialog = (EditRuleDialog *)user_data;
    CtkAppProfile *ctk_app_profile = CTK_APP_PROFILE(rule_dialog->parent);

    add_profile_callbacks_common(ctk_app_profile, rule_dialog->top_window);

    return FALSE;
}

static GtkWidget *create_rule_profile_name_entry(EditRuleDialog *dialog)
{
    GtkWidget *hbox;
    GtkWidget *button;
    GtkWidget *label;
    GtkWidget *combo_box;

    hbox = gtk_hbox_new(FALSE, 8);

    label = gtk_label_new("Profile Name");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    dialog->profile_name_combo = combo_box = gtk_combo_new();

    gtk_box_pack_start(GTK_BOX(hbox), combo_box, TRUE, TRUE, 0);

    dialog->rule_profile_name_changed_signal =
        g_signal_connect(G_OBJECT(GTK_COMBO(combo_box)->entry), "changed",
                         G_CALLBACK(rule_profile_name_changed),
                         (gpointer)dialog);

    button = gtk_button_new_with_label("Edit Profile");
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(rule_profile_entry_edit_profile_button_clicked),
                     (gpointer)dialog);

    button = gtk_button_new_with_label("New Profile");
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(rule_profile_entry_new_profile_button_clicked),
                     (gpointer)dialog);

    return hbox;
}

static void setting_key_renderer_func(GtkTreeViewColumn *tree_column,
                                      GtkCellRenderer   *cell,
                                      GtkTreeModel      *model,
                                      GtkTreeIter       *iter,
                                      gpointer           data)
{
    const char *key;
    json_t *setting;
    gtk_tree_model_get(model, iter, SETTING_LIST_STORE_COL_SETTING, &setting, -1);
    key = json_string_value(json_object_get(setting, "key"));
    g_object_set(cell, "text", key, NULL);
}

static void setting_type_renderer_func(GtkTreeViewColumn *tree_column,
                                       GtkCellRenderer   *cell,
                                       GtkTreeModel      *model,
                                       GtkTreeIter       *iter,
                                       gpointer           data)
{
    const char *type = NULL;
    json_t *setting, *value;
    gtk_tree_model_get(model, iter, SETTING_LIST_STORE_COL_SETTING, &setting, -1);
    value = json_object_get(setting, "value");

    switch(json_typeof(value)) {
        case JSON_STRING:
            type = "string";
            break;
        case JSON_INTEGER:
            type = "int";
            break;
        case JSON_REAL:
            type = "float";
            break;
        case JSON_TRUE:
        case JSON_FALSE:
            type = "bool";
            break;
        default:
            assert(0);
    }

    g_object_set(cell, "text", type, NULL);
}

static void setting_value_renderer_func(GtkTreeViewColumn *tree_column,
                                        GtkCellRenderer   *cell,
                                        GtkTreeModel      *model,
                                        GtkTreeIter       *iter,
                                        gpointer           data)
{
    json_t *setting;
    char *value;
    gtk_tree_model_get(model, iter, SETTING_LIST_STORE_COL_SETTING, &setting, -1);
    setting_get_key_value(setting, NULL, &value, TRUE);
    g_object_set(cell, "markup", value, NULL);
    free(value);
}

static gboolean run_error_dialog(GtkWindow *window,
                                 GString *fatal_errors,
                                 GString *nonfatal_errors,
                                 const char *op_string)
{
    GString *error_string;
    GtkWidget *error_dialog;
    gboolean success;
    gint result;

    if (!fatal_errors->len && !nonfatal_errors->len) {
        return TRUE;
    }

    error_string = g_string_new("");
    if (fatal_errors->len) {
        g_string_append_printf(error_string,
                               "nvidia-settings encountered the following configuration errors:\n\n"
                               "%s\n",
                               fatal_errors->str);
    }
    if (nonfatal_errors->len) {
        g_string_append_printf(error_string,
                               "%snvidia-settings encountered the following configuration issues:\n\n"
                               "%s\n",
                               fatal_errors->len ? "Also, " : "", nonfatal_errors->str);
    }

    if (fatal_errors->len) {
        g_string_append_printf(error_string,
                               "Please fix the configuration errors before attempting to %s.\n",
                               op_string);
    } else {
        g_string_append_printf(error_string,
                               "Continue to %s anyway?\n", op_string);
    }

    error_dialog = gtk_message_dialog_new(window,
                                          GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                          fatal_errors->len ? GTK_MESSAGE_ERROR : GTK_MESSAGE_QUESTION,
                                          fatal_errors->len ? GTK_BUTTONS_CLOSE : GTK_BUTTONS_YES_NO,
                                          "%s", error_string->str);
    result = gtk_dialog_run(GTK_DIALOG(error_dialog));

    if (!fatal_errors->len) {
        success = (result == GTK_RESPONSE_YES);
    } else {
        success = FALSE;
    }

    gtk_widget_destroy(error_dialog);

    return success;
}


static inline gboolean check_valid_source_file(CtkAppProfile *ctk_app_profile,
                                               const char *source_file_str,
                                               char **reason)
{
    return !!nv_app_profile_config_check_valid_source_file(ctk_app_profile->cur_config,
                                                           source_file_str,
                                                           reason);
}

// Check for inconsistencies and errors in the rule dialog box settings,
// and warn the user if any are detected
static gboolean edit_rule_dialog_validate(EditRuleDialog *dialog)
{
    CtkAppProfile *ctk_app_profile = CTK_APP_PROFILE(dialog->parent);
    GString *fatal_errors;
    GString *nonfatal_errors;
    gboolean success;
    char *reason;

    fatal_errors = g_string_new("");
    nonfatal_errors = g_string_new("");

    if (!check_valid_source_file(ctk_app_profile, dialog->source_file->str, &reason)) {
        g_string_append_printf(fatal_errors, "%s\tThe source filename \"%s\" is not valid in this configuration "
                                             "because %s\n", get_bullet(), dialog->source_file->str, reason);
        free(reason);
    }

    if (!ctk_apc_profile_model_get_profile(ctk_app_profile->apc_profile_model, dialog->profile_name->str)) {
        g_string_append_printf(nonfatal_errors, "%s\tThe profile \"%s\" referenced by this rule does not exist.\n",
                               get_bullet(), dialog->profile_name->str);
    }

    success = run_error_dialog(GTK_WINDOW(dialog->top_window),
                               fatal_errors,
                               nonfatal_errors,
                               "save this rule");

    g_string_free(fatal_errors, TRUE);
    g_string_free(nonfatal_errors, TRUE);

    return success;
}

static void edit_rule_dialog_save_changes(GtkWidget *widget, gpointer user_data)
{
    EditRuleDialog *dialog = (EditRuleDialog *)user_data;
    CtkAppProfile *ctk_app_profile = CTK_APP_PROFILE(dialog->parent);
    GtkWidget *source_file_entry = GTK_COMBO(dialog->source_file_combo)->entry;
    json_t *rule_json = json_object();
    json_t *pattern_json = json_object();

    // Get the latest values from our widgets
    g_string_assign(dialog->matches, gtk_entry_get_text(GTK_ENTRY(dialog->matches_entry)));
    g_string_assign(dialog->source_file, gtk_entry_get_text(GTK_ENTRY(source_file_entry)));

    // Check for inconsistencies and errors
    if (!edit_rule_dialog_validate(dialog)) {
        return;
    }

    // Construct the update object
    json_object_set_new(pattern_json, "feature", json_string(rule_feature_identifiers[dialog->feature]));
    json_object_set_new(pattern_json, "matches", json_string(dialog->matches->str));
    json_object_set_new(rule_json, "profile", json_string(dialog->profile_name->str));
    json_object_set_new(rule_json, "pattern", pattern_json);

    // Update the rule in the configuration
    if (dialog->new_rule) {
        ctk_apc_rule_model_create_rule(ctk_app_profile->apc_rule_model,
                                       dialog->source_file->str, rule_json);

    } else {
        ctk_apc_rule_model_update_rule(ctk_app_profile->apc_rule_model,
                                       dialog->source_file->str,
                                       dialog->rule_id,
                                       rule_json);
    }

    json_decref(rule_json);

    // Close the window, and re-sensitize the parent
    gtk_widget_set_sensitive(dialog->parent, TRUE);
    gtk_widget_hide(dialog->top_window);

    ctk_config_statusbar_message(ctk_app_profile->ctk_config,
                                 "Rule updated. %s",
                                 STATUSBAR_UPDATE_WARNING);
}

static void edit_rule_dialog_cancel(GtkWidget *widget, gpointer user_data)
{
    EditRuleDialog *dialog = (EditRuleDialog *)user_data;
    // Close the window, and re-sensitize the parent
    gtk_widget_set_sensitive(dialog->parent, TRUE);
    gtk_widget_hide(dialog->top_window);
}

static ToolbarItemTemplate *get_edit_rule_dialog_toolbar_items(EditRuleDialog *dialog,
                                                               size_t *num_items)
{
    ToolbarItemTemplate *items_copy;
    const ToolbarItemTemplate items[] = {
        {
            .text = UPDATE_RULE_LABEL,
            .help_text = "The Update Rule button allows you to save changes made to the rule definition.",
            .icon_id = GTK_STOCK_SAVE,
            .callback = G_CALLBACK(edit_rule_dialog_save_changes),
            .user_data = dialog,
        },
        {
            .text = "Cancel",
            .help_text = "The Cancel button allows you to discard any changes made to the rule definition.",
            .icon_id = GTK_STOCK_CANCEL,
            .callback = G_CALLBACK(edit_rule_dialog_cancel),
            .user_data = dialog,
        }
    };

    items_copy = malloc(sizeof(items));
    memcpy(items_copy, items, sizeof(items));

    *num_items = ARRAY_LEN(items);
    return items_copy;
}

static gboolean edit_rule_dialog_handle_delete(GtkWidget *widget,
                                               GdkEvent *event,
                                               gpointer user_data)
{
    EditRuleDialog *dialog = (EditRuleDialog *)user_data;
    gtk_widget_set_sensitive(dialog->parent, TRUE);
    gtk_widget_hide(widget);

    return TRUE;
}

static EditRuleDialog* edit_rule_dialog_new(CtkAppProfile *ctk_app_profile)
{
    ToolbarItemTemplate *edit_rule_dialog_toolbar_items;
    size_t num_edit_rule_dialog_toolbar_items;
    EditRuleDialog *dialog;
    GtkWidget *label;
    GtkWidget *container;
    GtkWidget *main_vbox, *vbox;
    GtkWidget *feature_menu, *profile_name_entry;
    GtkWidget *table;
    GtkWidget *frame;
    GtkWidget *entry;
    GtkWidget *tree_view;
    GtkWidget *toolbar;
    GtkWidget *alignment;
    GtkWidget *scroll_win;
    GList *toolbar_help_items;
    GList *toolbar_widget_items;

    const TreeViewColumnTemplate settings_tree_view_columns[] = {
        {
            .title = "Key",
            .renderer_func = setting_key_renderer_func,
            .func_data = NULL,
            .min_width = 200,
            .help_text = "Each entry in the \"Key\" column describes a key for a setting."
        },
        {
            .title = "Type",
            .renderer_func = setting_type_renderer_func,
            .min_width = 100,
            .func_data = NULL,
            .help_text = "Each entry in the \"Type\" column describes the underlying JSON type for "
                         "a setting value."
        },
        {
            .title = "Value",
            .renderer_func = setting_value_renderer_func,
            .func_data = NULL,
            .help_text = "Each entry in the \"Value\" column describes the value of a setting."
        }
    };


    dialog = malloc(sizeof(EditRuleDialog));
    if (!dialog) {
        return NULL;
    }

    dialog->help_data = NULL;

    dialog->parent = GTK_WIDGET(ctk_app_profile);
    dialog->top_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    gtk_window_set_modal(GTK_WINDOW(dialog->top_window), TRUE);

    g_signal_connect(G_OBJECT(dialog->top_window), "delete-event",
                     G_CALLBACK(edit_rule_dialog_handle_delete), dialog);

    dialog->source_file = g_string_new("");
    dialog->feature = RULE_FEATURE_PROCNAME;
    dialog->matches = g_string_new("");
    dialog->profile_name = g_string_new("");
    dialog->profile_settings_store = gtk_list_store_new(SETTING_LIST_STORE_NUM_COLS, G_TYPE_POINTER);
    dialog->file_sel = gtk_file_selection_new("Please select a source file for the rule");

    gtk_widget_set_size_request(dialog->top_window, 500, 480);
    gtk_container_set_border_width(GTK_CONTAINER(dialog->top_window), 8);

    main_vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_set_spacing(GTK_BOX(main_vbox), 8);

    gtk_container_add(GTK_CONTAINER(dialog->top_window), main_vbox);

    config_create_source_file_entry(ctk_app_profile->ctk_config,
                                    &container,
                                    &dialog->source_file_combo,
                                    &dialog->help_data,
                                    "rule",
                                    G_CALLBACK(rule_browse_button_clicked),
                                    (gpointer)dialog);

    gtk_box_pack_start(GTK_BOX(main_vbox), container, FALSE, FALSE, 0);

    frame = gtk_frame_new("Rule Pattern");
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);

    label = gtk_frame_get_label_widget(GTK_FRAME(frame));

    ctk_config_set_tooltip_and_add_help_data(ctk_app_profile->ctk_config,
                                         label,
                                         &dialog->help_data,
                                         "Rule Pattern",
                                         __rule_pattern_help,
                                         __rule_pattern_extended_help);

    // Add widgets to the "Rule Pattern" section
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);

    label = gtk_label_new("The following profile will be used if...");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    table = gtk_table_new(2, 2, FALSE);

    label = gtk_label_new("This feature:");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);

    feature_menu = create_feature_menu(dialog);
    gtk_table_attach_defaults(GTK_TABLE(table), feature_menu, 1, 2, 0, 1);

    label = gtk_label_new("Matches this string:");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);

    entry = gtk_entry_new();
    dialog->matches_entry = GTK_ENTRY(entry);
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, 1, 2);

    gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(frame), vbox);

    gtk_box_pack_start(GTK_BOX(main_vbox), frame, FALSE, FALSE, 0);

    frame = gtk_frame_new("Rule Profile");
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);

    label = gtk_frame_get_label_widget(GTK_FRAME(frame));

    ctk_config_set_tooltip_and_add_help_data(ctk_app_profile->ctk_config,
                                         label,
                                         &dialog->help_data,
                                         "Rule Profile",
                                         __rule_profile_help,
                                         __rule_profile_extended_help);

    vbox = gtk_vbox_new(FALSE, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);

    profile_name_entry = create_rule_profile_name_entry(dialog);
    gtk_box_pack_start(GTK_BOX(vbox), profile_name_entry, FALSE, FALSE, 0);

    label = gtk_label_new("This profile will apply the following settings...");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(dialog->profile_settings_store));
    populate_tree_view(GTK_TREE_VIEW(tree_view),
                       settings_tree_view_columns,
                       ctk_app_profile,
                       ARRAY_LEN(settings_tree_view_columns),
                       NULL);

    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tree_view), TRUE);

    scroll_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll_win), tree_view);
    gtk_box_pack_start(GTK_BOX(vbox), scroll_win, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(frame), vbox);

    gtk_box_pack_start(GTK_BOX(main_vbox), frame, TRUE, TRUE, 0);

    alignment = gtk_alignment_new(1.0, 0.5, 0.0, 0.0);
    toolbar = gtk_toolbar_new();

    dialog->help_data = g_list_reverse(dialog->help_data);

    edit_rule_dialog_toolbar_items = get_edit_rule_dialog_toolbar_items(dialog, &num_edit_rule_dialog_toolbar_items);
    populate_toolbar(GTK_TOOLBAR(toolbar),
                     edit_rule_dialog_toolbar_items,
                     num_edit_rule_dialog_toolbar_items,
                     &toolbar_help_items,
                     &toolbar_widget_items);

    dialog->help_data = g_list_concat(dialog->help_data, toolbar_help_items);

    // Save off the "Update Rule" button for later use
    dialog->add_edit_rule_button = find_widget_in_widget_data_list(toolbar_widget_items, UPDATE_RULE_LABEL);

    free(edit_rule_dialog_toolbar_items);

    gtk_container_add(GTK_CONTAINER(alignment), toolbar);
    gtk_box_pack_start(GTK_BOX(main_vbox), alignment, FALSE, FALSE, 0);

    return dialog;
}

static void edit_rule_dialog_destroy(EditRuleDialog *dialog)
{
    g_string_free(dialog->source_file, TRUE);
    g_string_free(dialog->matches, TRUE);
    g_string_free(dialog->profile_name, TRUE);
    ctk_help_data_list_free_full(dialog->help_data);
    free(dialog);
}

static void edit_profile_dialog_settings_new_row(GtkTreeView *tree_view,
                                                 GtkTreeModel *tree_model,
                                                 GtkTreePath **path,
                                                 GtkTreeViewColumn **column)
{
    GtkTreeIter iter;
    json_t *setting = json_object();
    json_object_set(setting, "key", json_string(""));
    json_object_set(setting, "value", json_false());
    gtk_list_store_append(GTK_LIST_STORE(tree_model), &iter);
    gtk_list_store_set(GTK_LIST_STORE(tree_model), &iter, SETTING_LIST_STORE_COL_SETTING, setting, -1);

    *path = gtk_tree_model_get_path(tree_model, &iter);
    *column = gtk_tree_view_get_column(tree_view, 0);
}

static void edit_profile_dialog_add_setting(GtkWidget *widget, gpointer user_data)
{
    EditProfileDialog *dialog = (EditProfileDialog *)user_data;
    GtkTreePath *path;
    GtkTreeViewColumn *column;

    edit_profile_dialog_settings_new_row(dialog->settings_view,
                                    GTK_TREE_MODEL(dialog->settings_store),
                                    &path,
                                    &column);

    gtk_widget_grab_focus(GTK_WIDGET(dialog->settings_view));
    gtk_tree_view_set_cursor(dialog->settings_view, path, column, TRUE);

    gtk_tree_path_free(path);
}

static void edit_profile_dialog_delete_setting_common(EditProfileDialog *dialog)
{
    GtkTreeIter iter;
    GtkTreePath *path;
    GtkTreeViewColumn *focus_column;

    // Set the focus to NULL to terminate any editing currently taking place
    // XXX: Since this row is about to be deleted, set the
    // setting_update_canceled flag to ensure that the model isn't saving
    // anything to this row and displaying bogus warnings to the user.
    dialog->setting_update_canceled = TRUE;
    gtk_window_set_focus(GTK_WINDOW(dialog->top_window), NULL);
    dialog->setting_update_canceled = FALSE;

    // Get currently highlighted row
    gtk_tree_view_get_cursor(dialog->settings_view, &path, &focus_column);

    if (!path) {
        return;
    }

    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(dialog->settings_store), &iter, path)) {
        return;
    }

    // Delete the row
    gtk_list_store_remove(dialog->settings_store, &iter);

    // Select next setting in the list, if available
    choose_next_row_in_list_view(dialog->settings_view,
                                 GTK_TREE_MODEL(dialog->settings_store),
                                 &path);
    gtk_tree_view_set_cursor(dialog->settings_view, path, NULL, FALSE);

    gtk_tree_path_free(path);
}

static void edit_profile_dialog_delete_setting(GtkWidget *widget, gpointer user_data)
{
    EditProfileDialog *dialog = (EditProfileDialog *)user_data;
    edit_profile_dialog_delete_setting_common(dialog);
}

static void edit_profile_dialog_edit_setting(GtkWidget *widget, gpointer user_data)
{
    EditProfileDialog *dialog = (EditProfileDialog *)user_data;
    GtkTreePath *path;
    GtkTreeViewColumn *first_column;

    // Get currently highlighted row
    gtk_tree_view_get_cursor(dialog->settings_view, &path, NULL);
    if (!path) {
        return;
    }

    first_column = gtk_tree_view_get_column(dialog->settings_view, 0);
    assert(first_column);

    gtk_widget_grab_focus(GTK_WIDGET(dialog->settings_view));
    gtk_tree_view_set_cursor(dialog->settings_view, path, first_column, TRUE);

    gtk_tree_path_free(path);
}

static gboolean append_setting(GtkTreeModel *model,
                               GtkTreePath *path,
                               GtkTreeIter *iter,
                               gpointer data)
{
    json_t *settings = (json_t *)data;
    json_t *setting;

    gtk_tree_model_get(model, iter, SETTING_LIST_STORE_COL_SETTING, &setting, -1);
    json_array_append(settings, setting);

    return FALSE;
}

static void edit_profile_dialog_update_settings(EditProfileDialog *dialog)
{
    json_array_clear(dialog->settings);
    gtk_tree_model_foreach(GTK_TREE_MODEL(dialog->settings_store),
                           append_setting,
                           (gpointer)dialog->settings);
}

static gboolean widget_get_visible(GtkWidget *widget)
{
    gboolean visible;

    g_object_get(G_OBJECT(widget),
                 "visible", &visible, NULL);

    return visible;
}

static const gchar *get_canonical_setting_key(const gchar *key)
{
    size_t i;
    for (i = 0; i < NUM_PROFILE_SETTINGS; i++) {
        if (!strcasecmp(key, profile_setting_keys[i])) {
            return profile_setting_keys[i];
        }
    }
    return NULL;
}

static gboolean check_unrecognized_setting_keys(const json_t *settings)
{
    const json_t *setting;
    const char *setting_key;
    size_t i, size;
    for (i = 0, size = json_array_size(settings); i < size; i++) {
        setting = json_array_get(settings, i);
        setting_key = json_string_value(json_object_get(setting, "key"));
        if (!get_canonical_setting_key(setting_key)) {
            return TRUE;
        }
    }

    return FALSE;
}

// Check for inconsistencies and errors in the profile dialog box settings,
// and warn the user if any are detected
static gboolean edit_profile_dialog_validate(EditProfileDialog *dialog)
{
    CtkAppProfile *ctk_app_profile = CTK_APP_PROFILE(dialog->parent);
    GString *fatal_errors;
    GString *nonfatal_errors;
    gboolean success;
    char *reason;

    fatal_errors = g_string_new("");
    nonfatal_errors = g_string_new("");

    if (!strcmp(dialog->name->str, "")) {
        g_string_append_printf(nonfatal_errors, "%s\tThe profile name is empty.\n", get_bullet());
    }

    if ((dialog->new_profile || strcmp(dialog->name->str, dialog->orig_name->str)) &&
        ctk_apc_profile_model_get_profile(ctk_app_profile->apc_profile_model,
                                          dialog->name->str)) {
        if (dialog->new_profile) {
            g_string_append_printf(nonfatal_errors,
                                   "%s\tA profile with the name \"%s\" already exists and will be "
                                   "overwritten.\n",
                                   get_bullet(), dialog->name->str);
        } else {
            g_string_append_printf(nonfatal_errors,
                                   "%s\tRenaming this profile from \"%s\" to \"%s\" will "
                                   "overwrite an existing profile.\n", get_bullet(),
                                   dialog->orig_name->str,
                                   dialog->name->str);
        }
    }

    if (!check_valid_source_file(ctk_app_profile, dialog->source_file->str, &reason)) {
        g_string_append_printf(fatal_errors, "%s\tThe source filename \"%s\" is not valid in this configuration "
                                             "because %s\n", get_bullet(), dialog->source_file->str, reason);
        free(reason);
    }

    if (check_unrecognized_setting_keys(dialog->settings)) {
        g_string_append_printf(nonfatal_errors, "%s\tThis profile has settings with keys that may not be recognized "
                                                "by the NVIDIA graphics driver. Consult the on-line help for a list "
                                                "of valid keys.\n", get_bullet());
    }

    success = run_error_dialog(GTK_WINDOW(dialog->top_window),
                               fatal_errors,
                               nonfatal_errors,
                               "save this profile");

    g_string_free(fatal_errors, TRUE);
    g_string_free(nonfatal_errors, TRUE);

    return success;
}

static void edit_profile_dialog_save_changes(GtkWidget *widget, gpointer user_data)
{
    EditProfileDialog *profile_dialog = (EditProfileDialog *)user_data;
    EditRuleDialog *rule_dialog;
    CtkAppProfile *ctk_app_profile = CTK_APP_PROFILE(profile_dialog->parent);
    GtkWidget *source_file_entry = GTK_COMBO(profile_dialog->source_file_combo)->entry;
    json_t *profile_json = json_object();
    GtkCombo *combo;
    GList *source_filenames;
    gboolean rules_fixed_up = FALSE;

    rule_dialog = ctk_app_profile->edit_rule_dialog;

    // Set the focus to NULL to terminate any editing currently taking place
    gtk_window_set_focus(GTK_WINDOW(profile_dialog->top_window), NULL);

    // Get the latest values from our widgets
    g_string_assign(profile_dialog->name, gtk_entry_get_text(GTK_ENTRY(profile_dialog->name_entry)));
    g_string_assign(profile_dialog->source_file, gtk_entry_get_text(GTK_ENTRY(source_file_entry)));
    edit_profile_dialog_update_settings(profile_dialog);

    // TODO delete any unset settings (nil key and value)?

    // Check for inconsistencies and errors
    if (!edit_profile_dialog_validate(profile_dialog)) {
        return;
    }

    // Construct the update object, using a deep copy of the settings array.
    json_object_set_new(profile_json, "settings",
                        json_deep_copy(profile_dialog->settings));

    // If this is an edit and the profile name changed, delete the old profile
    if (!profile_dialog->new_profile &&
        strcmp(profile_dialog->name->str, profile_dialog->orig_name->str)) {
        ctk_apc_profile_model_delete_profile(ctk_app_profile->apc_profile_model,
                                             profile_dialog->orig_name->str);
        if (ctk_app_profile->ctk_config->conf->booleans &
            CONFIG_PROPERTIES_UPDATE_RULES_ON_PROFILE_NAME_CHANGE) {
            rules_fixed_up =
                nv_app_profile_config_profile_name_change_fixup(
                    ctk_app_profile->cur_config,
                    profile_dialog->orig_name->str,
                    profile_dialog->name->str);
        }
    }


    // Update the profile in the configuration
    ctk_apc_profile_model_update_profile(ctk_app_profile->apc_profile_model,
                                         profile_dialog->source_file->str,
                                         profile_dialog->name->str,
                                         profile_json);

    // Refresh the view in the rule, if necessary
    if (widget_get_visible(rule_dialog->top_window)) {
        // XXX could this be abstracted?
        edit_rule_dialog_load_profile(rule_dialog, profile_dialog->name->str);
        source_filenames = get_source_filenames(ctk_app_profile);
        combo = GTK_COMBO(rule_dialog->source_file_combo);
        gtk_combo_set_popdown_strings(combo, source_filenames);
        gtk_entry_set_text(GTK_ENTRY(combo->entry), rule_dialog->source_file->str);
        string_list_free_full(source_filenames);
    }

    json_decref(profile_json);

    ctk_config_statusbar_message(ctk_app_profile->ctk_config,
                                 "Profile \"%s\" updated. %s%s",
                                 profile_dialog->name->str,
                                 rules_fixed_up ?
                                 "Some rules have been updated to refer "
                                 "to the new profile name. " : "",
                                 STATUSBAR_UPDATE_WARNING);

    // Close the window, and re-sensitize the caller
    gtk_widget_set_sensitive(profile_dialog->caller, TRUE);
    gtk_widget_hide(profile_dialog->top_window);
}

static void edit_profile_dialog_cancel(GtkWidget *widget, gpointer user_data)
{
    EditProfileDialog *dialog = (EditProfileDialog *)user_data;

    // Close the window, and re-sensitize the caller
    gtk_widget_set_sensitive(dialog->caller, TRUE);
    gtk_widget_hide(dialog->top_window);
}

static void get_profile_dialog_toolbar_items(EditProfileDialog *dialog,
                                             ToolbarItemTemplate **settings_items_copy,
                                             size_t *num_settings_items,
                                             ToolbarItemTemplate **dialog_items_copy,
                                             size_t *num_dialog_items)
{
    const ToolbarItemTemplate settings_items[] = {
        {
            .text = "Add Setting",
            .help_text = "The Add Setting button allows you to create a new setting in the profile.",
            .icon_id = GTK_STOCK_ADD,
            .callback = G_CALLBACK(edit_profile_dialog_add_setting),
            .user_data = dialog,
        },
        {
            .text = "Delete Setting",
            .help_text = "The Delete Setting button allows you to delete a highlighted setting from the profile.",
            .extended_help_text = "A setting can also be deleted from the profile by highlighting it in the list "
                                  "and hitting the Delete key.",
            .icon_id = GTK_STOCK_REMOVE,
            .callback = G_CALLBACK(edit_profile_dialog_delete_setting),
            .user_data = dialog,
        },
        {
            .text = "Edit Setting",
            .help_text = "The Edit Setting button allows you to edit a highlighted setting in the profile.",
            .extended_help_text = "This will activate an entry box in the setting's key column. To modify the setting's "
                                  "value, hit the Tab key or Right Arrow key, or double-click on the value.",
            .icon_id = GTK_STOCK_PREFERENCES,
            .callback = G_CALLBACK(edit_profile_dialog_edit_setting),
            .user_data = dialog,
        },
    };

    const ToolbarItemTemplate dialog_items[] = {
        {
            .text = UPDATE_PROFILE_LABEL,
            .help_text = "The Update Profile button allows you to save changes made to the profile definition.",
            .icon_id = GTK_STOCK_SAVE,
            .callback = G_CALLBACK(edit_profile_dialog_save_changes),
            .user_data = dialog,
        },
        {
            .text = "Cancel",
            .help_text = "The Cancel button allows you to discard any changes made to the profile definition.",
            .icon_id = GTK_STOCK_CANCEL,
            .callback = G_CALLBACK(edit_profile_dialog_cancel),
            .user_data = dialog,
        }
    };

    *settings_items_copy = malloc(sizeof(settings_items));
    memcpy(*settings_items_copy, settings_items, sizeof(settings_items));
    *num_settings_items = ARRAY_LEN(settings_items);

    *dialog_items_copy = malloc(sizeof(dialog_items));
    memcpy(*dialog_items_copy, dialog_items, sizeof(dialog_items));
    *num_dialog_items = ARRAY_LEN(dialog_items);
}


static void setting_key_edited(GtkCellRendererText *renderer,
                               gchar               *path_s,
                               gchar               *new_text,
                               gpointer            user_data)
{
    EditProfileDialog *dialog = (EditProfileDialog *)user_data;
    GtkTreePath *path;
    GtkTreeIter iter;
    json_t *setting;
    GString *error_string;
    const gchar *canonical_key;

    if (dialog->setting_update_canceled) {
        // Don't update anything
        return;
    }

    path = gtk_tree_path_new_from_string(path_s);
    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(dialog->settings_store),
                                 &iter, path)) {
        // The row might have been deleted. Cancel any update
        return;
    }

    gtk_tree_model_get(GTK_TREE_MODEL(dialog->settings_store), &iter,
                       SETTING_LIST_STORE_COL_SETTING, &setting, -1);

    canonical_key = get_canonical_setting_key(new_text);
    if (!canonical_key) {
        error_string = g_string_new("");
        g_string_printf(error_string, "The key [%s] is not recognized by nvidia-settings."
                                      "Please check for spelling errors (keys "
                                      "are NOT case sensitive).", new_text);
        gtk_statusbar_push(GTK_STATUSBAR(dialog->error_statusbar),
                           dialog->setting_error_context_id,
                           error_string->str);
        g_string_free(error_string, TRUE);
    }

    if (canonical_key) {
        json_object_set_new(setting, "key", json_string(canonical_key));
    } else {
        json_object_set_new(setting, "key", json_string(new_text));
    }

    gtk_tree_path_free(path);
}

static gboolean is_valid_setting_value(json_t *value, char **invalid_type_str)
{
    switch (json_typeof(value)) {
        case JSON_STRING:
        case JSON_TRUE:
        case JSON_FALSE:
        case JSON_REAL:
        case JSON_INTEGER:
            *invalid_type_str = NULL;
            return TRUE;
        case JSON_NULL:
            *invalid_type_str = "null";
            return FALSE;
        case JSON_OBJECT:
            *invalid_type_str = "object";
            return FALSE;
        case JSON_ARRAY:
            *invalid_type_str = "array";
            return FALSE;
        default:
            assert(0);
            return FALSE;
    }
}

static void setting_value_edited(GtkCellRendererText *renderer,
                                 gchar               *path_s,
                                 gchar               *new_text,
                                 gpointer            user_data)
{
    gchar *invalid_type_str = NULL;
    gchar *new_text_in_json;
    EditProfileDialog *dialog = (EditProfileDialog *)user_data;
    GtkTreePath *path;
    GtkTreeIter iter;
    json_t *setting;
    json_t *value;
    json_error_t error;
    GString *error_string;

    if (dialog->setting_update_canceled) {
        // Don't update anything
        return;
    }

    path = gtk_tree_path_new_from_string(path_s);
    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(dialog->settings_store),
                            &iter, path)) {
        return;
    }

    gtk_tree_model_get(GTK_TREE_MODEL(dialog->settings_store), &iter,
                       SETTING_LIST_STORE_COL_SETTING, &setting, -1);

    new_text_in_json = nv_app_profile_cfg_file_syntax_to_json(new_text);
    value = json_loads(new_text_in_json, JSON_DECODE_ANY, &error);

    if (!value) {
        error_string = g_string_new("");
        g_string_printf(error_string, "The value [%s] was not understood by the JSON parser.",
                        new_text);
        gtk_statusbar_push(GTK_STATUSBAR(dialog->error_statusbar),
                           dialog->setting_error_context_id,
                           error_string->str);
        g_string_free(error_string, TRUE);
        value = json_false();
    } else if (!is_valid_setting_value(value, &invalid_type_str)) {
        error_string = g_string_new("");
        g_string_printf(error_string, "A value of type \"%s\" is not allowed in the configuration.",
                        invalid_type_str);
        gtk_statusbar_push(GTK_STATUSBAR(dialog->error_statusbar),
                           dialog->setting_error_context_id,
                           error_string->str);
        g_string_free(error_string, TRUE);
        value = json_false();
    }

    json_object_set_new(setting, "value", value);
    free(new_text_in_json);
    gtk_tree_path_free(path);
}

static TreeViewColumnTemplate *get_profile_settings_tree_view_columns(EditProfileDialog *dialog,
                                                                      size_t *num_columns)
{
    TreeViewColumnTemplate *settings_tree_view_columns_copy;
    const TreeViewColumnTemplate settings_tree_view_columns[] = {
        {
            .title = "Key",
            .renderer_func = setting_key_renderer_func,
            .func_data = dialog,
            .min_width = 200,
            .editable = TRUE,
            .edit_callback = G_CALLBACK(setting_key_edited),
            .help_text = "Each entry in the \"Key\" column describes a key for a setting. "
                         "Any string is a valid key in the configuration, but only some strings "
                         "will be understood by the driver at runtime. See the \"Supported Setting Keys\" "
                         "section in the Application Profiles help page for a list of valid "
                         "application profile setting keys. To edit a setting key, double-click "
                         "on the cell containing the key."
        },
        {
            .title = "Type",
            .renderer_func = setting_type_renderer_func,
            .min_width = 100,
            .func_data = NULL,
            .help_text = "Each entry in the \"Type\" column describes the underlying JSON type for "
                         "a setting value. Supported JSON types are: string, true, false, and number. "
                         "This column is read-only."
        },
        {
            .title = "Value",
            .renderer_func = setting_value_renderer_func,
            .func_data = dialog,
            .editable = TRUE,
            .edit_callback = G_CALLBACK(setting_value_edited),
            .help_text = "Each entry in the \"Value\" column describes the value of a setting. To "
                         "edit a setting value, double-click on the cell containing the value. "
                         "Valid input is: an arbitrary string in double-quotes, true, false, or "
                         "an integer or floating-point number. Numbers can optionally be written in "
                         "hexadecimal or octal."
        }
    };

    settings_tree_view_columns_copy = malloc(sizeof(settings_tree_view_columns));
    memcpy(settings_tree_view_columns_copy, settings_tree_view_columns, sizeof(settings_tree_view_columns));
    *num_columns = ARRAY_LEN(settings_tree_view_columns);

    return settings_tree_view_columns_copy;
}

static gboolean profile_settings_tree_view_key_press_event(GtkWidget *widget,
                                                           GdkEvent *event,
                                                           gpointer user_data)
{
    gboolean propagate = FALSE; // Whether to call other handlers in the stack
    EditProfileDialog *dialog = (EditProfileDialog *)user_data;
    GdkEventKey *key_event;

    if (event->type == GDK_KEY_PRESS) {
        key_event = (GdkEventKey *)event;
        if (key_event->keyval == GDK_Delete) {
            edit_profile_dialog_delete_setting_common(dialog);
            propagate = TRUE;
        }

        gtk_statusbar_pop(GTK_STATUSBAR(dialog->error_statusbar), dialog->setting_error_context_id);
    }

    return propagate;
}

static gboolean edit_profile_dialog_handle_delete(GtkWidget *widget,
                                                  GdkEvent *event,
                                                  gpointer user_data)
{
    EditProfileDialog *dialog = (EditProfileDialog *)user_data;
    gtk_widget_set_sensitive(dialog->caller, TRUE);
    gtk_widget_hide(widget);

    return TRUE;
}

static gboolean edit_profile_dialog_generate_name_button_clicked(GtkWidget *widget,
                                                                 gpointer user_data)
{
    EditProfileDialog *dialog = (EditProfileDialog *)user_data;
    CtkAppProfile *ctk_app_profile = CTK_APP_PROFILE(dialog->parent);
    char *unused_profile_name;

    unused_profile_name = nv_app_profile_config_get_unused_profile_name(ctk_app_profile->cur_config);
    g_string_assign(dialog->name, unused_profile_name);
    gtk_entry_set_text(GTK_ENTRY(dialog->name_entry), dialog->name->str);

    return FALSE;
}

static const char __profile_name_help[] = "This entry box contains the current profile name, which is a unique identifier for "
                                          "this profile. Renaming the profile to an existing profile will cause the existing "
                                          "profile to be overwritten with this profile's contents.";
static const char __generate_name_button_help[] = "This button generates a unique name that is not currently used "
                                                  "by the configuration. This can be used to quickly add a new profile without "
                                                  "needing to worry about collisions with existing profile names.";

static EditProfileDialog *edit_profile_dialog_new(CtkAppProfile *ctk_app_profile)
{
    EditProfileDialog *dialog;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *main_vbox;
    GtkWidget *container;
    GtkWidget *entry;
    GtkWidget *toolbar;
    GtkWidget *tree_view;
    GtkWidget *scroll_win;
    GtkWidget *alignment;
    GtkWidget *statusbar;
    GtkWidget *button;
    GList *toolbar_widget_items;

    ToolbarItemTemplate *edit_profile_settings_toolbar_items, *edit_profile_dialog_toolbar_items;
    size_t num_edit_profile_settings_toolbar_items, num_edit_profile_dialog_toolbar_items;
    TreeViewColumnTemplate *settings_tree_view_columns;
    size_t num_settings_tree_view_columns;


    dialog = malloc(sizeof(EditProfileDialog));
    if (!dialog) {
        return NULL;
    }

    settings_tree_view_columns = get_profile_settings_tree_view_columns(dialog, &num_settings_tree_view_columns);

    get_profile_dialog_toolbar_items(dialog,
                                     &edit_profile_settings_toolbar_items,
                                     &num_edit_profile_settings_toolbar_items,
                                     &edit_profile_dialog_toolbar_items,
                                     &num_edit_profile_dialog_toolbar_items);

    dialog->parent = GTK_WIDGET(ctk_app_profile);
    dialog->top_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    dialog->top_help_data = NULL;
    dialog->setting_toolbar_help_data = NULL;
    dialog->bottom_help_data = NULL;

    gtk_window_set_modal(GTK_WINDOW(dialog->top_window), TRUE);

    g_signal_connect(G_OBJECT(dialog->top_window), "delete-event",
                     G_CALLBACK(edit_profile_dialog_handle_delete), dialog);

    gtk_widget_set_size_request(dialog->top_window, 500, 480);
    gtk_container_set_border_width(GTK_CONTAINER(dialog->top_window), 8);

    dialog->name = g_string_new("");
    dialog->orig_name = g_string_new("");
    dialog->source_file = g_string_new("");
    dialog->settings = json_array();

    dialog->settings_store = gtk_list_store_new(SETTING_LIST_STORE_NUM_COLS, G_TYPE_POINTER);
    dialog->file_sel = gtk_file_selection_new("Please select a source file for the profile");

    main_vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_set_spacing(GTK_BOX(main_vbox), 8);

    gtk_container_add(GTK_CONTAINER(dialog->top_window), main_vbox);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_set_spacing(GTK_BOX(hbox), 4);

    label = gtk_label_new("Profile Name");
    dialog->name_entry = entry = gtk_entry_new();

    ctk_config_set_tooltip_and_add_help_data(ctk_app_profile->ctk_config,
                                         label,
                                         &dialog->top_help_data,
                                         "Profile Name",
                                         __profile_name_help,
                                         NULL);

    dialog->generate_name_button = button = gtk_button_new_with_label("Generate Name");

    ctk_config_set_tooltip_and_add_help_data(ctk_app_profile->ctk_config,
                                         button,
                                         &dialog->top_help_data,
                                         "Generate Name",
                                         __generate_name_button_help,
                                         NULL);

    dialog->top_help_data = g_list_reverse(dialog->top_help_data);

    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(edit_profile_dialog_generate_name_button_clicked),
                     (gpointer)dialog);

    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(main_vbox), hbox, FALSE, FALSE, 0);

    config_create_source_file_entry(ctk_app_profile->ctk_config,
                                    &container,
                                    &dialog->source_file_combo,
                                    &dialog->top_help_data,
                                    "profile",
                                    G_CALLBACK(profile_browse_button_clicked),
                                    (gpointer)dialog);

    gtk_box_pack_start(GTK_BOX(main_vbox), container, FALSE, FALSE, 0);

    toolbar = gtk_toolbar_new();
    populate_toolbar(GTK_TOOLBAR(toolbar),
                     edit_profile_settings_toolbar_items,
                     num_edit_profile_settings_toolbar_items,
                     &dialog->setting_toolbar_help_data,
                     NULL);

    gtk_box_pack_start(GTK_BOX(main_vbox), toolbar, FALSE, FALSE, 0);

    tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(dialog->settings_store));
    populate_tree_view(GTK_TREE_VIEW(tree_view),
                       settings_tree_view_columns,
                       ctk_app_profile,
                       num_settings_tree_view_columns,
                       &dialog->setting_column_help_data);

    g_signal_connect(G_OBJECT(tree_view), "key-press-event",
                     G_CALLBACK(profile_settings_tree_view_key_press_event),
                     (gpointer)dialog);

    dialog->settings_view = GTK_TREE_VIEW(tree_view);

    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tree_view), TRUE);

    scroll_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll_win), tree_view);
    gtk_box_pack_start(GTK_BOX(main_vbox), scroll_win, TRUE, TRUE, 0);

    dialog->setting_update_canceled = FALSE;

    dialog->error_statusbar = statusbar = gtk_statusbar_new();
    gtk_box_pack_start(GTK_BOX(main_vbox), statusbar, FALSE, FALSE, 0);

    dialog->setting_error_context_id =
        gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar),
                                     "Profile Settings");

    alignment = gtk_alignment_new(1.0, 0.5, 0.0, 0.0);
    toolbar = gtk_toolbar_new();
    populate_toolbar(GTK_TOOLBAR(toolbar),
                     edit_profile_dialog_toolbar_items,
                     num_edit_profile_dialog_toolbar_items,
                     &dialog->bottom_help_data,
                     &toolbar_widget_items);

    // Save off the "Update Profile" button for later use
    dialog->add_edit_profile_button = find_widget_in_widget_data_list(toolbar_widget_items, UPDATE_PROFILE_LABEL);

    widget_data_list_free_full(toolbar_widget_items);

    gtk_container_add(GTK_CONTAINER(alignment), toolbar);
    gtk_box_pack_start(GTK_BOX(main_vbox), alignment, FALSE, FALSE, 0);

    free(edit_profile_settings_toolbar_items);
    free(edit_profile_dialog_toolbar_items);
    free(settings_tree_view_columns);

    return dialog;
}

static void edit_profile_dialog_destroy(EditProfileDialog *dialog)
{
    g_string_free(dialog->name, TRUE);
    g_string_free(dialog->orig_name, TRUE);
    g_string_free(dialog->source_file, TRUE);
    json_decref(dialog->settings);

    ctk_help_data_list_free_full(dialog->top_help_data);
    ctk_help_data_list_free_full(dialog->setting_column_help_data);
    ctk_help_data_list_free_full(dialog->setting_toolbar_help_data);
    ctk_help_data_list_free_full(dialog->bottom_help_data);
    free(dialog);
}

static void rules_tree_view_row_activated_callback(GtkTreeView *tree_view,
                                                   GtkTreePath *path,
                                                   GtkTreeViewColumn *column,
                                                   gpointer user_data)
{
    CtkAppProfile *ctk_app_profile = (CtkAppProfile *)user_data;
    edit_rule_callbacks_common(ctk_app_profile, path);
}

static GtkWidget* create_rules_page(CtkAppProfile *ctk_app_profile)
{
    GtkWidget *vbox;
    GtkWidget *scroll_win;
    GtkWidget *tree_view;
    GtkWidget *toolbar;
    GtkTreeModel *model;

    const ToolbarItemTemplate rules_toolbar_items[] = {
        {
            .text = "Add Rule",
            .help_text = "The Add Rule button allows you to create a new rule for applying custom settings "
                         "to applications which match a given pattern.",
            .extended_help_text = "See the \"Add/Edit Rule Dialog Box\" help section for more "
                                  "information on adding new rules.",
            .icon_id = GTK_STOCK_ADD,
            .callback = (GCallback)add_rule_callback,
            .user_data = ctk_app_profile
        },
        {
            .text = "Delete Rule",
            .help_text = "The Delete Rule button allows you to remove a highlighted rule from the list.",
            .icon_id = GTK_STOCK_REMOVE,
            .callback = (GCallback)delete_rule_callback,
            .user_data = ctk_app_profile
        },
        {
            .text = "Increase Rule Priority",
            .help_text = "This increases the priority of the highlighted rule in the list. If multiple rules "
                         "with a conflicting driver setting match the same application, the application will "
                         "take on the setting value of the highest-priority rule (lowest number) in the list.",
            .extended_help_text = "Note that the priority of a rule is partially determined by the source file "
                                  "where the rule is defined, since the NVIDIA driver prioritizes rules based "
                                  "on their position along the configuration file search path. Hence, nvidia-settings "
                                  "may move the rule to a different source file if it is necessary for the rule to achieve "
                                  "a particular priority.",
            .icon_id = GTK_STOCK_GO_UP,
            .callback = (GCallback)increase_rule_priority_callback,
            .user_data = ctk_app_profile
        },
        {
            .text = "Decrease Rule Priority",
            .help_text = "This decreases the priority of the highlighted rule in the list. If multiple rules "
                         "with a conflicting driver setting match the same application, the application will "
                         "take on the setting value of the highest-priority rule (lowest number) in the list.",
            .icon_id = GTK_STOCK_GO_DOWN,
            .callback = (GCallback)decrease_rule_priority_callback,
            .user_data = ctk_app_profile
        },
        {
            .text = "Edit Rule",
            .help_text = "The Edit Rule button allows you to edit a highlighted rule in the list.",
            .extended_help_text = "See the \"Add/Edit Rule Dialog Box\" help section for more "
                                  "information on editing rules.",
            // Would be nice to use GTK_STOCK_EDIT here, but unfortunately only
            // available from 2.6 onwards...
            .icon_id = GTK_STOCK_PREFERENCES,
            .callback = (GCallback)edit_rule_callback,
            .user_data = ctk_app_profile
        },
    };

    const TreeViewColumnTemplate rules_tree_view_columns[] = {
        // TODO asterisk column to denote changes
        {
            .title = "Priority",
            .renderer_func = rule_order_renderer_func,
            .func_data = NULL,
            .help_text = "This column describes the priority of each rule in the configuration. "
                         "If two rules match the same process and affect settings which overlap, "
                         "the overlapping settings will be set to the values specified by the rule "
                         "with the lower number (higher priority) in this column."
        },
        {
            .title = "Pattern",
            .renderer_func = rule_pattern_renderer_func,
            .func_data = NULL,
            .help_text = "This column describes the pattern against which the driver will compare "
                         "the currently running process to determine if it should apply profile settings. ",
            .extended_help_text = "See the \"Supported Features\" help section for more information on "
                                  "supported pattern types."
        },
        {
            .title = "Profile Settings",
            .renderer_func = rule_profile_settings_renderer_func,
            .func_data = (gpointer)ctk_app_profile,
            .help_text = "This column describes the settings that will be applied to processes "
                         "that match the pattern in each rule. Note that profile settings are properties "
                         "of the profile itself, and not the associated rule."
        },
        {
            .title = "Profile Name",
            .attribute = "text",
            .attr_col = CTK_APC_RULE_MODEL_COL_PROFILE_NAME,
            .help_text = "This column describes the name of the profile that will be applied to processes "
                         "that match the pattern in each rule."
        },
        {
            .title = "Source File",
            .attribute = "text",
            .attr_col = CTK_APC_RULE_MODEL_COL_FILENAME,
            .help_text = "This column describes the configuration file where the rule is defined. Note that "
                         "the NVIDIA® Linux Graphics Driver searches for application profiles along a fixed "
                         "search path, and the location of the configuration file in the search path can "
                         "affect a rule's priority. See the README for more details."
        },
    };

    vbox = gtk_vbox_new(FALSE, 0);

    /* Create the toolbar */
    toolbar = gtk_toolbar_new();
    populate_toolbar(GTK_TOOLBAR(toolbar),
                     rules_toolbar_items,
                     ARRAY_LEN(rules_toolbar_items),
                     &ctk_app_profile->rules_help_data,
                     NULL);

    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    /* Create the main tree view */
    scroll_win = gtk_scrolled_window_new(NULL, NULL);

    model = GTK_TREE_MODEL(ctk_app_profile->apc_rule_model);

    tree_view = gtk_tree_view_new_with_model(model);

    populate_tree_view(GTK_TREE_VIEW(tree_view),
                       rules_tree_view_columns,
                       ctk_app_profile,
                       ARRAY_LEN(rules_tree_view_columns),
                       &ctk_app_profile->rules_columns_help_data);

    g_signal_connect(G_OBJECT(tree_view), "row-activated",
                     G_CALLBACK(rules_tree_view_row_activated_callback),
                     (gpointer)ctk_app_profile);

    g_signal_connect(G_OBJECT(tree_view), "key-press-event",
                     G_CALLBACK(rules_tree_view_key_press_event),
                     (gpointer)ctk_app_profile);

    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tree_view), TRUE);

    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(tree_view), TRUE);

    gtk_container_add(GTK_CONTAINER(scroll_win), tree_view);

    ctk_app_profile->main_rule_view = GTK_TREE_VIEW(tree_view);

    gtk_box_pack_start(GTK_BOX(vbox), scroll_win, TRUE, TRUE, 0);

    return vbox;
}

static void profile_settings_renderer_func(GtkTreeViewColumn *tree_column,
                                           GtkCellRenderer   *cell,
                                           GtkTreeModel      *model,
                                           GtkTreeIter       *iter,
                                           gpointer           data)
{
    char *settings_string;
    json_t *settings;

    gtk_tree_model_get(model, iter, CTK_APC_PROFILE_MODEL_COL_SETTINGS, &settings, -1);

    settings_string = serialize_settings(settings, TRUE);

    g_object_set(cell, "markup", settings_string, NULL);

    free(settings_string);
    json_decref(settings);
}

static void delete_profile_callback_common(CtkAppProfile *ctk_app_profile)
{
    GtkTreeViewColumn *focus_column;
    GtkTreeIter iter;
    GtkTreePath *path;
    char *profile_name;

    // Get currently highlighted row
    gtk_tree_view_get_cursor(ctk_app_profile->main_profile_view,
                             &path, &focus_column);
    if (!path) {
        return;
    }

    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(ctk_app_profile->apc_profile_model),
                                 &iter, path)) {
        return;
    }

    gtk_tree_model_get(GTK_TREE_MODEL(ctk_app_profile->apc_profile_model),
                       &iter, CTK_APC_PROFILE_MODEL_COL_NAME, &profile_name, -1);

    // Delete the row
    ctk_apc_profile_model_delete_profile(ctk_app_profile->apc_profile_model,
                                         profile_name);

    // Select next profile in the list, if available
    choose_next_row_in_list_view(ctk_app_profile->main_profile_view,
                                 GTK_TREE_MODEL(ctk_app_profile->apc_profile_model),
                                 &path);
    gtk_tree_view_set_cursor(ctk_app_profile->main_profile_view,
                             path, NULL, FALSE);

    ctk_config_statusbar_message(ctk_app_profile->ctk_config,
                                 "Profile \"%s\" deleted. %s",
                                 profile_name,
                                 STATUSBAR_UPDATE_WARNING);

    gtk_tree_path_free(path);
    free(profile_name);
}

static void delete_profile_callback(GtkWidget *widget, gpointer user_data)
{
    CtkAppProfile *ctk_app_profile = (CtkAppProfile *)user_data;
    delete_profile_callback_common(ctk_app_profile);
}

static gboolean profiles_tree_view_key_press_event(GtkWidget *widget,
                                                   GdkEvent *event,
                                                   gpointer user_data)
{
    CtkAppProfile *ctk_app_profile = (CtkAppProfile *)user_data;
    GdkEventKey *key_event;

    if (event->type == GDK_KEY_PRESS) {
        key_event = (GdkEventKey *)event;
        if (key_event->keyval == GDK_Delete) {
            delete_profile_callback_common(ctk_app_profile);
            return TRUE;
        }
    }

    // Use default handlers
    return FALSE;
}

static void edit_profile_dialog_load_values(EditProfileDialog *dialog)
{
    GList *strings;
    GtkCombo *combo;

    // window title
    gtk_window_set_title(GTK_WINDOW(dialog->top_window),
                         dialog->new_profile ? "Add new profile" : "Edit existing profile");

    // add/edit button
    button_set_label_and_stock_icon(GTK_BUTTON(dialog->add_edit_profile_button),
                                    "Update Profile",
                                    dialog->new_profile ? GTK_STOCK_ADD : GTK_STOCK_PREFERENCES);

    // profile name
    gtk_entry_set_text(GTK_ENTRY(dialog->name_entry), dialog->name->str);

    // source file
    combo = GTK_COMBO(dialog->source_file_combo);
    strings = get_source_filenames(CTK_APP_PROFILE(dialog->parent));

    gtk_combo_set_popdown_strings(combo, strings);

    if (dialog->new_profile) {
        if (g_list_length(strings)) {
            // Choose first string in the list
            g_string_assign(dialog->source_file, (gchar *)strings->data);
        } else {
            g_string_assign(dialog->source_file, "");
        }
    }

    gtk_entry_set_text(GTK_ENTRY(combo->entry), dialog->source_file->str);

    string_list_free_full(strings);

    // profile settings
    if (!dialog->new_profile) {
        load_settings_from_profile(CTK_APP_PROFILE(dialog->parent),
                                   dialog->settings_store,
                                   dialog->name->str);
    } else {
        gtk_list_store_clear(dialog->settings_store);
    }
}

static void edit_profile_dialog_show(EditProfileDialog *dialog)
{
    edit_profile_dialog_load_values(dialog);
    gtk_widget_show_all(dialog->top_window);

    // disable focusing to calling window until this window closed
    gtk_window_set_transient_for(GTK_WINDOW(dialog->top_window),
                                 GTK_WINDOW(gtk_widget_get_toplevel(dialog->caller)));
    gtk_widget_set_sensitive(dialog->caller, FALSE);
}

static void add_profile_callbacks_common(CtkAppProfile *ctk_app_profile,
                                         GtkWidget *caller)
{
    EditProfileDialog *dialog = ctk_app_profile->edit_profile_dialog;
    char *unused_profile_name = nv_app_profile_config_get_unused_profile_name(ctk_app_profile->cur_config);

    dialog->new_profile = TRUE;
    dialog->caller = caller;

    g_string_assign(dialog->name, unused_profile_name);
    g_string_truncate(dialog->orig_name, 0);

    free(unused_profile_name);

    edit_profile_dialog_show(dialog);
}

static void add_profile_callback(GtkWidget *widget, gpointer user_data)
{
    CtkAppProfile *ctk_app_profile = (CtkAppProfile *)user_data;
    add_profile_callbacks_common(ctk_app_profile, GTK_WIDGET(ctk_app_profile));
}

static void edit_profile_callbacks_common(CtkAppProfile *ctk_app_profile,
                                          GtkTreePath *path,
                                          GtkWidget *caller)
{
    GtkTreeIter iter;
    EditProfileDialog *dialog = ctk_app_profile->edit_profile_dialog;
    gchar *name, *filename;
    json_t *settings;

    if (!path) {
        return;
    }

    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(ctk_app_profile->apc_profile_model),
                                 &iter, path)) {
        return;
    }

    gtk_tree_model_get(GTK_TREE_MODEL(ctk_app_profile->apc_profile_model),
                       &iter,
                       CTK_APC_PROFILE_MODEL_COL_NAME, &name,
                       CTK_APC_PROFILE_MODEL_COL_SETTINGS, &settings,
                       CTK_APC_PROFILE_MODEL_COL_FILENAME, &filename,
                       -1);

    dialog->new_profile = FALSE;
    dialog->caller = caller;

    g_string_assign(dialog->name, name);
    g_string_assign(dialog->orig_name, name);

    dialog->settings = json_deep_copy(settings);
    g_string_assign(dialog->source_file, filename);

    edit_profile_dialog_show(dialog);

    json_decref(settings);
    free(name);
    free(filename);
}

static void edit_profile_callback(GtkWidget *widget, gpointer user_data)
{
    GtkTreeViewColumn *focus_column;
    GtkTreePath *path;
    CtkAppProfile *ctk_app_profile = (CtkAppProfile *)user_data;

    // Get currently highlighted row
    gtk_tree_view_get_cursor(ctk_app_profile->main_profile_view,
                             &path, &focus_column);

    edit_profile_callbacks_common(ctk_app_profile, path, GTK_WIDGET(ctk_app_profile));

    gtk_tree_path_free(path);
}

static void profiles_tree_view_row_activated_callback(GtkTreeView *tree_view,
                                                      GtkTreePath *path,
                                                      GtkTreeViewColumn *column,
                                                      gpointer user_data)
{
    CtkAppProfile *ctk_app_profile = (CtkAppProfile *)user_data;
    edit_profile_callbacks_common(ctk_app_profile, path, GTK_WIDGET(ctk_app_profile));
}


static GtkWidget* create_profiles_page(CtkAppProfile *ctk_app_profile)
{
    GtkWidget *vbox;
    GtkWidget *toolbar;
    GtkWidget *scroll_win;
    GtkWidget *tree_view;
    GtkTreeModel *model;

    const ToolbarItemTemplate profiles_toolbar_items[] = {
        {
            .text = "Add Profile",
            .help_text = "The Add Profile button allows you to create a new profile for applying custom settings "
                            "to applications which match a given pattern.",
            .extended_help_text = "See the \"Add/Edit Profile Dialog Box\" help section for more "
                                  "information on adding new profiles.",
            .icon_id = GTK_STOCK_ADD,
            .callback = (GCallback)add_profile_callback,
            .user_data = ctk_app_profile
        },
        {
            .text = "Delete Profile",
            .help_text = "The Delete Profile button allows you to remove a highlighted profile from the list.",
            .icon_id = GTK_STOCK_REMOVE,
            .callback = (GCallback)delete_profile_callback,
            .user_data = ctk_app_profile
        },
        {
            .text = "Edit Profile",
            .help_text = "The Edit Profile button allows you to edit a highlighted profile in the list.",
            .extended_help_text = "See the \"Add/Edit Profile Dialog Box\" help section for more "
                                  "information on editing profiles.",
            // Would be nice to use GTK_STOCK_EDIT here, but unfortunately only
            // available from 2.6 onwards...
            .icon_id = GTK_STOCK_PREFERENCES,
            .callback = (GCallback)edit_profile_callback,
            .user_data = ctk_app_profile
        },
    };

    const TreeViewColumnTemplate profiles_tree_view_columns[] = {
        // TODO asterisk column to denote changes
        {
            .title = "Profile Name",
            .attribute = "text",
            .attr_col = CTK_APC_PROFILE_MODEL_COL_NAME,
            .sortable = TRUE,
            .sort_column_id = CTK_APC_PROFILE_MODEL_COL_NAME,
            .help_text = "This column describes the name of the profile."
        },
        {
            .title = "Profile Settings",
            .renderer_func = profile_settings_renderer_func,
            .func_data = NULL,
            .sortable = TRUE,
            .sort_column_id = CTK_APC_PROFILE_MODEL_COL_SETTINGS,
            .help_text = "This column describes the settings that will be applied by rules "
                         "which use this profile."
        },
        {
            .title = "Source File",
            .attribute = "text",
            .attr_col = CTK_APC_PROFILE_MODEL_COL_FILENAME,
            .sortable = TRUE,
            .sort_column_id = CTK_APC_PROFILE_MODEL_COL_FILENAME,
            .help_text = "This column describes the configuration file where the profile is defined."
        },
    };

    vbox = gtk_vbox_new(FALSE, 0);

    /* Create the toolbar */
    toolbar = gtk_toolbar_new();
    populate_toolbar(GTK_TOOLBAR(toolbar),
                     profiles_toolbar_items,
                     ARRAY_LEN(profiles_toolbar_items),
                     &ctk_app_profile->profiles_help_data,
                     NULL);

    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    /* Create the main tree view */
    scroll_win = gtk_scrolled_window_new(NULL, NULL);

    model = GTK_TREE_MODEL(ctk_app_profile->apc_profile_model);
    tree_view = gtk_tree_view_new_with_model(model);

    populate_tree_view(GTK_TREE_VIEW(tree_view),
                       profiles_tree_view_columns,
                       ctk_app_profile,
                       ARRAY_LEN(profiles_tree_view_columns),
                       &ctk_app_profile->profiles_columns_help_data);

    g_signal_connect(G_OBJECT(tree_view), "row-activated",
                     G_CALLBACK(profiles_tree_view_row_activated_callback),
                     (gpointer)ctk_app_profile);

    g_signal_connect(G_OBJECT(tree_view), "key-press-event",
                     G_CALLBACK(profiles_tree_view_key_press_event),
                     (gpointer)ctk_app_profile);

    ctk_app_profile->main_profile_view = GTK_TREE_VIEW(tree_view);

    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tree_view), TRUE);

    gtk_container_add(GTK_CONTAINER(scroll_win), tree_view);

    gtk_box_pack_start(GTK_BOX(vbox), scroll_win, TRUE, TRUE, 0);

    return vbox;
}

static char *get_default_global_config_file(void)
{
    const char *homeStr = getenv("HOME");
    if (homeStr) {
        return nvstrcat(homeStr, "/.nv/nvidia-application-profile-globals-rc", NULL);
    } else {
        nv_error_msg("The environment variable HOME is not set. Any "
                     "modifications to global application profile settings "
                     "will not be saved.");
        return NULL;
    }
}

#define SEARCH_PATH_NUM_FILES 4

static char **get_default_search_path(size_t *num_files)
{
    size_t i = 0;
    char **filenames = malloc(SEARCH_PATH_NUM_FILES * sizeof(char *));
    const char *homeStr = getenv("HOME");

    if (homeStr) {
        filenames[i++] = nvstrcat(homeStr, "/.nv/nvidia-application-profiles-rc", NULL);
        filenames[i++] = nvstrcat(homeStr, "/.nv/nvidia-application-profiles-rc.d", NULL);
    }
    filenames[i++] = strdup("/etc/nvidia/nvidia-application-profiles-rc");
    filenames[i++] = strdup("/etc/nvidia/nvidia-application-profiles-rc.d");

    *num_files = i;
    assert(i <= SEARCH_PATH_NUM_FILES);

    return filenames;
}

static void free_search_path(char **search_path, size_t search_path_size)
{
    while (search_path_size--) {
        free(search_path[search_path_size]);
    }
    free(search_path);
}

static void app_profile_load_global_settings(CtkAppProfile *ctk_app_profile,
                                             AppProfileConfig *config)
{
    // Temporarily disable propagating statusbar messages since the
    // enabled_check_button_toggled() callback will otherwise update the
    // statusbar
    ctk_app_profile->ctk_config->status_bar.enabled = FALSE;
    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(ctk_app_profile->enable_check_button),
        nv_app_profile_config_get_enabled(config));
    ctk_app_profile->ctk_config->status_bar.enabled = TRUE;
}

static void app_profile_reload(CtkAppProfile *ctk_app_profile)
{
    char *global_config_file;
    char **search_path;
    size_t search_path_size;

    nv_app_profile_config_free(ctk_app_profile->cur_config);
    nv_app_profile_config_free(ctk_app_profile->gold_config);

    search_path = get_default_search_path(&search_path_size);
    global_config_file = get_default_global_config_file();
    ctk_app_profile->gold_config = nv_app_profile_config_load(global_config_file,
                                                              search_path,
                                                              search_path_size);
    ctk_app_profile->cur_config = nv_app_profile_config_dup(ctk_app_profile->gold_config);
    free_search_path(search_path, search_path_size);
    free(global_config_file);

    ctk_apc_profile_model_attach(ctk_app_profile->apc_profile_model, ctk_app_profile->cur_config);
    ctk_apc_rule_model_attach(ctk_app_profile->apc_rule_model, ctk_app_profile->cur_config);
    app_profile_load_global_settings(ctk_app_profile, ctk_app_profile->cur_config);
}


static void reload_callback(GtkWidget *widget, gpointer user_data)
{
    CtkAppProfile *ctk_app_profile = (CtkAppProfile *)user_data;
    json_t *updates;
    gboolean do_reload = TRUE;
    GString *fatal_errors = g_string_new("");
    GString *nonfatal_errors = g_string_new("");

    static const char unsaved_changes_error[] =
        "There are unsaved changes in the configuration which will be permanently lost if "
        "the configuration is reloaded from disk.\n";
    static const char files_altered_error[] =
        "Some configuration files may have been modified externally since the configuration "
        "was last loaded from disk.\n";

    updates = nv_app_profile_config_validate(ctk_app_profile->cur_config,
                                             ctk_app_profile->gold_config);

    if (json_array_size(updates) > 0) {
        g_string_append_printf(nonfatal_errors, "%s\t%s", get_bullet(), unsaved_changes_error);
    }
    if (nv_app_profile_config_check_backing_files(ctk_app_profile->cur_config)) {
        g_string_append_printf(nonfatal_errors, "%s\t%s", get_bullet(), files_altered_error);
    }

    do_reload = run_error_dialog(GTK_WINDOW(gtk_widget_get_toplevel(
                                    GTK_WIDGET(ctk_app_profile))),
                                 fatal_errors,
                                 nonfatal_errors,
                                 "reload the configuration from disk");


    if (do_reload) {
        app_profile_reload(ctk_app_profile);
        ctk_config_statusbar_message(ctk_app_profile->ctk_config,
                                     "Application profile configuration reloaded from disk.");
    }

    g_string_free(fatal_errors, TRUE);
    g_string_free(nonfatal_errors, TRUE);
}

static void save_changes_callback(GtkWidget *widget, gpointer user_data);

static ToolbarItemTemplate *get_save_reload_toolbar_items(CtkAppProfile *ctk_app_profile, size_t *num_save_reload_toolbar_items)
{
    ToolbarItemTemplate *save_reload_toolbar_items_copy;
    const ToolbarItemTemplate save_reload_toolbar_items[] = {
        {
            .text = "Save Changes",
            .help_text = "The Save Changes button allows you to save any changes to application profile "
                         "configuration files to disk.",
            .extended_help_text = "This button displays a dialog box which allows you to preview the changes "
                                  "that will be made to the JSON configuration files, and toggle whether nvidia-settings "
                                  "should make backup copies of the original files before overwriting existing files.",
            .icon_id = GTK_STOCK_SAVE,
            .callback = (GCallback)save_changes_callback,
            .user_data = ctk_app_profile
        },
        {
            .text = "Reload",
            .help_text = "The Reload button allows you to reload application profile configuration from "
                         "disk, reverting any unsaved changes.",
            .extended_help_text = "If nvidia-settings detects unsaved changes in the configuration, this button will "
                                  "display a dialog box to warn you before attempting to reload.",
            .icon_id = GTK_STOCK_REFRESH,
            .callback = (GCallback)reload_callback,
            .user_data = ctk_app_profile
        }
    };

    save_reload_toolbar_items_copy = malloc(sizeof(save_reload_toolbar_items));
    memcpy(save_reload_toolbar_items_copy, save_reload_toolbar_items, sizeof(save_reload_toolbar_items));

    *num_save_reload_toolbar_items = ARRAY_LEN(save_reload_toolbar_items);
    return save_reload_toolbar_items_copy;
}

static void save_app_profile_changes_dialog_save_changes(GtkWidget *widget, gpointer user_data)
{
    gboolean do_save = TRUE;
    gboolean do_reload = TRUE;
    gboolean do_backup = TRUE;
    gint result;
    int ret;
    GtkWidget *error_dialog;
    SaveAppProfileChangesDialog *dialog = (SaveAppProfileChangesDialog *)user_data;
    CtkAppProfile *ctk_app_profile = CTK_APP_PROFILE(dialog->parent);
    static const char config_files_changed_string[] =
        "nvidia-settings has detected that configuration files have changed "
        "since the configuration was last loaded. Saving the configuration "
        "may cause these changes to be permanently lost. Continue anyway?\n";
    static const char write_errors_occurred_string[] =
        "nvidia-settings encountered errors when writing to the configuration. "
        "Some changes may not have been saved. Reload the configuration anyway?\n";

    // First check for possible conflicts
    if (nv_app_profile_config_check_backing_files(ctk_app_profile->cur_config)) {
        error_dialog = gtk_message_dialog_new(GTK_WINDOW(dialog->top_window),
                                              GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_MESSAGE_QUESTION,
                                              GTK_BUTTONS_YES_NO,
                                              "%s", config_files_changed_string);
        result = gtk_dialog_run(GTK_DIALOG(error_dialog));
        if (result != GTK_RESPONSE_YES) {
            do_save = FALSE;
        }
        gtk_widget_destroy(error_dialog);
    }

    do_backup = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->backup_check_button));

    if (do_save) {
        ret = nv_app_profile_config_save_updates(ctk_app_profile->cur_config, dialog->updates,
                                                 do_backup);
        if (ret < 0) {
            error_dialog = gtk_message_dialog_new(GTK_WINDOW(dialog->top_window),
                                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  GTK_MESSAGE_QUESTION,
                                                  GTK_BUTTONS_YES_NO,
                                                  "%s", write_errors_occurred_string);
            result = gtk_dialog_run(GTK_DIALOG(error_dialog));
            if (result != GTK_RESPONSE_YES) {
                do_reload = FALSE;
            }
            gtk_widget_destroy(error_dialog);
        }

        if (do_reload) {
            app_profile_reload(CTK_APP_PROFILE(dialog->parent));
        }

        ctk_config_statusbar_message(ctk_app_profile->ctk_config,
                                     "Application profile configuration saved to disk.");
    }

    json_decref(dialog->updates);
    dialog->updates = NULL;

    gtk_widget_set_sensitive(dialog->parent, TRUE);
    gtk_widget_hide(dialog->top_window);
}

static void save_app_profile_changes_dialog_cancel(GtkWidget *widget, gpointer user_data)
{
    SaveAppProfileChangesDialog *dialog = (SaveAppProfileChangesDialog *)user_data;

    json_decref(dialog->updates);
    dialog->updates = NULL;

    gtk_widget_set_sensitive(dialog->parent, TRUE);
    gtk_widget_hide(dialog->top_window);
}

static ToolbarItemTemplate *get_save_app_profile_changes_toolbar_items(SaveAppProfileChangesDialog *dialog,
                                                                       size_t *num_items)
{
    ToolbarItemTemplate *items_copy;
    const ToolbarItemTemplate items[] = {
        {
            .text = "Save Changes",
            .help_text = "Save the changes to disk.",
            .icon_id = GTK_STOCK_SAVE,
            .callback = G_CALLBACK(save_app_profile_changes_dialog_save_changes),
            .user_data = dialog,
        },
        {
            .text = "Cancel",
            .help_text = "Cancel the save operation.",
            .icon_id = GTK_STOCK_CANCEL,
            .callback = G_CALLBACK(save_app_profile_changes_dialog_cancel),
            .user_data = dialog,
        }
    };

    items_copy = malloc(sizeof(items));
    memcpy(items_copy, items, sizeof(items));

    *num_items = ARRAY_LEN(items);
    return items_copy;
}

static void save_app_profile_changes_dialog_set_preview_visibility(SaveAppProfileChangesDialog *dialog,
                                                                   gboolean visible)
{
    dialog->show_preview = visible;
    if (visible) {
        gtk_widget_show(dialog->preview_vbox);
        gtk_window_set_resizable(GTK_WINDOW(dialog->top_window), TRUE);
        gtk_widget_set_size_request(dialog->preview_vbox, -1, 400);
        gtk_button_set_label(GTK_BUTTON(dialog->preview_button), "Hide Preview");
    } else {
        gtk_widget_hide(dialog->preview_vbox);
        gtk_window_set_resizable(GTK_WINDOW(dialog->top_window), FALSE);
        gtk_button_set_label(GTK_BUTTON(dialog->preview_button), "Show Preview");
    }
}

static gboolean save_app_profile_changes_show_preview_button_clicked(GtkWidget *widget, gpointer user_data)
{
    SaveAppProfileChangesDialog *save_dialog = (SaveAppProfileChangesDialog *)user_data;

    // Toggle visibility of the preview window
    save_app_profile_changes_dialog_set_preview_visibility(save_dialog, !save_dialog->show_preview);
    return FALSE;
}

static void save_app_profile_settings_dialog_load_current_update(SaveAppProfileChangesDialog *dialog)
{
    CtkAppProfile *ctk_app_profile;
    size_t i, size;
    CtkDropDownMenu *menu;
    const char *filename;
    const char *text;
    char *backup_filename;
    GtkTextBuffer *text_buffer;
    json_t *update, *update_filename;

    ctk_app_profile = CTK_APP_PROFILE(dialog->parent);

    menu = CTK_DROP_DOWN_MENU(dialog->preview_file_menu);
    filename = ctk_drop_down_menu_get_current_name(menu);

    text = NULL;
    for (i = 0, size = json_array_size(dialog->updates); i < size; i++) {
        update = json_array_get(dialog->updates, i);
        update_filename = json_object_get(update, "filename");
        if (!strcmp(json_string_value(update_filename), filename)) {
            text = json_string_value(json_object_get(update, "text"));
        }
    }

    backup_filename = nv_app_profile_config_get_backup_filename(ctk_app_profile->cur_config, filename);
    gtk_entry_set_text(GTK_ENTRY(dialog->preview_backup_entry), backup_filename);
    free(backup_filename);

    text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(dialog->preview_text_view));

    if (text) {
        gtk_text_buffer_set_text(text_buffer, text, -1);
    } else {
        gtk_text_buffer_set_text(text_buffer, "", -1);
    }
}

static void save_app_profile_changes_dialog_preview_changed(GtkWidget *widget, gpointer user_data)
{
    SaveAppProfileChangesDialog *dialog = (SaveAppProfileChangesDialog *)user_data;
    save_app_profile_settings_dialog_load_current_update(dialog);
}

static gboolean save_app_profile_changes_dialog_handle_delete(GtkWidget *widget,
                                                              GdkEvent *event,
                                                              gpointer user_data)
{
    SaveAppProfileChangesDialog *dialog = (SaveAppProfileChangesDialog *)user_data;
    gtk_widget_set_sensitive(dialog->parent, TRUE);
    gtk_widget_hide(widget);

    return TRUE;
}

static SaveAppProfileChangesDialog *save_app_profile_changes_dialog_new(CtkAppProfile *ctk_app_profile)
{
    ToolbarItemTemplate *toolbar_items;
    size_t num_toolbar_items;
    GtkWidget *toolbar;
    GtkWidget *alignment;
    GtkWidget *vbox, *preview_vbox, *hbox;
    GtkWidget *label;
    GtkWidget *menu;
    GtkWidget *check_button;
    GtkWidget *scroll_win, *text_view;
    SaveAppProfileChangesDialog *dialog = malloc(sizeof(SaveAppProfileChangesDialog));

    dialog->parent = GTK_WIDGET(ctk_app_profile);
    dialog->top_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    dialog->show_preview = FALSE;

    gtk_window_set_title(GTK_WINDOW(dialog->top_window), "Save Changes");

    gtk_window_set_modal(GTK_WINDOW(dialog->top_window), TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(dialog->top_window), 8);

    g_signal_connect(G_OBJECT(dialog->top_window), "delete-event",
                     G_CALLBACK(save_app_profile_changes_dialog_handle_delete), dialog);


    gtk_widget_set_size_request(dialog->top_window, 500, -1);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_set_spacing(GTK_BOX(vbox), 8);

    gtk_container_add(GTK_CONTAINER(dialog->top_window), vbox);

    label = gtk_label_new("The following files will be modified after the configuration is saved.");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 8);

    dialog->preview_file_menu = menu = ctk_drop_down_menu_new(CTK_DROP_DOWN_MENU_FLAG_COMBO);
    gtk_box_pack_start(GTK_BOX(hbox), menu, TRUE, TRUE, 0);

    dialog->preview_changed_signal =
        g_signal_connect(G_OBJECT(menu), "changed",
                         G_CALLBACK(save_app_profile_changes_dialog_preview_changed),
                         (gpointer)dialog);

    dialog->preview_button = gtk_button_new_with_label("Show Preview");
    gtk_box_pack_start(GTK_BOX(hbox), dialog->preview_button, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(dialog->preview_button), "clicked",
                     G_CALLBACK(save_app_profile_changes_show_preview_button_clicked),
                     (gpointer)dialog);
    ctk_config_set_tooltip(ctk_app_profile->ctk_config,
                           dialog->preview_button,
                           "This button allows you to toggle previewing the new contents of "
                           "the currently selected configuration file.");

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    dialog->preview_vbox = preview_vbox = gtk_vbox_new(FALSE, 8);

    hbox = gtk_hbox_new(FALSE, 8);

    label = gtk_label_new("Backup filename");
    ctk_config_set_tooltip(ctk_app_profile->ctk_config,
                           label,
                           "This text field contains the filename that nvidia-settings will use "
                           "to back up the currently selected configuration file when saving the "
                           "configuration.");

    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    dialog->preview_backup_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), dialog->preview_backup_entry, TRUE, TRUE, 0);
    gtk_entry_set_editable(GTK_ENTRY(dialog->preview_backup_entry), FALSE);

    gtk_box_pack_start(GTK_BOX(preview_vbox), hbox, FALSE, FALSE, 0);

    scroll_win = gtk_scrolled_window_new(NULL, NULL);
    dialog->preview_text_view = text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_CHAR);
    gtk_container_add(GTK_CONTAINER(scroll_win), text_view);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll_win), GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(preview_vbox), scroll_win, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), preview_vbox, TRUE, TRUE, 0);

    dialog->backup_check_button = check_button =
        gtk_check_button_new_with_label("Back up original files");
    // Enable backups by default
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), TRUE);

    ctk_config_set_tooltip(ctk_app_profile->ctk_config,
                           check_button,
                           "This checkbox determines whether nvidia-settings will attempt to back up "
                           "the original configuration files before saving the new configuration.");

    gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

    alignment = gtk_alignment_new(1.0, 0.5, 0.0, 0.0);
    toolbar = gtk_toolbar_new();
    toolbar_items = get_save_app_profile_changes_toolbar_items(dialog, &num_toolbar_items);
    populate_toolbar(GTK_TOOLBAR(toolbar),
                     toolbar_items,
                     num_toolbar_items,
                     NULL,
                     NULL);
    free(toolbar_items);

    gtk_container_add(GTK_CONTAINER(alignment), toolbar);
    gtk_box_pack_start(GTK_BOX(vbox), alignment, FALSE, FALSE, 0);

    return dialog;
}

static void save_app_profile_changes_dialog_destroy(SaveAppProfileChangesDialog *dialog)
{
    json_decref(dialog->updates);
    ctk_help_data_list_free_full(dialog->help_data);
    free(dialog);
}

static GList *get_update_filenames(json_t *updates)
{
    GList *update_filenames = NULL;
    json_t *update, *update_filename_json;
    gchar *update_filename;
    size_t i, size;
    for (i = 0, size = json_array_size(updates); i < size; i++) {
        update = json_array_get(updates, i);
        update_filename_json = json_object_get(update, "filename");
        update_filename = strdup(json_string_value(update_filename_json));
        update_filenames = g_list_prepend(update_filenames, update_filename);
    }

    return update_filenames;
}

static void add_preview_file(gpointer data, gpointer user_data)
{
    const char *filename = (const char*)data;
    CtkDropDownMenu *menu = (CtkDropDownMenu *)user_data;

    ctk_drop_down_menu_append_item(menu, filename, 0);
}

static void save_app_profile_changes_dialog_load_values(SaveAppProfileChangesDialog *dialog)
{
    GList *update_filenames;

    update_filenames = get_update_filenames(dialog->updates);
    ctk_drop_down_menu_reset(CTK_DROP_DOWN_MENU(dialog->preview_file_menu));
    g_list_foreach(update_filenames, add_preview_file, (gpointer)dialog->preview_file_menu);

    save_app_profile_settings_dialog_load_current_update(dialog);

    string_list_free_full(update_filenames);
}

static void save_app_profile_changes_dialog_show(SaveAppProfileChangesDialog *dialog)
{
    // Temporarily disable the "changed" signal to prevent races between the
    // update below and callbacks which fire when the window opens
    g_signal_handler_block(G_OBJECT(dialog->preview_file_menu),
                           dialog->preview_changed_signal);

    save_app_profile_changes_dialog_load_values(dialog);
    gtk_widget_show_all(dialog->top_window);
    // Hide preview window by default
    save_app_profile_changes_dialog_set_preview_visibility(dialog, dialog->show_preview);

    g_signal_handler_unblock(G_OBJECT(dialog->preview_file_menu),
                             dialog->preview_changed_signal);

    gtk_window_set_transient_for(GTK_WINDOW(dialog->top_window),
                                 GTK_WINDOW(gtk_widget_get_toplevel(dialog->parent)));
    gtk_widget_set_sensitive(dialog->parent, FALSE);
}

static void save_changes_callback(GtkWidget *widget, gpointer user_data)
{
    CtkAppProfile *ctk_app_profile = (CtkAppProfile *)user_data;
    SaveAppProfileChangesDialog *dialog = ctk_app_profile->save_app_profile_changes_dialog;
    json_t *updates;

    nv_app_profile_config_check_backing_files(ctk_app_profile->cur_config);

    updates = nv_app_profile_config_validate(ctk_app_profile->cur_config,
                                             ctk_app_profile->gold_config);

    if (json_array_size(updates)) {
        dialog->updates = updates;
        save_app_profile_changes_dialog_show(dialog);
    }
}

static const char __enabling_application_profiles_help[] =
    "Application profile support can be toggled by clicking on the \"Enable application profiles\" "
    "checkbox. Note that changes to this setting will not be saved to disk until the \"Save Changes\" "
    "button is clicked.";
static const char __rules_page_help[] =
    "The Rules page allows you to specify rules for assigning profiles to applications.";
static const char __rules_page_extended_help[] =
    "Rules are presented in a list sorted by priority; higher-priority items appear farther "
    "up in the list and have a smaller priority number. Dragging and dropping a rule in this list "
    "reorders it (potentially modifying its source file; see below), and double-clicking on a "
    "given rule will open a dialog box which lets the user edit the rule (see the \"Add/Edit Rule "
    "Dialog Box\" help section for more information). A rule can be deleted by highlighting it in "
    "the view and hitting the Delete key.\n\n"
    "Note that changes made to rules in this page are not saved to disk until the \"Save Changes\" "
    "button is clicked.";
static const char __profiles_page_help[] =
    "The Profiles page allows you to create and modify profiles in the configuration.";
static const char __profiles_page_extended_help[] =
    "Profiles are presented in a list which can be sorted by profile name, profile settings, and "
    "originating source file. Double-clicking on a profile will open a dialog box which lets the user "
    "edit the rule (see the \"Add/Edit Profile Dialog Box\" help section for more information). A "
    "profile can be deleted by highlighting it in the view and hitting the Delete key.\n\n"
    "Note that changes made to profiles in this page are not saved to disk until the \"Save Changes\" "
    "button is clicked.";


GtkTextBuffer *ctk_app_profile_create_help(CtkAppProfile *ctk_app_profile, GtkTextTagTable *table)
{
    size_t j;
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);
    ctk_help_title(b, &i, "Application Profiles Help");

    ctk_help_para(b, &i, "Use this page to configure application profiles for "
                         "use with the NVIDIA® Linux Graphics Driver. Application profiles "
                         "are collections of settings that are applied on a per-process basis. "
                         "When the driver is loaded into the process, it detects various attributes "
                         "of the running process and determines whether settings should be applied "
                         "based on these attributes. This mechanism allows users to selectively override "
                         "driver settings for a particular application without the need to set environment "
                         "variables on the command line prior to running the application.");
    ctk_help_para(b, &i, "Application profile configuration consists of \"rules\" and \"profiles\". A \"profile\" defines "
                         "what settings to use, and a \"rule\" identifies an application and defines what profile "
                         "should be used with that application.");

    ctk_help_para(b, &i, "A rule identifies an application by describing various features of the application; for example, "
                         "the name of the application binary (e.g. \"glxgears\") or a shared library loaded into the application "
                         "(e.g. \"libpthread.so.0\"). The particular features supported by this NVIDIA® Linux implementation "
                         "are listed below in the \"Supported Features\" section.");

    ctk_help_para(b, &i, "For more information on application profiles, please consult the README.");

    ctk_help_heading(b, &i, "Global Settings");
    ctk_help_para(b, &i, "These settings apply to all profiles and rules within the configuration. ");

    ctk_help_data_list_print_terms(b, &i, ctk_app_profile->global_settings_help_data);

    ctk_help_heading(b, &i, "Rules Page");
    ctk_help_para(b, &i, __rules_page_help);
    ctk_help_para(b, &i, __rules_page_extended_help);

    ctk_help_para(b, &i, "There are several buttons above the list of rules "
                         "which can be used to modify the configuration:");
    ctk_help_data_list_print_terms(b, &i, ctk_app_profile->rules_help_data);

    ctk_help_heading(b, &i, "Rule Properties");
    ctk_help_para(b, &i,  "Each row in the list of rules is divided into several "
                          "columns which describe different properties of a rule: ");
    ctk_help_data_list_print_terms(b, &i, ctk_app_profile->rules_columns_help_data);

    ctk_help_heading(b, &i, "Add/Edit Rule Dialog Box");
    ctk_help_para(b, &i, "When adding a new rule or editing an existing rule, nvidia-settings "
                         "opens a dialog box for you to modify the rule's attributes. ");
    ctk_help_data_list_print_terms(b, &i, ctk_app_profile->edit_rule_dialog->help_data);

    ctk_help_heading(b, &i, "Profiles Page");
    ctk_help_para(b, &i, __profiles_page_help);
    ctk_help_para(b, &i, __profiles_page_extended_help);
    ctk_help_para(b, &i, "There are several buttons above the list of profiles "
                         "which can be used to modify the configuration:");
    ctk_help_data_list_print_terms(b, &i, ctk_app_profile->profiles_help_data);

    ctk_help_heading(b, &i, "Profile Properties");
    ctk_help_para(b, &i,  "Each row in the list of profiles is divided into several "
                          "columns which describe different properties of a profile:");
    ctk_help_data_list_print_terms(b, &i, ctk_app_profile->profiles_columns_help_data);

    ctk_help_heading(b, &i, "Add/Edit Profile Dialog Box");
    ctk_help_para(b, &i, "When adding a new profile or editing an existing profile, nvidia-settings "
                         "opens a dialog box for you to modify the profile's attributes. "
                         "See \"Editing Settings in a Profile\" for information on editing settings.");
    ctk_help_data_list_print_terms(b, &i, ctk_app_profile->edit_profile_dialog->top_help_data);
    ctk_help_data_list_print_terms(b, &i, ctk_app_profile->edit_profile_dialog->bottom_help_data);

    ctk_help_heading(b, &i, "Editing Settings in a Profile");
    ctk_help_para(b, &i, "Settings in a profile are presented in a list view with the following columns: ");
    ctk_help_data_list_print_terms(b, &i, ctk_app_profile->edit_profile_dialog->setting_column_help_data);

    ctk_help_para(b, &i, "Settings can be modified using the following toolbar buttons: ");
    ctk_help_data_list_print_terms(b, &i, ctk_app_profile->edit_profile_dialog->setting_toolbar_help_data);

    ctk_help_heading(b, &i, "Saving and Reverting Changes");

    ctk_help_para(b, &i, "Changes made to the application profile configuration will not take effect until "
                         "they are saved to disk. Buttons to save and restore the configuration "
                         "are located on the bottom of the Application Profiles page.");
    ctk_help_data_list_print_terms(b, &i, ctk_app_profile->save_reload_help_data);

    ctk_help_heading(b, &i, "Supported Features");

    ctk_help_para(b, &i, "This NVIDIA® Linux Graphics Driver supports detection of the following features:");

    for (j = 0; j < NUM_RULE_FEATURES; j++) {
        ctk_help_term(b, &i, rule_feature_label_strings[j]);
        ctk_help_para(b, &i, rule_feature_help_text[j]);
    }

    ctk_help_heading(b, &i, "Supported Setting Keys");

    ctk_help_para(b, &i, "This NVIDIA® Linux Graphics Driver supports the following application profile setting "
                         "keys. For more information on a given key, please consult the README.");

    for (j = 0; j < NUM_PROFILE_SETTINGS; j++) {
        ctk_help_term(b, &i, profile_setting_keys[j]);
        ctk_help_para(b, &i, profile_setting_descriptions[j]);
    }

    ctk_help_finish(b);

    return b;
}

static void enabled_check_button_toggled(GtkToggleButton *toggle_button,
                                         gpointer user_data)
{
    CtkAppProfile *ctk_app_profile = (CtkAppProfile *)user_data;

    nv_app_profile_config_set_enabled(ctk_app_profile->cur_config,
                                      gtk_toggle_button_get_active(toggle_button));

    ctk_config_statusbar_message(ctk_app_profile->ctk_config,
                                 "Application profiles are %s. %s",
                                 gtk_toggle_button_get_active(toggle_button) ?
                                 "enabled" : "disabled",
                                 STATUSBAR_UPDATE_WARNING);
}

GtkWidget* ctk_app_profile_new(CtkConfig *ctk_config)
{
    GObject *object;
    CtkAppProfile *ctk_app_profile;
    GtkWidget *banner;
    GtkWidget *hseparator;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *alignment;
    GtkWidget *notebook;
    GtkWidget *rules_page, *profiles_page;
    GtkWidget *toolbar;

    char *global_config_file;
    char **search_path;
    size_t search_path_size;
    ToolbarItemTemplate *save_reload_toolbar_items;
    size_t num_save_reload_toolbar_items;


    /* Create the CtkAppProfile object */
    object = g_object_new(CTK_TYPE_APP_PROFILE, NULL);

    ctk_app_profile = CTK_APP_PROFILE(object);
    ctk_app_profile->ctk_config = ctk_config;

    gtk_box_set_spacing(GTK_BOX(ctk_app_profile), 10);

    /* Load app profile settings */
    // TODO only load this if the page is exposed
    search_path = get_default_search_path(&search_path_size);
    global_config_file = get_default_global_config_file();
    ctk_app_profile->gold_config = nv_app_profile_config_load(global_config_file,
                                                              search_path,
                                                              search_path_size);
    ctk_app_profile->cur_config = nv_app_profile_config_dup(ctk_app_profile->gold_config);
    free_search_path(search_path, search_path_size);
    free(global_config_file);

    ctk_app_profile->apc_profile_model = ctk_apc_profile_model_new(ctk_app_profile->cur_config);
    ctk_app_profile->apc_rule_model = ctk_apc_rule_model_new(ctk_app_profile->cur_config);

    /* Create the banner */
    banner = ctk_banner_image_new(BANNER_ARTWORK_CONFIG);
    gtk_box_pack_start(GTK_BOX(ctk_app_profile), banner, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ctk_app_profile), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("Application Profiles");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);

    ctk_app_profile->enable_check_button =
        gtk_check_button_new_with_label("Enable application profiles");
    gtk_box_pack_start(GTK_BOX(ctk_app_profile),
                       ctk_app_profile->enable_check_button,
                       FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(ctk_app_profile->enable_check_button), "toggled",
                     G_CALLBACK(enabled_check_button_toggled),
                     (gpointer)ctk_app_profile);


    ctk_app_profile->global_settings_help_data = NULL;
    ctk_config_set_tooltip_and_add_help_data(ctk_app_profile->ctk_config,
                                             ctk_app_profile->enable_check_button,
                                             &ctk_app_profile->global_settings_help_data,
                                             "Enabling Application Profiles",
                                             __enabling_application_profiles_help,
                                             NULL);

    app_profile_load_global_settings(ctk_app_profile,
                                     ctk_app_profile->cur_config);

    // XXX add a search box?

    /* Create the primary notebook for rule/profile config */
    ctk_app_profile->notebook = notebook = gtk_notebook_new();

    /* Build the rules page */
    rules_page = create_rules_page(ctk_app_profile);
    label = gtk_label_new("Rules");

    ctk_config_set_tooltip(ctk_app_profile->ctk_config, label, __rules_page_help);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), rules_page, label);

    /* Build the profiles page */
    profiles_page = create_profiles_page(ctk_app_profile);
    label = gtk_label_new("Profiles");

    ctk_config_set_tooltip(ctk_app_profile->ctk_config, label, __profiles_page_help);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), profiles_page, label);

    /* Add the notebook to the main container */
    gtk_box_pack_start(GTK_BOX(ctk_app_profile), notebook, TRUE, TRUE, 0);

    /* Create the save and restore buttons */
    alignment = gtk_alignment_new(1.0, 0.5, 0.0, 0.0);
    toolbar = gtk_toolbar_new();
    save_reload_toolbar_items = get_save_reload_toolbar_items(ctk_app_profile, &num_save_reload_toolbar_items);
    populate_toolbar(GTK_TOOLBAR(toolbar),
                     save_reload_toolbar_items,
                     num_save_reload_toolbar_items,
                     &ctk_app_profile->save_reload_help_data,
                     NULL);
    free(save_reload_toolbar_items);

    gtk_container_add(GTK_CONTAINER(alignment), toolbar);
    gtk_box_pack_start(GTK_BOX(ctk_app_profile), alignment, FALSE, FALSE, 0);

    gtk_widget_show_all(GTK_WIDGET(ctk_app_profile));

    /* Create edit profile/rule window */
    ctk_app_profile->edit_rule_dialog = edit_rule_dialog_new(ctk_app_profile);
    ctk_app_profile->edit_profile_dialog = edit_profile_dialog_new(ctk_app_profile);
    ctk_app_profile->save_app_profile_changes_dialog = save_app_profile_changes_dialog_new(ctk_app_profile);

    return GTK_WIDGET(ctk_app_profile);
}
