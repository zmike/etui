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

#include <fitz.h>
#include <mupdf.h>

#include "Etui.h"
#include "etui_module.h"
#include "etui_module_pdf.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

/**
 * @cond LOCAL
 */

#ifdef ETUI_MODULE_PDF_DEFAULT_LOG_COLOR
# undef ETUI_MODULE_PDF_DEFAULT_LOG_COLOR
#endif
#define ETUI_MODULE_PDF_DEFAULT_LOG_COLOR EINA_COLOR_LIGHTCYAN

#ifdef ERR
# undef ERR
#endif
#define ERR(...)  EINA_LOG_DOM_ERR(_etui_module_pdf_log_domain, __VA_ARGS__)

#ifdef DBG
# undef DBG
#endif
#define DBG(...)  EINA_LOG_DOM_DBG(_etui_module_pdf_log_domain, __VA_ARGS__)

#ifdef INF
# undef INF
#endif
#define INF(...)  EINA_LOG_DOM_INFO(_etui_module_pdf_log_domain, __VA_ARGS__)

#ifdef WRN
# undef WRN
#endif
#define WRN(...)  EINA_LOG_DOM_WARN(_etui_module_pdf_log_domain, __VA_ARGS__)

#ifdef CRIT
# undef CRIT
#endif
#define CRIT(...) EINA_LOG_DOM_CRIT(_etui_module_pdf_log_domain, __VA_ARGS__)

typedef struct _Etui_Provider_Data Etui_Provider_Data;

struct _Etui_Provider_Data
{
    /* specific EFL stuff for the module */
    Evas_Object *obj;

    /* specific PDF stuff for the module */

    /* Document */
    struct
    {
        fz_context *ctx;
        pdf_document *doc;
    } doc;

    /* Current page */
    struct
    {
        pdf_page *page;
    } page;
};

static int _etui_module_pdf_init_count = 0;
static int _etui_module_pdf_log_domain = -1;

static void *
_etui_pdf_init(Evas *evas)
{
    Etui_Provider_Data *pd;

    pd = (Etui_Provider_Data *)calloc(1, sizeof(Etui_Provider_Data));
    if (!pd)
        return NULL;

    DBG("init module");

    pd->obj = evas_object_image_add(evas);
    if (!pd->obj)
        goto free_pd;

    /* FIXME:
     * 1st parameter: custom memory allocator ?
     * 2nd parameter: locks/unlocks for multithreading
     */
    pd->doc.ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
    if (!pd->doc.ctx)
    {
        ERR("Could not create context");
        goto del_obj;
    }

    return pd;

  del_obj:
    evas_object_del(pd->obj);
  free_pd:
    free(pd);

    return NULL;
}

static void
_etui_pdf_shutdown(void *d)
{
    Etui_Provider_Data *pd;

    if (!d)
        return;

    DBG("shutdown module");

    pd = (Etui_Provider_Data *)d;
    fz_free_context(pd->doc.ctx);
    evas_object_del(pd->obj);
    free(pd);
}

static Evas_Object *
_etui_pdf_evas_object_get(void *d)
{
    if (!d)
        return NULL;

    return ((Etui_Provider_Data *)d)->obj;
}

static Eina_Bool
_etui_pdf_file_open(void *d, const char *filename)
{
    Etui_Provider_Data *pd;

    if (!d || !filename || !*filename)
        return EINA_FALSE;

    DBG("open file %s", filename);

    pd = (Etui_Provider_Data *)d;

    pd->doc.doc = pdf_open_document(pd->doc.ctx, filename);
    if (!pd->doc.doc)
    {
        ERR("Could not open file %s", filename);
        return EINA_FALSE;
    }

    /* FIXME: get PDF info */

    pd->page.page = pdf_load_page(pd->doc.doc, 0);
    if (!pd->page.page)
    {
        ERR("Could not load page 0");
        goto close_document;
    }

    return EINA_TRUE;

  close_document:
  pdf_close_document(pd->doc.doc);

  return EINA_FALSE;
}

static void
_etui_pdf_file_close(void *d)
{
    Etui_Provider_Data *pd;

    if (!d)
        return;

    pd = (Etui_Provider_Data *)d;

    /* FIXME: ask mupdf devs to see if one can get the filename from a document*/
    //DBG("close file %s", pd->filename);
    printf("close file\n");

    if (pd->doc.doc)
    {
        pdf_close_document(pd->doc.doc);
        pd->doc.doc = NULL;
    }

    if (pd->page.page)
    {
        pdf_free_page(pd->doc.doc, pd->page.page);
        pd->page.page = NULL;
    }
}

static Eina_Bool
_etui_pdf_password_needed(void *d)
{
    Etui_Provider_Data *pd;

    if (!d)
        return EINA_FALSE;

    pd = (Etui_Provider_Data *)d;
    return pdf_needs_password(pd->doc.doc) ? EINA_TRUE : EINA_FALSE;
}

static Eina_Bool
_etui_pdf_password_set(void *d, const char *password)
{
    Etui_Provider_Data *pd;

    if (!d)
        return EINA_FALSE;

    pd = (Etui_Provider_Data *)d;
    return pdf_authenticate_password(pd->doc.doc, (char *)password) ? EINA_TRUE : EINA_FALSE;
}

static int
_etui_pdf_pages_count(void *d)
{
    Etui_Provider_Data *pd;

    if (!d)
        return EINA_FALSE;

    pd = (Etui_Provider_Data *)d;
    return pdf_count_pages(pd->doc.doc);
}

static Etui_Provider_Descriptor _etui_provider_descriptor_pdf =
{
    /* .name            */ "pdf",
    /* .version         */ ETUI_PROVIDER_DESCRIPTOR_VERSION,
    /* .priority        */ ETUI_PROVIDER_DESCRIPTOR_PRIORITY_DEFAULT,
    /* .init            */ _etui_pdf_init,
    /* .shutdown        */ _etui_pdf_shutdown,
    /* .evas_object_get */ _etui_pdf_evas_object_get,
    /* .file_open       */ _etui_pdf_file_open,
    /* .file_close      */ _etui_pdf_file_close,
    /* .password_needed */ _etui_pdf_password_needed,
    /* .password_set    */ _etui_pdf_password_set,
    /* .pages_count     */ _etui_pdf_pages_count
};

/**
 * @endcond
 */


/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

Eina_Bool
etui_module_pdf_init(void)
{
    if (_etui_module_pdf_init_count > 0)
    {
        _etui_module_pdf_init_count++;
        return EINA_TRUE;
    }

    _etui_module_pdf_log_domain = eina_log_domain_register("etui-pdf",
                                                           ETUI_MODULE_PDF_DEFAULT_LOG_COLOR);
    if (_etui_module_pdf_log_domain < 0)
    {
        EINA_LOG_CRIT("Could not register log domain 'etui-pdf'");
        return EINA_FALSE;
    }

    if (!etui_module_register(&_etui_provider_descriptor_pdf))
    {
        ERR("Could not register module %p", &_etui_provider_descriptor_pdf);
        goto unregister_log;
    }

    _etui_module_pdf_init_count = 1;
    return EINA_TRUE;

  unregister_log:
    eina_log_domain_unregister(_etui_module_pdf_log_domain);
    _etui_module_pdf_log_domain = -1;

    return EINA_FALSE;
}

void
etui_module_pdf_shutdown(void)
{
    if (_etui_module_pdf_init_count > 1)
    {
        _etui_module_pdf_init_count--;
        return;
    }
    else if (_etui_module_pdf_init_count == 0)
    {
        ERR("Too many etui_module_pdf_shutdown() calls");
        return;
    }


    DBG("shutdown pdf module");

    _etui_module_pdf_init_count = 0;

    etui_module_unregister(&_etui_provider_descriptor_pdf);

    eina_log_domain_unregister(_etui_module_pdf_log_domain);
    _etui_module_pdf_log_domain = -1;
}

#ifndef ETUI_BUILD_STATIC_PDF

EINA_MODULE_INIT(etui_module_pdf_init);
EINA_MODULE_SHUTDOWN(etui_module_pdf_shutdown);

#endif

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/