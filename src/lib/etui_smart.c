/* Etui - Multi-document rendering library using the EFL
 * Copyright (C) 2013 Vincent Torri
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Eina.h>
#include <Evas.h>
#include <Efreet_Mime.h>

#include "Etui.h"
#include "etui_private.h"
#include "etui_module.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

/**
 * @cond LOCAL
 */

typedef struct Etui_Smart_Data_ Etui_Smart_Data;

struct Etui_Smart_Data_
{
    EINA_REFCOUNT;

    /* properties */
    Etui_Provider_Instance *provider_instance;
    char *filename;

    /* private */
    Evas_Object *obj;
};

static Evas_Smart *_etui_smart = NULL;

/* internal smart object routines */

#define ETUI_SMART_OBJ_GET(smart, o, type)             \
{                                                      \
    char *_etui_smart_str;                             \
                                                       \
    if (!o) return;                                    \
    smart = evas_object_smart_data_get(o);             \
    if (!smart) return;                                \
    _etui_smart_str = (char *)evas_object_type_get(o); \
    if (!_etui_smart_str) return;                      \
    if (strcmp(_etui_smart_str, type)) return;         \
}

#define ETUI_SMART_OBJ_GET_RETURN(smart, o, type, ret) \
{                                                      \
    char *_etui_smart_str;                             \
                                                       \
    if (!o) return ret;                                \
    smart = evas_object_smart_data_get(o);             \
    if (!smart) return ret;                            \
    _etui_smart_str = (char *)evas_object_type_get(o); \
    if (!_etui_smart_str) return ret;                  \
    if (strcmp(_etui_smart_str, type)) return ret;     \
}

#define ETUI_OBJ_NAME "etui_object"

/* static void */
/* _mouse_move(void *data, Evas *ev EINA_UNUSED, Evas_Object *obj, void *event_info) */
/* { */
/*     Evas_Event_Mouse_Move *e; */
/*     Etui_Smart_Data *sd; */
/*     int x, y, iw, ih; */
/*     Evas_Coord ox, oy, ow, oh; */

/*     e = event_info; */
/*     sd = data; */
/*     evas_object_geometry_get(obj, &ox, &oy, &ow, &oh); */
/*     evas_object_image_size_get(obj, &iw, &ih); */
/*     if ((iw < 1) || (ih < 1)) return; */
/*     x = (((int)e->cur.canvas.x - ox) * iw) / ow; */
/*     y = (((int)e->cur.canvas.y - oy) * ih) / oh; */
/*     emotion_engine_instance_event_mouse_move_feed(sd->engine_instance, x, y); */
/* } */

/* static void */
/* _mouse_down(void *data, Evas *ev EINA_UNUSED, Evas_Object *obj, void *event_info) */
/* { */
/*     Evas_Event_Mouse_Down *e; */
/*     Etui_Smart_Data *sd; */
/*     int x, y, iw, ih; */
/*     Evas_Coord ox, oy, ow, oh; */

/*     e = event_info; */
/*     sd = data; */
/*     evas_object_geometry_get(obj, &ox, &oy, &ow, &oh); */
/*     evas_object_image_size_get(obj, &iw, &ih); */
/*     if ((iw < 1) || (ih < 1)) return; */
/*     x = (((int)e->canvas.x - ox) * iw) / ow; */
/*     y = (((int)e->canvas.y - oy) * ih) / oh; */
/*     emotion_engine_instance_event_mouse_button_feed(sd->engine_instance, 1, x, y); */
/* } */

static void
_etui_smart_add(Evas_Object * obj)
{
    Etui_Smart_Data *sd;

    sd = calloc(1, sizeof(Etui_Smart_Data));
    if (!sd) return;

    EINA_REFCOUNT_INIT(sd);

/*     evas_object_event_callback_add(sd->obj, EVAS_CALLBACK_MOUSE_MOVE, _etui_mouse_move, sd); */
/*     evas_object_event_callback_add(sd->obj, EVAS_CALLBACK_MOUSE_DOWN, _etui_mouse_down, sd); */
    evas_object_smart_data_set(obj, sd);
}

static void
_etui_smart_del(Evas_Object * obj)
{
    Etui_Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd) return;

    EINA_REFCOUNT_UNREF(sd)
    {
        if (sd->provider_instance)
        {
            etui_provider_instance_file_close(sd->provider_instance);
            etui_provider_instance_del(sd->provider_instance);
            sd->provider_instance = NULL;
        }

        if (sd->obj)
            evas_object_del(sd->obj);
        if (sd->filename)
            free(sd->filename);
        free(sd);
    }
}

static void
_etui_smart_move(Evas_Object * obj, Evas_Coord x, Evas_Coord y)
{
    Etui_Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd) return;

    evas_object_move(sd->obj, x, y);
}

static void
_etui_smart_resize(Evas_Object * obj, Evas_Coord w, Evas_Coord h)
{
    Etui_Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd) return;

    /* FIXME: not always image object */
    evas_object_image_fill_set(sd->obj, 0, 0, w, h);
    evas_object_resize(sd->obj, w, h);
}

static void
_etui_smart_show(Evas_Object * obj)
{
    Etui_Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd) return;

    evas_object_show(sd->obj);

}

static void
_etui_smart_hide(Evas_Object * obj)
{
    Etui_Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd) return;

    evas_object_hide(sd->obj);
}

static void
_etui_smart_color_set(Evas_Object * obj, int r, int g, int b, int a)
{
    Etui_Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd) return;

    evas_object_color_set(sd->obj, r, g, b, a);
}

static void
_etui_smart_clip_set(Evas_Object * obj, Evas_Object * clip)
{
    Etui_Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd) return;

    evas_object_clip_set(sd->obj, clip);
}

static void
_etui_smart_clip_unset(Evas_Object * obj)
{
    Etui_Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd) return;

    evas_object_clip_unset(sd->obj);
}

static void
_etui_smart_init(void)
{
    static Evas_Smart_Class sc = EVAS_SMART_CLASS_INIT_NAME_VERSION(ETUI_OBJ_NAME);

    if (_etui_smart) return;

    if (!sc.add)
    {
        sc.add = _etui_smart_add;
        sc.del = _etui_smart_del;
        sc.move = _etui_smart_move;
        sc.resize = _etui_smart_resize;
        sc.show = _etui_smart_show;
        sc.hide = _etui_smart_hide;
        sc.color_set = _etui_smart_color_set;
        sc.clip_set = _etui_smart_clip_set;
        sc.clip_unset = _etui_smart_clip_unset;
/*         sc.callbacks = _etui_smart_callbacks; */
/*         sc.calculate = _etui_smart_calculate; */
    }
    _etui_smart = evas_smart_class_new(&sc);
}

/**
 * @endcond
 */


/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

EAPI Evas_Object *
etui_object_add(Evas *evas)
{
    _etui_smart_init();
    return evas_object_smart_add(evas, _etui_smart);
}

EAPI Eina_Bool
etui_object_file_set(Evas_Object *obj, const char *filename)
{
    char file[PATH_MAX];
    char *res;
    const char *mime;
    const char *module_name = NULL;
    Etui_Smart_Data *sd;

    if (!filename || !*filename)
        return EINA_FALSE;

    ETUI_SMART_OBJ_GET_RETURN(sd, obj, ETUI_OBJ_NAME, EINA_FALSE);

    res = realpath(filename, file);
    if (!res)
        return EINA_FALSE;
    if (sd->filename && (!strcmp(file, sd->filename)))
        return EINA_TRUE;

    mime = efreet_mime_type_get(file);
    if (mime)
    {
        if (strcmp(mime, "application/pdf") == 0)
            module_name = "pdf";
        else if (strcmp(mime, "application/postscript") == 0)
            module_name = "ps";
        else if (strcmp(mime, "text/plain") == 0)
            module_name = "txt";
        else if ((strcmp(mime, "application/x-cba") == 0) ||
                 (strcmp(mime, "application/x-cbr") == 0) ||
                 (strcmp(mime, "application/x-cbt") == 0) ||
                 (strcmp(mime, "application/x-cbz") == 0) ||
                 (strcmp(mime, "application/x-cb7") == 0) ||
                 (strcmp(mime, "image/bmp") == 0) ||
                 (strcmp(mime, "image/gif") == 0) ||
                 (strcmp(mime, "image/jpeg") == 0) ||
                 (strcmp(mime, "image/pjpeg") == 0) ||
                 (strcmp(mime, "image/png") == 0) ||
                 (strcmp(mime, "image/x-png") == 0) ||
                 (strcmp(mime, "image/x-portable-pixmap") == 0) || /* ppm */
                 (strcmp(mime, "image/svg+xml") == 0) ||
                 (strcmp(mime, "image/x-tga") == 0) ||
                 (strcmp(mime, "image/tiff") == 0) ||
                 (strcmp(mime, "image/x-xpixmap") == 0)) /* xpm */
            module_name = "img";
    }

    /* TODO : finish... */

    if (etui_provider_instance_name_equal(sd->provider_instance, module_name))
    {
        DBG("no need to reset module, already set: %s", module_name);
    }
    else
    {
        if (sd->provider_instance)
            etui_provider_instance_del(sd->provider_instance);
        sd->provider_instance = etui_provider_instance_new(module_name, evas_object_evas_get(obj));
    }

    if (!sd->provider_instance)
    {
        ERR("can not find a suitable instace (requested: %s)", module_name);
        return EINA_FALSE;
    }

    /* TODO: use stringshare ?? */
    sd->filename = strdup(file);
    if (!etui_provider_instance_file_open(sd->provider_instance, sd->filename))
    {
        WRN("Couldn't open file=%s", sd->filename);
        return EINA_FALSE;
    }

    return EINA_TRUE;
}

EAPI const char *
etui_object_filename_get(Evas_Object *obj)
{
   Etui_Smart_Data *sd;

   ETUI_SMART_OBJ_GET_RETURN(sd, obj, ETUI_OBJ_NAME, NULL);

   return sd->filename;
}

EAPI Eina_Bool
etui_object_document_password_needed(Evas_Object *obj)
{
   Etui_Smart_Data *sd;

   ETUI_SMART_OBJ_GET_RETURN(sd, obj, ETUI_OBJ_NAME, EINA_FALSE);

   return etui_provider_instance_password_needed(sd->provider_instance);
}

EAPI Eina_Bool
etui_object_document_password_set(Evas_Object *obj, const char *password)
{
   Etui_Smart_Data *sd;

   ETUI_SMART_OBJ_GET_RETURN(sd, obj, ETUI_OBJ_NAME, EINA_FALSE);

   return etui_provider_instance_password_set(sd->provider_instance, password);
}

EAPI int
etui_object_document_pages_count(Evas_Object *obj)
{
   Etui_Smart_Data *sd;

   ETUI_SMART_OBJ_GET_RETURN(sd, obj, ETUI_OBJ_NAME, -1);

   return etui_provider_instance_pages_count(sd->provider_instance);
}