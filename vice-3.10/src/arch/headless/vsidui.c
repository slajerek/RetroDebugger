/** \file   vsidui.c
 * \brief   Headless VSID UI
 *
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"

#include <stdio.h>

#include "ui.h"
#include "vicii.h"
#include "hvsc.h"
#include "vsidui.h"


void vsid_ui_close(void)
{
    /* printf("%s\n", __func__); */

    hvsc_exit();
}


/** \brief  Display tune author in the UI
 *
 * \param[in]   author  author name
 */
void vsid_ui_display_author(const char *author)
{
    /* printf("%s\n", __func__); */
}


/** \brief  Display tune copyright info in the UI
 *
 * \param[in]   copright    copyright info
 */
void vsid_ui_display_copyright(const char *copyright)
{
    /* printf("%s\n", __func__); */
}


/** \brief  Set IRQ type for the UI
 *
 * \param[in]   irq IRQ type
 */
void vsid_ui_display_irqtype(const char *irq)
{
    /* printf("%s\n", __func__); */
}


/** \brief  Display tune name in the UI
 *
 * \param[in]   name    tune name
 */
void vsid_ui_display_name(const char *name)
{
    /* printf("%s\n", __func__); */
}


/** \brief  Set number of tunes for the UI
 *
 * \param[in]   count   number of tunes
 */
void vsid_ui_display_nr_of_tunes(int count)
{
    /* printf("%s\n", __func__); */
}


/** \brief  Set SID model for the UI
 *
 * \param[in]   model   SID model
 */
void vsid_ui_display_sid_model(int model)
{
    /* printf("%s\n", __func__); */
}


/** \brief  Set sync factor for the UI
 *
 * \param[in]   sync    sync factor
 */
void vsid_ui_display_sync(int sync)
{
    /* printf("%s\n", __func__); */
}


/** \brief  Set run time of tune in the UI
 *
 * \param[in]   sec seconds of play time
 */
void vsid_ui_display_time(unsigned int dsec)
{
    /* printf("%s\n", __func__); */
}


/** \brief  Set current tune number in the UI
 *
 * \param[in]   nr  tune number
 */
void vsid_ui_display_tune_nr(int nr)
{
    /* printf("%s\n", __func__); */
}


/** \brief  Set driver info text for the UI
 *
 * \param[in]   driver_info_text    text with driver info (duh)
 */
void vsid_ui_setdrv(char *driver_info_text)
{
    /* printf("%s\n", __func__); */
}


/** \brief  Set default tune number in the UI
 *
 * \param[in]   nr  tune number
 */
void vsid_ui_set_default_tune(int nr)
{
    /* printf("%s\n", __func__); */
}


/** \brief  Set driver address
 *
 * \param[in]   addr    driver address
 */
void vsid_ui_set_driver_addr(uint16_t addr)
{
    /* printf("%s\n", __func__); */
}


/** \brief  Set load address
 *
 * \param[in]   addr    load address
 */
void vsid_ui_set_load_addr(uint16_t addr)
{
    /* printf("%s\n", __func__); */
}


/** \brief  Set init routine address
 *
 * \param[in]   addr    init routine address
 */
void vsid_ui_set_init_addr(uint16_t addr)
{
    /* printf("%s\n", __func__); */
}


/** \brief  Set play routine address
 *
 * \param[in]   addr    play routine address
 */
void vsid_ui_set_play_addr(uint16_t addr)
{
    /* printf("%s\n", __func__); */
}


/** \brief  Set size of SID on actual machine
 *
 * \param[in]   size    size of SID
 */
void vsid_ui_set_data_size(uint16_t size)
{
    /* printf("%s\n", __func__); */
}


#if 0
/** \brief  Identify the canvas used to create a window
 *
 * \return  window index on success, -1 on failure
 */
static int identify_canvas(video_canvas_t *canvas)
{
    /* printf("%s\n", __func__); */

    if (canvas != vicii_get_canvas()) {
        return -1;
    }
    return PRIMARY_WINDOW;
}
#endif


/** \brief  Initialize the VSID UI
 *
 * \return  0 on success, -1 on failure
 */
int vsid_ui_init(void)
{
    /* printf("%s\n", __func__); */

    return 0;
}
