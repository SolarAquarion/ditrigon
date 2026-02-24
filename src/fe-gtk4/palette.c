/* SPDX-License_Identifier: GPL-2.0-or-later */
/* GTK4 palette loader/saver */

#include "fe-gtk4.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "../common/util.h"
#include "../common/cfgfiles.h"
#include "palette.h"

HcColor16 colors[] =
{
	/* xtext colors */
	{ 0xe9e9, 0xe9e9, 0xe9e9 }, /* 0 white */
	{ 0x0000, 0x0000, 0x0000 }, /* 1 black */
	{ 0x132e, 0xa2f0, 0xdd19 }, /* 2 blue */
	{ 0x2a3d, 0x8ccc, 0x2a3d }, /* 3 green */
	{ 0xc7c7, 0x3232, 0x3232 }, /* 4 red */
	{ 0xc7c7, 0x3232, 0x3232 }, /* 5 light red */
	{ 0x8000, 0x2666, 0x7fff }, /* 6 purple */
	{ 0x6666, 0x3636, 0x1f1f }, /* 7 orange */
	{ 0xd999, 0xa6d3, 0x4147 }, /* 8 yellow */
	{ 0x3d70, 0xcccc, 0x3d70 }, /* 9 green */
	{ 0x199a, 0x5555, 0x5555 }, /* 10 aqua */
	{ 0x2eef, 0x8ccc, 0x74df }, /* 11 light aqua */
	{ 0x451e, 0x451e, 0xe666 }, /* 12 blue */
	{ 0xb0b0, 0x3737, 0xb0b0 }, /* 13 light purple */
	{ 0x4c4c, 0x4c4c, 0x4c4c }, /* 14 grey */
	{ 0x9595, 0x9595, 0x9595 }, /* 15 light grey */

	{ 0xe9e9, 0xe9e9, 0xe9e9 }, /* 16 white */
	{ 0x0000, 0x0000, 0x0000 }, /* 17 black */
	{ 0x123c, 0x9023, 0xeaaa }, /* 18 blue */
	{ 0x2a3d, 0x8ccc, 0x2a3d }, /* 19 green */
	{ 0xf259, 0x5518, 0x15e8 }, /* 20 red */
	{ 0xf469, 0x62a3, 0x17f9 }, /* 21 light red */
	{ 0x8000, 0x2666, 0x7fff }, /* 22 purple */
	{ 0x8c35, 0x8a8d, 0x8a78 }, /* 23 orange */
	{ 0xd999, 0xa6d3, 0x4147 }, /* 24 yellow */
	{ 0x3d70, 0xcccc, 0x3d70 }, /* 25 green */
	{ 0x199a, 0x5555, 0x5555 }, /* 26 aqua */
	{ 0x2eef, 0x8ccc, 0x74df }, /* 27 light aqua */
	{ 0x451e, 0x451e, 0xe666 }, /* 28 blue */
	{ 0xb0b0, 0x3737, 0xb0b0 }, /* 29 light purple */
	{ 0xffff, 0xffff, 0xffff }, /* 30 grey */
	{ 0x9595, 0x9595, 0x9595 }, /* 31 light grey */

	{ 0xffff, 0xffff, 0xffff }, /* 32 marktext fg */
	{ 0x3535, 0x6e6e, 0xc1c1 }, /* 33 marktext bg */
	{ 0xc7c7, 0xc7c7, 0xc7c7 }, /* 34 foreground */
	{ 0x3333, 0x3333, 0x3333 }, /* 35 background */
	{ 0xf13e, 0xea5e, 0x2a4e }, /* 36 marker line */

	/* GUI colors */
	{ 0x9999, 0x0000, 0x0000 }, /* 37 tab new data */
	{ 0x0000, 0x0000, 0xffff }, /* 38 tab hilight */
	{ 0xffff, 0x0000, 0x0000 }, /* 39 tab new msg */
	{ 0x9595, 0x9595, 0x9595 }, /* 40 away */
	{ 0x9595, 0x9595, 0x9595 }, /* 41 spell checker */
};

void
palette_alloc (GtkWidget *widget)
{
	(void) widget;
	/* GTK4 uses RGBA and CSS styling; no colormap allocation is required. */
}

void
palette_load (void)
{
	int i;
	int j;
	int fh;
	char prefname[256];
	struct stat st;
	char *cfg;
	guint16 red;
	guint16 green;
	guint16 blue;

	fh = hexchat_open_file ("colors.conf", O_RDONLY, 0, 0);
	if (fh == -1)
		return;

	fstat (fh, &st);
	cfg = g_malloc0 (st.st_size + 1);
	if (read (fh, cfg, st.st_size) < 0)
	{
		g_warning ("Failed to read colors.conf");
		g_free (cfg);
		close (fh);
		return;
	}

	/* mIRC colors 0-31 */
	for (i = 0; i < 32; i++)
	{
		g_snprintf (prefname, sizeof prefname, "color_%d", i);
		cfg_get_color (cfg, prefname, &red, &green, &blue);
		colors[i].red = red;
		colors[i].green = green;
		colors[i].blue = blue;
	}

	/* extended colors 256+ -> 32..MAX_COL */
	for (i = 256, j = 32; j < MAX_COL + 1; i++, j++)
	{
		g_snprintf (prefname, sizeof prefname, "color_%d", i);
		cfg_get_color (cfg, prefname, &red, &green, &blue);
		colors[j].red = red;
		colors[j].green = green;
		colors[j].blue = blue;
	}

	g_free (cfg);
	close (fh);
}

void
palette_save (void)
{
	int i;
	int j;
	int fh;
	char prefname[256];

	fh = hexchat_open_file ("colors.conf", O_TRUNC | O_WRONLY | O_CREAT, 0600, XOF_DOMODE);
	if (fh == -1)
		return;

	/* mIRC colors 0-31 */
	for (i = 0; i < 32; i++)
	{
		g_snprintf (prefname, sizeof prefname, "color_%d", i);
		cfg_put_color (fh, colors[i].red, colors[i].green, colors[i].blue, prefname);
	}

	/* extended colors 256+ -> 32..MAX_COL */
	for (i = 256, j = 32; j < MAX_COL + 1; i++, j++)
	{
		g_snprintf (prefname, sizeof prefname, "color_%d", i);
		cfg_put_color (fh, colors[j].red, colors[j].green, colors[j].blue, prefname);
	}

	close (fh);
}
