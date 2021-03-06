/* This file is part of rev-lister
 *
 * AUTHORS
 *     Sven Herzberg  <herzi@gnome-de.org>
 *
 * Copyright (C) 2007  Sven Herzberg
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <glib.h>
#include <glib/gi18n.h>

#define _XOPEN_SOURCE
#include <time.h>

static void
print_revs (gchar* key,
	    gpointer values,
	    gpointer unused /* max_values */)
{
	// FIXME: provide proper ascii art bars: #+-
	g_print ("%s: %d\n",
		 key,
		 GPOINTER_TO_SIZE (values));
}

int
main (int   argc,
      char**argv)
{
        gchar* author = NULL;
        gchar* since = NULL;
        GOptionContext* parser;
        GOptionEntry  entries[] =
          {
            {"author", 'a', 0, G_OPTION_ARG_STRING, &author, N_("only count commits from AUTHOR"), N_("AUTHOR")},
            {"since", 's', 0, G_OPTION_ARG_STRING, &since, N_("only count commits starting at DATE"), N_("DATE")},
            {NULL}
          };
	GError* error  = NULL;
	gchar * out    = NULL;
	gint    status = 0;
	gchar **lines  = NULL;
	gchar **iter;
        GPtrArray* spawn_argv = g_ptr_array_new ();

        parser = g_option_context_new ("");
        g_option_context_add_main_entries (parser, entries, NULL);
        if (!g_option_context_parse (parser, &argc, &argv, &error))
          {
            g_printerr ("Usage: %s\n%s\n",
                        argv[0],
                        error->message);
            g_error_free (error);

            return 1;
          }

        g_ptr_array_add (spawn_argv, "git");
        g_ptr_array_add (spawn_argv, "rev-list");
        g_ptr_array_add (spawn_argv, "--pretty=format:%ai");

        if (since)
          {
            g_ptr_array_add (spawn_argv, g_strdup_printf ("--since=%s", since));
          }
        if (author)
          {
            g_ptr_array_add (spawn_argv, g_strdup_printf ("--author=%s", author));
          }

        g_ptr_array_add (spawn_argv, "--all");
        g_ptr_array_add (spawn_argv, NULL);

        g_spawn_sync (g_get_current_dir (),
                      (gchar**) spawn_argv->pdata, NULL,
                      G_SPAWN_SEARCH_PATH, NULL, NULL,
                      &out, NULL,
                      &status, &error);

	if (error) {
		g_warning ("Error executing 'git rev-list': %s",
			   error->message);
		g_error_free (error);
		g_free (out);
		return 1;
	};

	if (status != 0) {
		g_warning ("git rev-list didn't return 0");
		g_free (out);
		return 2;
	}

	GHashTable* revs = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	lines = g_strsplit (out, "\n", -1);
	g_free (out);
	for (iter = lines; iter && *iter; iter++) {
		if (G_LIKELY (**iter)) {
			if (!g_str_has_prefix (*iter, "commit ")) {
				gchar** words = g_strsplit (*iter, " ", 2);
				gsize count = GPOINTER_TO_SIZE (g_hash_table_lookup (revs, words[0]));
				count++;
				g_hash_table_insert (revs, g_strdup (words[0]), GSIZE_TO_POINTER (count));
				g_strfreev (words);
			}
		}
	}
	g_strfreev (lines);
	g_hash_table_foreach (revs, (GHFunc)print_revs, NULL);

	return 0;
}

/* vim:set et: */
