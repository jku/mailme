/*
 * Copyright (C) 2010-2011 Intel Corporation.
 *
 * Author: Nicolas Dufresne <nicolas.dufresne@collabora.co.uk>
 * Author: Jussi Kukkonen <jku@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */


#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <libnotify/notify.h>

#include "config.h"

#include "mailme-telepathy.h"
#include "mailme-telepathy-account.h"

static void
print_status (MailmeTelepathyAccount *account)
{
  gchar *display_name = NULL;
  guint unread_count = 0;

  g_object_get (account, "display-name", &display_name, NULL);
  g_object_get (account, "unread-count", &unread_count, NULL);

  g_print ("Display name: %s\n", display_name);
  g_print ("Unread count: %u\n", unread_count);

  g_free (display_name);
}

static void
on_tp_provider_prepared (GObject      *source,
                         GAsyncResult *result,
                         gpointer      user_data)
{
  GMainLoop *loop = user_data;
  GError *error = NULL;
  if (!mailme_telepathy_prepare_finish (MAILME_TELEPATHY (source), result,
        &error))
  {
    g_warning ("Failed to prepare Telepathy provider: %s", error->message);
    g_main_loop_quit (loop);
    return;
  }
}

static void update_notification (NotifyNotification *note,
                                 const char         *display_name,
                                 guint               unread_count,
                                 guint               last_unread_count)
{
  GError *error = NULL;

  if (unread_count == 0) {
    if (!notify_notification_close (note, &error)) {
      g_printerr ("Failed to close notification: %s", error->message);
      g_error_free (error);
    }
  } else {
    char *msg;

    /* Translators: Notification text */
    msg = g_strdup_printf (ngettext ("One unread mail on %1$s",
                                     "%2$d unread mails on %1$s",
                                     unread_count),
                           display_name, unread_count);
    notify_notification_update (note, msg, NULL, "mail-unread");

    /* FIXME: at least gnome-shell does not update the title unless
     * we _show(), but we don't really want to do that when unread count
     * is going down... */
    if (unread_count > last_unread_count)
      if (!notify_notification_show (note, &error)) {
        g_printerr ("Failed to show notification: %s", error->message);
        g_error_free (error);
      }
    g_free (msg);
  }
}

static void
on_unread_count_changed (GObject    *object,
                         GParamSpec *pspec,
                         gpointer    user_data)
{
  MailmeTelepathyAccount *account;
  NotifyNotification *note;
  gchar *display_name = NULL;
  guint unread_count = 0;
  guint last_unread_count;

  account = MAILME_TELEPATHY_ACCOUNT (object);
  note = g_object_get_data (object, "notification");

  g_return_if_fail (note != NULL);

  print_status (account);

  g_object_get (account,
                "display-name", &display_name,
                "unread-count", &unread_count,
                NULL);
  last_unread_count = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (account),
                                                           "last-unread-count"));

  update_notification (note, display_name, unread_count, last_unread_count);

  g_object_set_data (G_OBJECT (account), "last-unread-count",
                     GUINT_TO_POINTER(unread_count));
  g_free(display_name);
}

static void
on_get_inbox (GObject      *source,
              GAsyncResult *result,
              gpointer      user_data)
{
  MailmeInboxOpenFormat format;
  gchar *value;
  GError *error = NULL;

  value = mailme_telepathy_account_get_inbox_finish (
                                        MAILME_TELEPATHY_ACCOUNT (source),
                                        result,
                                        &format,
                                        &error);

  if (error)
  {
    g_warning ("Failed to get inbox information: %s", error->message);
    g_error_free (error);
    return;
  }

  switch (format) {
    case MAILME_INBOX_URI:
      if (!g_app_info_launch_default_for_uri (value, NULL, NULL))
        g_warning ("Failed to launch default handler for uri %s", value);
      break;
    default:
      g_warning ("Only inbox uri handling has been implemented");
      break;
  }

  g_free (value);
}

static void
on_open_action (NotifyNotification *notification,
                char *action,
                gpointer user_data)
{
  mailme_telepathy_account_get_inbox_async (MAILME_TELEPATHY_ACCOUNT (user_data),
                                            on_get_inbox,
                                            NULL);
}

static void
on_account_added (MailmeTelepathy        *tp_provider,
                  MailmeTelepathyAccount *account,
                  gpointer                user_data)
{
  /* Translators: button on a "unread mail" notification, will open
     a web page (e.g. gmail.com inbox) or a mail client */
  char *title = _("Open");
  NotifyNotification *notification;

  notification = notify_notification_new ("", NULL, NULL);
  notify_notification_set_category (notification, "email.arrived");

  notify_notification_add_action (notification,
                                  "open", title,
                                  on_open_action,
                                  account, NULL);

  g_object_set_data_full (G_OBJECT (account), "notification",
                          notification, g_object_unref);
  g_object_set_data (G_OBJECT (account), "last-unread-count",
                     GUINT_TO_POINTER(0));
  g_signal_connect (account,
                    "notify::unread-count",
                    G_CALLBACK (on_unread_count_changed),
                    NULL);
}

static void
on_account_removed (MailmeTelepathy        *tp_provider,
                    MailmeTelepathyAccount *account,
                    gpointer                user_data)
{
  NotifyNotification *note;
  GError *error = NULL;

  note = g_object_get_data (G_OBJECT (account), "notification");
    if (!notify_notification_close (note, &error)) {
      g_printerr ("Failed to close notification: %s", error->message);
      g_error_free (error);
    }

  g_signal_handlers_disconnect_by_func (account, on_unread_count_changed, NULL);
}

gint main (gint argc, gchar **argv)
{
  GMainLoop *loop;
  MailmeTelepathy *tp_provider;

  setlocale (LC_ALL, "");
  bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
  textdomain(GETTEXT_PACKAGE);

  g_type_init ();

  /* Translators: 'application title' for the notifications, will be
     shown in e.g. gnome-shell message tray */
  notify_init (_("Unread mail"));

  loop = g_main_loop_new (NULL, FALSE);

  tp_provider = g_object_new (MAILME_TYPE_TELEPATHY, NULL);
  mailme_telepathy_prepare_async (tp_provider,
                                  on_tp_provider_prepared,
                                  loop);

  g_signal_connect (tp_provider,
                    "account-added",
                    G_CALLBACK (on_account_added),
                    NULL);

  g_signal_connect (tp_provider,
                    "account-removed",
                    G_CALLBACK (on_account_removed),
                    NULL);

  g_main_loop_run (loop);

  g_object_unref (tp_provider);
  g_object_unref (loop);
  notify_uninit ();

  return 0;
}
