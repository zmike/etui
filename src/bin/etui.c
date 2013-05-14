/* Etui - Multi-document rendering library using the EFL
 * Copyright (C) 2013 Vincent Torri
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library;
 * if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Eina.h>
#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Getopt.h>
#include <Ecore_Evas.h>

#include <Etui.h>

static const Ecore_Getopt _etui_options =
{
    "etui",
    "%prog [options] <filename>",
    PACKAGE_VERSION,
    "(C) 2013 Vincent Torri",
    "AGPL",
    "EFL-based multi-document viewer.",
    1,
    {
        ECORE_GETOPT_STORE_STR('e', "engine", "ecore-evas engine to use"),
        ECORE_GETOPT_CALLBACK_NOARGS('E', "list-engines", "list ecore-evas engines",
                                     ecore_getopt_callback_ecore_evas_list_engines, NULL),
        ECORE_GETOPT_CALLBACK_ARGS('g', "geometry", "geometry to use in x:y:w:h form.", "X:Y:W:H",
                                   ecore_getopt_callback_geometry_parse, NULL),
        ECORE_GETOPT_VERSION('V', "version"),
        ECORE_GETOPT_COPYRIGHT('R', "copyright"),
        ECORE_GETOPT_LICENSE('L', "license"),
        ECORE_GETOPT_HELP('h', "help"),
        ECORE_GETOPT_SENTINEL
    }
};

static Eina_Bool
_etui_signal_exit(void *data EINA_UNUSED, int ev_type EINA_UNUSED, void *ev EINA_UNUSED)
{
    printf("exiting signal...\n");
    ecore_main_loop_quit();
    return EINA_TRUE;
}

static void _etui_delete_request_cb(Ecore_Evas *ee EINA_UNUSED)
{
    ecore_main_loop_quit();
}

int main(int argc, char *argv[])
{
    char buf[4096];
    Eina_Rectangle geometry = {0, 0, 480, 640};
    char *engine = NULL;
    unsigned char engines_listed = 0;
    unsigned char help = 0;
    Ecore_Getopt_Value values[] = {
        ECORE_GETOPT_VALUE_STR(engine),
        ECORE_GETOPT_VALUE_BOOL(engines_listed),
        ECORE_GETOPT_VALUE_PTR_CAST(geometry),
        ECORE_GETOPT_VALUE_BOOL(help),
        ECORE_GETOPT_VALUE_BOOL(help),
        ECORE_GETOPT_VALUE_BOOL(help),
        ECORE_GETOPT_VALUE_BOOL(help),
        ECORE_GETOPT_VALUE_NONE
    };
    Ecore_Evas *ee;
    Evas *evas;
    Evas_Object *o;
    int args;

    if (!ecore_evas_init())
    {
        printf("can not init ecore_evas\n");
        return -1;
    }

    ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, _etui_signal_exit, NULL);

    ecore_app_args_set(argc, (const char **)argv);
    args = ecore_getopt_parse(&_etui_options, values, argc, argv);
    if (args < 0)
        goto shutdown_ecore_evas;
    else if (help)
        goto shutdown_ecore_evas;
    else if (engines_listed)
        goto shutdown_ecore_evas;
    else if (args == argc)
    {
        printf("must provide at least one file to display\n");
        goto shutdown_ecore_evas;
    }

    if ((geometry.w == 0) || (geometry.h == 0))
    {
        if (geometry.w == 0) geometry.w = 480;
        if (geometry.h == 0) geometry.h = 640;
    }

    ee = ecore_evas_new(NULL, geometry.x, geometry.y, 0, 0, NULL);
    if (!ee)
    {
        printf("can not create Ecore_Evas\n");
        goto shutdown_ecore_evas;
    }

    ecore_evas_callback_delete_request_set(ee, _etui_delete_request_cb);
    evas = ecore_evas_get(ee);

    snprintf(buf, sizeof(buf), "Etui - %s\n", argv[args]);
    ecore_evas_title_set(ee, buf);
    ecore_evas_name_class_set(ee, "etui", "main");

    if (!etui_init())
        goto shutdown_ecore_evas;

    o = etui_object_add(evas);
    if (!etui_object_file_set(o, argv[args]))
    {
        printf("can not open file %s\n", argv[args]);
        goto shutdown_ecore_evas;
    }
    evas_object_show(o);

    printf("pages : %d\n", etui_object_document_pages_count(o));

    ecore_evas_resize(ee, geometry.w, geometry.h);
    ecore_evas_show(ee);

    ecore_main_loop_begin();

    etui_shutdown();
    ecore_evas_shutdown();

    return 0;

  shutdown_ecore_evas:
    ecore_evas_shutdown();

    return -1;
}