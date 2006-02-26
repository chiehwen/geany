/*
 *      highligting.c - this file is part of Geany, a fast and lightweight IDE
 *
 *      Copyright 2006 Enrico Troeger <enrico.troeger@uvena.de>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 */

#include "geany.h"

#ifdef GEANY_WIN32
# include "win32.h"
#else
/* my_strtod() is an simple implementation of strtod() for Win32 systems(declared in win32.h),
 * because the Win32 API cannot handle hexadecimal numbers(*grrr* it is an ANSI-C function),
 * my_strtod does only work for numbers like 0x..., on non-Win32 systems use the ANSI-C
 * function strtod() */
# include <stdlib.h>
# define my_strtod(x, y) strtod(x, y)
#endif

#include "highlighting.h"

#include "utils.h"


static style_set *types[GEANY_MAX_FILE_TYPES] = { NULL };


/* simple wrapper function to print file errors in DEBUG mode */
static void style_set_load_file(GKeyFile *key_file, const gchar *file, GKeyFileFlags flags, GError **just_for_compatibility)
{
	GError *error = NULL;
	gboolean done = g_key_file_load_from_file(key_file, file, flags, &error);
	if (! done && error != NULL)
	{
		geany_debug("Failed to open %s (%s)", file, error->message);
		g_error_free(error);
	}
}


static gchar *styleset_get_string(GKeyFile *config, const gchar *section, const gchar *key)
{
	GError *error = NULL;
	gchar *result;

	if (config == NULL || section == NULL) return NULL;

	result = g_key_file_get_string(config, section, key, &error);
	//if (error) geany_debug(error->message);
	if (error) g_error_free(error);

	return result;
}


static void styleset_get_hex(GKeyFile *config, const gchar *section, const gchar *key,
					  const gchar *foreground, const gchar *background, const gchar *bold, gint array[])
{
	GError *error = NULL;
	gchar **list;
	gsize len;

	if (config == NULL || section == NULL) return;

	list = g_key_file_get_string_list(config, section, key, &len, &error);

	if (list != NULL && list[0] != NULL) array[0] = (gint) my_strtod(list[0], NULL);
	else array[0] = (gint) my_strtod(foreground, NULL);
	if (list != NULL && list[1] != NULL) array[1] = (gint) my_strtod(list[1], NULL);
	else array[1] = (gint) my_strtod(background, NULL);
	if (list != NULL && list[2] != NULL) array[2] = utils_atob(list[2]);
	else array[2] = utils_atob(bold);

	g_strfreev(list);
}


static void styleset_set_style(ScintillaObject *sci, gint style, gint filetype, gint styling_index)
{
	SSM(sci, SCI_STYLESETFORE, style, types[filetype]->styling[styling_index][0]);
	SSM(sci, SCI_STYLESETBACK, style, types[filetype]->styling[styling_index][1]);
	SSM(sci, SCI_STYLESETBOLD, style, types[filetype]->styling[styling_index][2]);
}


void styleset_free_styles()
{
	gint i;

	for (i = 0; i < GEANY_MAX_FILE_TYPES; i++)
	{
		if (types[i] != NULL)
		{
			g_strfreev(types[i]->keywords);
			g_free(types[i]);
		}
	}
}


static void styleset_common_init(void)
{
	GKeyFile *config = g_key_file_new();

	style_set_load_file(config, GEANY_DATA_DIR "/filetypes.common", G_KEY_FILE_KEEP_COMMENTS, NULL);

	types[GEANY_FILETYPES_ALL] = g_new(style_set, 1);
	styleset_get_hex(config, "styling", "default", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_ALL]->styling[0]);
	styleset_get_hex(config, "styling", "selection", "0xc0c0c0", "0x00007f", "false", types[GEANY_FILETYPES_ALL]->styling[1]);
	styleset_get_hex(config, "styling", "brace_good", "0xff0000", "0xffffff", "false", types[GEANY_FILETYPES_ALL]->styling[2]);
	styleset_get_hex(config, "styling", "brace_bad", "0x0000ff", "0xffffff", "false", types[GEANY_FILETYPES_ALL]->styling[3]);

	types[GEANY_FILETYPES_ALL]->keywords = NULL;

	g_key_file_free(config);
}


void styleset_common(ScintillaObject *sci, gint style_bits)
{
	if (types[GEANY_FILETYPES_ALL] == NULL) styleset_common_init();

	SSM(sci, SCI_STYLESETFORE, STYLE_DEFAULT, types[GEANY_FILETYPES_ALL]->styling[0][0]);
	SSM(sci, SCI_STYLESETBACK, STYLE_DEFAULT, types[GEANY_FILETYPES_ALL]->styling[0][1]);
	SSM(sci, SCI_STYLESETBOLD, STYLE_DEFAULT, types[GEANY_FILETYPES_ALL]->styling[0][2]);

	SSM(sci, SCI_STYLECLEARALL, 0, 0);

	SSM(sci, SCI_SETUSETABS, TRUE, 0);
	SSM(sci, SCI_SETTABWIDTH, app->pref_editor_tab_width, 0);

	// colourize the current line
	SSM(sci, SCI_SETCARETLINEBACK, 0xE5E5E5, 0);
	SSM(sci, SCI_SETCARETLINEVISIBLE, 1, 0);

	// a darker grey for the line number margin
	SSM(sci, SCI_STYLESETBACK, STYLE_LINENUMBER, 0xD0D0D0);
	//SSM(sci, SCI_STYLESETBACK, STYLE_LINENUMBER, 0xD0D0D0);

	// define marker symbols
	// 0 -> line marker
	SSM(sci, SCI_MARKERDEFINE, 0, SC_MARK_SHORTARROW);
	SSM(sci, SCI_MARKERSETFORE, 0, 0x00007f);
	SSM(sci, SCI_MARKERSETBACK, 0, 0x00ffff);

	// 1 -> user marker
	SSM(sci, SCI_MARKERDEFINE, 1, SC_MARK_PLUS);
	SSM(sci, SCI_MARKERSETFORE, 1, 0x000000);
	SSM(sci, SCI_MARKERSETBACK, 1, 0xB8F4B8);

    // 2 -> folding marker, other folding settings
	SSM(sci, SCI_SETMARGINTYPEN, 2, SC_MARGIN_SYMBOL);
	SSM(sci, SCI_SETMARGINMASKN, 2, SC_MASK_FOLDERS);
	SSM(sci, SCI_SETFOLDFLAGS, 0, 0);

    SSM (sci,SCI_MARKERDEFINE,  SC_MARKNUM_FOLDEROPEN, SC_MARK_BOXMINUS);
	SSM (sci,SCI_MARKERSETFORE, SC_MARKNUM_FOLDEROPEN, 0xffffff);
	SSM (sci,SCI_MARKERSETBACK, SC_MARKNUM_FOLDEROPEN, 0x000000);
    SSM (sci,SCI_MARKERDEFINE,  SC_MARKNUM_FOLDER, SC_MARK_BOXPLUS);
	SSM (sci,SCI_MARKERSETFORE, SC_MARKNUM_FOLDER, 0xffffff);
	SSM (sci,SCI_MARKERSETBACK, SC_MARKNUM_FOLDER, 0x000000);
    SSM (sci,SCI_MARKERDEFINE,  SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE);
	SSM (sci,SCI_MARKERSETFORE, SC_MARKNUM_FOLDERSUB, 0xffffff);
	SSM (sci,SCI_MARKERSETBACK, SC_MARKNUM_FOLDERSUB, 0x000000);
    SSM (sci,SCI_MARKERDEFINE,  SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER);
	SSM (sci,SCI_MARKERSETFORE, SC_MARKNUM_FOLDERTAIL, 0xffffff);
	SSM (sci,SCI_MARKERSETBACK, SC_MARKNUM_FOLDERTAIL, 0x000000);
    SSM (sci,SCI_MARKERDEFINE,  SC_MARKNUM_FOLDEREND, SC_MARK_BOXPLUSCONNECTED);
	SSM (sci,SCI_MARKERSETFORE, SC_MARKNUM_FOLDEREND, 0xffffff);
	SSM (sci,SCI_MARKERSETBACK, SC_MARKNUM_FOLDEREND, 0x000000);
    SSM (sci,SCI_MARKERDEFINE,  SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUSCONNECTED);
	SSM (sci,SCI_MARKERSETFORE, SC_MARKNUM_FOLDEROPENMID, 0xffffff);
	SSM (sci,SCI_MARKERSETBACK, SC_MARKNUM_FOLDEROPENMID, 0x000000);
    SSM (sci,SCI_MARKERDEFINE,  SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER);
	SSM (sci,SCI_MARKERSETFORE, SC_MARKNUM_FOLDERMIDTAIL, 0xffffff);
	SSM (sci,SCI_MARKERSETBACK, SC_MARKNUM_FOLDERMIDTAIL, 0x000000);

	SSM(sci, SCI_SETPROPERTY, (sptr_t) "fold", (sptr_t) "1");
	SSM(sci, SCI_SETPROPERTY, (sptr_t) "fold.compact", (sptr_t) "1");
	SSM(sci, SCI_SETPROPERTY, (sptr_t) "fold.comment", (sptr_t) "1");
	SSM(sci, SCI_SETPROPERTY, (sptr_t) "fold.preprocessor", (sptr_t) "1");
	SSM(sci, SCI_SETPROPERTY, (sptr_t) "fold.at.else", (sptr_t) "1");


	SSM(sci, SCI_SETSELFORE, 1, types[GEANY_FILETYPES_ALL]->styling[1][0]);
	SSM(sci, SCI_SETSELBACK, 1, types[GEANY_FILETYPES_ALL]->styling[1][1]);

	SSM (sci, SCI_SETSTYLEBITS, style_bits, 0);

	styleset_set_style(sci, STYLE_BRACELIGHT, GEANY_FILETYPES_ALL, 2);
	styleset_set_style(sci, STYLE_BRACEBAD, GEANY_FILETYPES_ALL, 2);
}


static void styleset_c_init(void)
{
	GKeyFile *config = g_key_file_new();

	style_set_load_file(config, GEANY_DATA_DIR "/filetypes.c", G_KEY_FILE_KEEP_COMMENTS, NULL);

	types[GEANY_FILETYPES_C] = g_new(style_set, 1);
	styleset_get_hex(config, "styling", "default", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_C]->styling[0]);
	styleset_get_hex(config, "styling", "comment", "0x0000ff", "0xffffff", "false", types[GEANY_FILETYPES_C]->styling[1]);
	styleset_get_hex(config, "styling", "commentline", "0x0000ff", "0xffffff", "false", types[GEANY_FILETYPES_C]->styling[2]);
	styleset_get_hex(config, "styling", "commentdoc", "0x0000ff", "0xffffff", "false", types[GEANY_FILETYPES_C]->styling[3]);
	styleset_get_hex(config, "styling", "number", "0x007f00", "0xffffff", "false", types[GEANY_FILETYPES_C]->styling[4]);
	styleset_get_hex(config, "styling", "word", "0x991111", "0xffffff", "true", types[GEANY_FILETYPES_C]->styling[5]);
	styleset_get_hex(config, "styling", "word2", "0x00007F", "0xffffff", "true", types[GEANY_FILETYPES_C]->styling[6]);
	styleset_get_hex(config, "styling", "string", "0x1E90FF", "0xffffff", "false", types[GEANY_FILETYPES_C]->styling[7]);
	styleset_get_hex(config, "styling", "character", "0x1E90FF", "0xffffff", "false", types[GEANY_FILETYPES_C]->styling[8]);
	styleset_get_hex(config, "styling", "uuid", "0x804040", "0xffffff", "false", types[GEANY_FILETYPES_C]->styling[9]);
	styleset_get_hex(config, "styling", "preprocessor", "0x7F7F00", "0xffffff", "false", types[GEANY_FILETYPES_C]->styling[10]);
	styleset_get_hex(config, "styling", "operator", "0x101030", "0xffffff", "false", types[GEANY_FILETYPES_C]->styling[11]);
	styleset_get_hex(config, "styling", "identifier", "0x100000", "0xffffff", "false", types[GEANY_FILETYPES_C]->styling[12]);
	styleset_get_hex(config, "styling", "stringeol", "0x000000", "0xe0c0e0", "false", types[GEANY_FILETYPES_C]->styling[13]);
	styleset_get_hex(config, "styling", "verbatim", "0x101030", "0xffffff", "false", types[GEANY_FILETYPES_C]->styling[14]);
	styleset_get_hex(config, "styling", "regex", "0x101030", "0xffffff", "false", types[GEANY_FILETYPES_C]->styling[15]);
	styleset_get_hex(config, "styling", "commentlinedoc", "0x0000ff", "0xffffff", "true", types[GEANY_FILETYPES_C]->styling[16]);
	styleset_get_hex(config, "styling", "commentdockeyword", "0x0000ff", "0xffffff", "true", types[GEANY_FILETYPES_C]->styling[17]);
	styleset_get_hex(config, "styling", "globalclass", "0xbb1111", "0xffffff", "true", types[GEANY_FILETYPES_C]->styling[18]);

	types[GEANY_FILETYPES_C]->keywords = g_new(gchar*, 3);
	types[GEANY_FILETYPES_C]->keywords[0] = styleset_get_string(config, "keywords", "primary");
	if (types[GEANY_FILETYPES_C]->keywords[0] == NULL)
			types[GEANY_FILETYPES_C]->keywords[0] = g_strdup("if const struct char int float double void long for while do case switch return");
	types[GEANY_FILETYPES_C]->keywords[1] = styleset_get_string(config, "keywords", "docComment");
	if (types[GEANY_FILETYPES_C]->keywords[1] == NULL)
			types[GEANY_FILETYPES_C]->keywords[1] = g_strdup("TODO FIXME");
	types[GEANY_FILETYPES_C]->keywords[2] = NULL;

	g_key_file_free(config);

	// load global tags file for C autocompletion
	if (! app->ignore_global_tags)	tm_workspace_load_global_tags(GEANY_DATA_DIR "/global.tags");
}


void styleset_c(ScintillaObject *sci)
{

	if (types[GEANY_FILETYPES_C] == NULL) styleset_c_init();

	styleset_common(sci, 5);


	/* Assign global keywords */
	if ((app->tm_workspace) && (app->tm_workspace->global_tags))
	{
		guint j;
		GPtrArray *g_typedefs = tm_tags_extract(app->tm_workspace->global_tags, tm_tag_typedef_t | tm_tag_struct_t | tm_tag_class_t);
		if ((g_typedefs) && (g_typedefs->len > 0))
		{
			GString *s = g_string_sized_new(g_typedefs->len * 10);
			for (j = 0; j < g_typedefs->len; ++j)
			{
				if (!(TM_TAG(g_typedefs->pdata[j])->atts.entry.scope))
				{
					g_string_append(s, TM_TAG(g_typedefs->pdata[j])->name);
					g_string_append_c(s, ' ');
				}
			}
			SSM(sci, SCI_SETKEYWORDS, 1, (sptr_t) s->str);
			g_string_free(s, TRUE);
		}
		g_ptr_array_free(g_typedefs, TRUE);
	}

	SSM (sci, SCI_SETWORDCHARS, 0, (sptr_t) GEANY_WORDCHARS);
	SSM (sci, SCI_AUTOCSETMAXHEIGHT, 8, 0);

	SSM (sci, SCI_SETLEXER, SCLEX_CPP, 0);

	//SSM (sci, SCI_SETCONTROLCHARSYMBOL, 32, 0);

	SSM(sci, SCI_SETKEYWORDS, 0, (sptr_t) types[GEANY_FILETYPES_C]->keywords[0]);
	//SSM(sci, SCI_SETKEYWORDS, 1, (sptr_t) secondaryKeyWords);
	SSM(sci, SCI_SETKEYWORDS, 2, (sptr_t) types[GEANY_FILETYPES_C]->keywords[1]);
	//SSM(sci, SCI_SETKEYWORDS, 4, (sptr_t) typedefsKeyWords);

	styleset_set_style(sci, SCE_C_DEFAULT, GEANY_FILETYPES_C, 0);
	styleset_set_style(sci, SCE_C_COMMENT, GEANY_FILETYPES_C, 1);
	styleset_set_style(sci, SCE_C_COMMENTLINE, GEANY_FILETYPES_C, 2);
	styleset_set_style(sci, SCE_C_COMMENTDOC, GEANY_FILETYPES_C, 3);
	styleset_set_style(sci, SCE_C_NUMBER, GEANY_FILETYPES_C, 4);
	styleset_set_style(sci, SCE_C_WORD, GEANY_FILETYPES_C, 5);
	styleset_set_style(sci, SCE_C_WORD2, GEANY_FILETYPES_C, 6);
	styleset_set_style(sci, SCE_C_STRING, GEANY_FILETYPES_C, 7);
	styleset_set_style(sci, SCE_C_CHARACTER, GEANY_FILETYPES_C, 8);
	styleset_set_style(sci, SCE_C_UUID, GEANY_FILETYPES_C, 9);
	styleset_set_style(sci, SCE_C_PREPROCESSOR, GEANY_FILETYPES_C, 10);
	styleset_set_style(sci, SCE_C_OPERATOR, GEANY_FILETYPES_C, 11);
	styleset_set_style(sci, SCE_C_IDENTIFIER, GEANY_FILETYPES_C, 12);
	styleset_set_style(sci, SCE_C_STRINGEOL, GEANY_FILETYPES_ALL, 13);
	styleset_set_style(sci, SCE_C_VERBATIM, GEANY_FILETYPES_C, 14);
	styleset_set_style(sci, SCE_C_REGEX, GEANY_FILETYPES_C, 15);
	SSM(sci, SCI_STYLESETITALIC, SCE_C_COMMENTLINEDOC, TRUE);
	styleset_set_style(sci, SCE_C_COMMENTLINEDOC, GEANY_FILETYPES_C, 16);
	styleset_set_style(sci, SCE_C_COMMENTDOCKEYWORD, GEANY_FILETYPES_C, 17);

	SSM(sci, SCI_STYLESETFORE, SCE_C_COMMENTDOCKEYWORDERROR, 0x0000ff);
	SSM(sci, SCI_STYLESETBACK, SCE_C_COMMENTDOCKEYWORDERROR, 0xFFFFFF);
	SSM(sci, SCI_STYLESETITALIC, SCE_C_COMMENTDOCKEYWORDERROR, TRUE);

	// is used for local structs and typedefs
	styleset_set_style(sci, SCE_C_GLOBALCLASS, GEANY_FILETYPES_C, 18);

	SSM(sci, SCI_SETWHITESPACEFORE, 1, 0xc0c0c0);

	SSM(sci, SCI_SETPROPERTY, (sptr_t) "styling.within.preprocessor", (sptr_t) "1");
	SSM(sci, SCI_SETPROPERTY, (sptr_t) "file.patterns.cpp", (sptr_t) "*.cpp;*.cxx;*.cc");
	SSM(sci, SCI_SETPROPERTY, (sptr_t) "preprocessor.symbol.$(file.patterns.cpp)", (sptr_t) "#");
	SSM(sci, SCI_SETPROPERTY, (sptr_t) "preprocessor.start.$(file.patterns.cpp)", (sptr_t) "if ifdef ifndef");
	SSM(sci, SCI_SETPROPERTY, (sptr_t) "preprocessor.middle.$(file.patterns.cpp)", (sptr_t) "else elif");
	SSM(sci, SCI_SETPROPERTY, (sptr_t) "preprocessor.end.$(file.patterns.cpp)", (sptr_t) "endif");
}


static void styleset_pascal_init(void)
{
	GKeyFile *config = g_key_file_new();

	style_set_load_file(config, GEANY_DATA_DIR "/filetypes.pascal", G_KEY_FILE_KEEP_COMMENTS, NULL);

	types[GEANY_FILETYPES_PASCAL] = g_new(style_set, 1);
	styleset_get_hex(config, "styling", "default", "0x0000ff", "0xffffff", "false", types[GEANY_FILETYPES_PASCAL]->styling[0]);
	styleset_get_hex(config, "styling", "comment", "0x0000ff", "0xffffff", "false", types[GEANY_FILETYPES_PASCAL]->styling[1]);
	styleset_get_hex(config, "styling", "number", "0x007F00", "0xffffff", "false", types[GEANY_FILETYPES_PASCAL]->styling[2]);
	styleset_get_hex(config, "styling", "word", "0x991111", "0xffffff", "true", types[GEANY_FILETYPES_PASCAL]->styling[3]);
	styleset_get_hex(config, "styling", "string", "0x1E90FF", "0xffffff", "false", types[GEANY_FILETYPES_PASCAL]->styling[4]);
	styleset_get_hex(config, "styling", "character", "0x004040", "0xffffff", "false", types[GEANY_FILETYPES_PASCAL]->styling[5]);
	styleset_get_hex(config, "styling", "preprocessor", "0x7F7F00", "0xffffff", "false", types[GEANY_FILETYPES_PASCAL]->styling[6]);
	styleset_get_hex(config, "styling", "operator", "0x101030", "0xffffff", "false", types[GEANY_FILETYPES_PASCAL]->styling[7]);
	styleset_get_hex(config, "styling", "identifier", "0x100000", "0xffffff", "false", types[GEANY_FILETYPES_PASCAL]->styling[8]);
	styleset_get_hex(config, "styling", "regex", "0x13631B", "0xffffff", "false", types[GEANY_FILETYPES_PASCAL]->styling[9]);

	types[GEANY_FILETYPES_PASCAL]->keywords = g_new(gchar*, 2);
	types[GEANY_FILETYPES_PASCAL]->keywords[0] = styleset_get_string(config, "keywords", "primary");
	if (types[GEANY_FILETYPES_PASCAL]->keywords[0] == NULL)
			types[GEANY_FILETYPES_PASCAL]->keywords[0] = g_strdup("word integer char string byte real \
									for to do until repeat program if uses then else case var begin end \
									asm unit interface implementation procedure function object try class");
	types[GEANY_FILETYPES_PASCAL]->keywords[1] = NULL;

	g_key_file_free(config);
}


void styleset_pascal(ScintillaObject *sci)
{
	if (types[GEANY_FILETYPES_PASCAL] == NULL) styleset_pascal_init();

	styleset_common(sci, 5);

	SSM (sci, SCI_SETWORDCHARS, 0, (sptr_t) GEANY_WORDCHARS);
	SSM (sci, SCI_AUTOCSETMAXHEIGHT, 8, 0);

	SSM (sci, SCI_SETLEXER, SCLEX_PASCAL, 0);

	SSM(sci, SCI_SETKEYWORDS, 0, (sptr_t) types[GEANY_FILETYPES_PASCAL]->keywords[0]);

	styleset_set_style(sci, SCE_C_DEFAULT, GEANY_FILETYPES_PASCAL, 0);
	styleset_set_style(sci, SCE_C_COMMENT, GEANY_FILETYPES_PASCAL, 1);
	styleset_set_style(sci, SCE_C_NUMBER, GEANY_FILETYPES_PASCAL, 2);
	styleset_set_style(sci, SCE_C_WORD, GEANY_FILETYPES_PASCAL, 3);
	styleset_set_style(sci, SCE_C_STRING, GEANY_FILETYPES_PASCAL, 4);
	styleset_set_style(sci, SCE_C_CHARACTER, GEANY_FILETYPES_PASCAL, 5);
	styleset_set_style(sci, SCE_C_PREPROCESSOR, GEANY_FILETYPES_PASCAL, 6);
	styleset_set_style(sci, SCE_C_OPERATOR, GEANY_FILETYPES_PASCAL, 7);
	styleset_set_style(sci, SCE_C_IDENTIFIER, GEANY_FILETYPES_PASCAL, 8);
	styleset_set_style(sci, SCE_C_REGEX, GEANY_FILETYPES_PASCAL, 9);

	SSM(sci, SCI_SETWHITESPACEFORE, 1, 0xc0c0c0);

	//SSM(sci, SCI_SETPROPERTY, (sptr_t) "styling.within.preprocessor", (sptr_t) "1");

}


static void styleset_makefile_init(void)
{
	GKeyFile *config = g_key_file_new();

	style_set_load_file(config, GEANY_DATA_DIR "/filetypes.makefile", G_KEY_FILE_KEEP_COMMENTS, NULL);

	types[GEANY_FILETYPES_MAKE] = g_new(style_set, 1);
	styleset_get_hex(config, "styling", "default", "0xFF0000", "0xffffff", "false", types[GEANY_FILETYPES_MAKE]->styling[0]);
	styleset_get_hex(config, "styling", "comment", "0x0000ff", "0xffffff", "false", types[GEANY_FILETYPES_MAKE]->styling[1]);
	styleset_get_hex(config, "styling", "preprocessor", "0x7F7F00", "0xffffff", "false", types[GEANY_FILETYPES_MAKE]->styling[2]);
	styleset_get_hex(config, "styling", "identifier", "0x007F00", "0xffffff", "false", types[GEANY_FILETYPES_MAKE]->styling[3]);
	styleset_get_hex(config, "styling", "operator", "0x101030", "0xffffff", "false", types[GEANY_FILETYPES_MAKE]->styling[4]);
	styleset_get_hex(config, "styling", "target", "0xff0000", "0xffffff", "false", types[GEANY_FILETYPES_MAKE]->styling[5]);
	styleset_get_hex(config, "styling", "ideol", "0x008000", "0xffffff", "false", types[GEANY_FILETYPES_MAKE]->styling[6]);

	types[GEANY_FILETYPES_MAKE]->keywords = NULL;

	g_key_file_free(config);
}


void styleset_makefile(ScintillaObject *sci)
{
	if (types[GEANY_FILETYPES_MAKE] == NULL) styleset_makefile_init();

	styleset_common(sci, 5);

	SSM (sci, SCI_SETLEXER, SCLEX_MAKEFILE, 0);

	styleset_set_style(sci, SCE_MAKE_DEFAULT, GEANY_FILETYPES_MAKE, 0);
	styleset_set_style(sci, SCE_MAKE_COMMENT, GEANY_FILETYPES_MAKE, 1);
	styleset_set_style(sci, SCE_MAKE_PREPROCESSOR, GEANY_FILETYPES_MAKE, 2);
	styleset_set_style(sci, SCE_MAKE_IDENTIFIER, GEANY_FILETYPES_MAKE, 3);
	styleset_set_style(sci, SCE_MAKE_OPERATOR, GEANY_FILETYPES_MAKE, 4);
	styleset_set_style(sci, SCE_MAKE_TARGET, GEANY_FILETYPES_MAKE, 5);
	styleset_set_style(sci, SCE_MAKE_IDEOL, GEANY_FILETYPES_MAKE, 6);
}


static void styleset_tex_init(void)
{
	GKeyFile *config = g_key_file_new();

	style_set_load_file(config, GEANY_DATA_DIR "/filetypes.tex", G_KEY_FILE_KEEP_COMMENTS, NULL);

	types[GEANY_FILETYPES_TEX] = g_new(style_set, 1);
	styleset_get_hex(config, "styling", "default", "0xFF0000", "0xffffff", "false", types[GEANY_FILETYPES_TEX]->styling[0]);
	styleset_get_hex(config, "styling", "command", "0x0000ff", "0xffffff", "true", types[GEANY_FILETYPES_TEX]->styling[1]);
	styleset_get_hex(config, "styling", "tag", "0x7F7F00", "0xffffff", "true", types[GEANY_FILETYPES_TEX]->styling[2]);
	styleset_get_hex(config, "styling", "math", "0x007F00", "0xffffff", "false", types[GEANY_FILETYPES_TEX]->styling[3]);
	styleset_get_hex(config, "styling", "comment", "0x007F00", "0xffffff", "false", types[GEANY_FILETYPES_TEX]->styling[4]);

	types[GEANY_FILETYPES_TEX]->keywords = g_new(gchar*, 2);
	types[GEANY_FILETYPES_TEX]->keywords[0] = styleset_get_string(config, "keywords", "primary");
	if (types[GEANY_FILETYPES_TEX]->keywords[0] == NULL)
			types[GEANY_FILETYPES_TEX]->keywords[0] = g_strdup("section subsection begin item");
	types[GEANY_FILETYPES_TEX]->keywords[1] = NULL;

	g_key_file_free(config);
}


void styleset_tex(ScintillaObject *sci)
{
	if (types[GEANY_FILETYPES_TEX] == NULL) styleset_tex_init();

	styleset_common(sci, 5);

	SSM (sci, SCI_SETLEXER, SCLEX_LATEX, 0);

	SSM(sci, SCI_SETKEYWORDS, 0, (sptr_t) types[GEANY_FILETYPES_TEX]->keywords[0]);

	styleset_set_style(sci, SCE_L_DEFAULT, GEANY_FILETYPES_TEX, 0);
	styleset_set_style(sci, SCE_L_COMMAND, GEANY_FILETYPES_TEX, 1);
	styleset_set_style(sci, SCE_L_TAG, GEANY_FILETYPES_TEX, 2);
	styleset_set_style(sci, SCE_L_MATH, GEANY_FILETYPES_TEX, 3);
	SSM(sci, SCI_STYLESETITALIC, SCE_L_COMMENT, TRUE);
	styleset_set_style(sci, SCE_L_COMMENT, GEANY_FILETYPES_TEX, 4);
}


void styleset_php(ScintillaObject *sci)
{
	styleset_common(sci, 7);

	SSM (sci, SCI_SETPROPERTY, (sptr_t) "phpscript.mode", (sptr_t) "1");
	SSM (sci, SCI_SETLEXER, SCLEX_HTML, 0);

	// DWELL notification for URL highlighting
	SSM(sci, SCI_SETMOUSEDWELLTIME, 500, 0);

	// use the same colouring for HTML; XML and so on
	styleset_markup(sci);

}


static void styleset_markup_init(void)
{
	GKeyFile *config = g_key_file_new();

	style_set_load_file(config, GEANY_DATA_DIR "/filetypes.markup", G_KEY_FILE_KEEP_COMMENTS, NULL);

	types[GEANY_FILETYPES_XML] = g_new(style_set, 1);
	styleset_get_hex(config, "styling", "html_default", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[0]);
	styleset_get_hex(config, "styling", "html_tag", "0x990000", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[1]);
	styleset_get_hex(config, "styling", "html_tagunknown", "0x0000ff", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[2]);
	styleset_get_hex(config, "styling", "html_attribute", "0x007F00", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[3]);
	styleset_get_hex(config, "styling", "html_attributeunknown", "0x0000ff", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[4]);
	styleset_get_hex(config, "styling", "html_number", "0x800080", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[5]);
	styleset_get_hex(config, "styling", "html_doublestring", "0x1E90FF", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[6]);
	styleset_get_hex(config, "styling", "html_singlestring", "0x1E90FF", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[7]);
	styleset_get_hex(config, "styling", "html_other", "0x800080", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[8]);
	styleset_get_hex(config, "styling", "html_comment", "0x808080", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[9]);
	styleset_get_hex(config, "styling", "html_entity", "0x800080", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[10]);
	styleset_get_hex(config, "styling", "html_tagend", "0x800000", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[11]);
	styleset_get_hex(config, "styling", "html_xmlstart", "0x990000", "0xf0f0f0", "false", types[GEANY_FILETYPES_XML]->styling[12]);
	styleset_get_hex(config, "styling", "html_xmlend", "0x990000", "0xf0f0f0", "false", types[GEANY_FILETYPES_XML]->styling[13]);
	styleset_get_hex(config, "styling", "html_script", "0x800000", "0xf0f0f0", "false", types[GEANY_FILETYPES_XML]->styling[14]);
	styleset_get_hex(config, "styling", "html_asp", "0x4f4f00", "0xf0f0f0", "false", types[GEANY_FILETYPES_XML]->styling[15]);
	styleset_get_hex(config, "styling", "html_aspat", "0x4f4f00", "0xf0f0f0", "false", types[GEANY_FILETYPES_XML]->styling[16]);
	styleset_get_hex(config, "styling", "html_cdata", "0x990066", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[17]);
	styleset_get_hex(config, "styling", "html_question", "0xff0000", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[18]);
	styleset_get_hex(config, "styling", "html_value", "0x990066", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[19]);
	styleset_get_hex(config, "styling", "html_xccomment", "0x990066", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[20]);

	styleset_get_hex(config, "styling", "sgml_default", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[21]);
	styleset_get_hex(config, "styling", "sgml_comment", "0x808080", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[22]);
	styleset_get_hex(config, "styling", "sgml_special", "0x007F00", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[23]);
	styleset_get_hex(config, "styling", "sgml_command", "0x991111", "0xffffff", "true", types[GEANY_FILETYPES_XML]->styling[24]);
	styleset_get_hex(config, "styling", "sgml_doublestring", "0x1E90FF", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[25]);
	styleset_get_hex(config, "styling", "sgml_simplestring", "0x004040", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[26]);
	styleset_get_hex(config, "styling", "sgml_1st_param", "0x804040", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[27]);
	styleset_get_hex(config, "styling", "sgml_entity", "0x101030", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[28]);
	styleset_get_hex(config, "styling", "sgml_block_default", "0x100000", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[29]);
	styleset_get_hex(config, "styling", "sgml_1st_param_comment", "0x906040", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[30]);
	styleset_get_hex(config, "styling", "sgml_error", "0x0000ff", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[31]);

	styleset_get_hex(config, "styling", "php_default", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[32]);
	styleset_get_hex(config, "styling", "php_simplestring", "0x008000", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[33]);
	styleset_get_hex(config, "styling", "php_hstring", "0x008000", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[34]);
	styleset_get_hex(config, "styling", "php_number", "0x006060", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[35]);
	styleset_get_hex(config, "styling", "php_word", "0x990000", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[36]);
	styleset_get_hex(config, "styling", "php_variable", "0x00007F", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[37]);
	styleset_get_hex(config, "styling", "php_comment", "0x808080", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[38]);
	styleset_get_hex(config, "styling", "php_commentline", "0x808080", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[39]);
	styleset_get_hex(config, "styling", "php_operator", "0x602010", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[40]);
	styleset_get_hex(config, "styling", "php_hstring_variable", "0x601010", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[41]);
	styleset_get_hex(config, "styling", "php_complex_variable", "0x105010", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[42]);

	styleset_get_hex(config, "styling", "jscript_start", "0x808000", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[43]);
	styleset_get_hex(config, "styling", "jscript_default", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[44]);
	styleset_get_hex(config, "styling", "jscript_comment", "0x222222", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[45]);
	styleset_get_hex(config, "styling", "jscript_commentline", "0x808080", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[46]);
	styleset_get_hex(config, "styling", "jscript_commentdoc", "0x808080", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[47]);
	styleset_get_hex(config, "styling", "jscript_number", "0x606000", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[48]);
	styleset_get_hex(config, "styling", "jscript_word", "0xff0000", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[49]);
	styleset_get_hex(config, "styling", "jscript_keyword", "0x001050", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[50]);
	styleset_get_hex(config, "styling", "jscript_doublestring", "0x080080", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[51]);
	styleset_get_hex(config, "styling", "jscript_singlestring", "0x080080", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[52]);
	styleset_get_hex(config, "styling", "jscript_symbols", "0x501000", "0xffffff", "false", types[GEANY_FILETYPES_XML]->styling[53]);
	styleset_get_hex(config, "styling", "jscript_stringeol", "0x000000", "0xe0c0e0", "false", types[GEANY_FILETYPES_XML]->styling[54]);

	types[GEANY_FILETYPES_XML]->keywords = g_new(gchar*, 7);
	types[GEANY_FILETYPES_XML]->keywords[0] = styleset_get_string(config, "keywords", "html");
	if (types[GEANY_FILETYPES_XML]->keywords[0] == NULL)
			types[GEANY_FILETYPES_XML]->keywords[0] = g_strdup("a abbr acronym address applet area b base basefont bdo big blockquote body br button caption center cite code col colgroup dd del dfn dir div dl dt em embed fieldset font form frame frameset h1 h2 h3 h4 h5 h6 head hr html i iframe img input ins isindex kbd label legend li link map menu meta noframes noscript object ol optgroup option p param pre q quality s samp script select small span strike strong style sub sup table tbody td textarea tfoot th thead title tr tt u ul var xmlns leftmargin topmargin abbr accept-charset accept accesskey action align alink alt archive axis background bgcolor border cellpadding cellspacing char charoff charset checked cite class classid clear codebase codetype color cols colspan compact content coords data datafld dataformatas datapagesize datasrc datetime declare defer dir disabled enctype face for frame frameborder selected headers height href hreflang hspace http-equiv id ismap label lang language link longdesc marginwidth marginheight maxlength media framespacing method multiple name nohref noresize noshade nowrap object onblur onchange onclick ondblclick onfocus onkeydown onkeypress onkeyup onload onmousedown onmousemove onmouseover onmouseout onmouseup onreset onselect onsubmit onunload profile prompt pluginspage readonly rel rev rows rowspan rules scheme scope scrolling shape size span src standby start style summary tabindex target text title type usemap valign value valuetype version vlink vspace width text password checkbox radio submit reset file hidden image public doctype xml");
	types[GEANY_FILETYPES_XML]->keywords[1] = styleset_get_string(config, "keywords", "javascript");
	if (types[GEANY_FILETYPES_XML]->keywords[1] == NULL)
			types[GEANY_FILETYPES_XML]->keywords[1] = g_strdup("break this for while null else var false void new delete typeof if in continue true function with return case super extends do const try debugger catch switch finally enum export default class throw import length concat join pop push reverse shift slice splice sort unshift Date Infinity NaN undefined escape eval isFinite isNaN Number parseFloat parseInt string unescape Math abs acos asin atan atan2 ceil cos exp floor log max min pow random round sin sqrt tan MAX_VALUE MIN_VALUE NEGATIVE_INFINITY POSITIVE_INFINITY toString valueOf String length anchor big bold charAt charCodeAt concat fixed fontcolor fontsize fromCharCode indexOf italics lastIndexOf link slice small split strike sub substr substring sup toLowerCase toUpperCase");
	types[GEANY_FILETYPES_XML]->keywords[2] = styleset_get_string(config, "keywords", "vbscript");
	if (types[GEANY_FILETYPES_XML]->keywords[2] == NULL)
			types[GEANY_FILETYPES_XML]->keywords[2] = g_strdup("and as byref byval case call const continue dim do each else elseif end error exit false for function global goto if in loop me new next not nothing on optional or private public redim rem resume select set sub then to true type while with boolean byte currency date double integer long object single string type variant");
	types[GEANY_FILETYPES_XML]->keywords[3] = styleset_get_string(config, "keywords", "python");
	if (types[GEANY_FILETYPES_XML]->keywords[3] == NULL)
			types[GEANY_FILETYPES_XML]->keywords[3] = g_strdup("and assert break class continue complex def del elif else except exec finally for from global if import in inherit is int lambda not or pass print raise return tuple try unicode while yield long float str list");
	types[GEANY_FILETYPES_XML]->keywords[4] = styleset_get_string(config, "keywords", "php");
	if (types[GEANY_FILETYPES_XML]->keywords[4] == NULL)
			types[GEANY_FILETYPES_XML]->keywords[4] = g_strdup("and or xor FILE exception LINE array as break case class const continue declare default die do echo else elseif empty  enddeclare endfor endforeach endif endswitch endwhile eval exit extends for foreach function global if include include_once  isset list new print require require_once return static switch unset use var while FUNCTION CLASS METHOD final php_user_filter interface implements extends public private protected abstract clone try catch throw cfunction old_function this");
	types[GEANY_FILETYPES_XML]->keywords[5] = styleset_get_string(config, "keywords", "sgml");
	if (types[GEANY_FILETYPES_XML]->keywords[5] == NULL)
			types[GEANY_FILETYPES_XML]->keywords[5] = g_strdup("ELEMENT DOCTYPE ATTLIST ENTITY NOTATION");
	types[GEANY_FILETYPES_XML]->keywords[6] = NULL;

	g_key_file_free(config);
}


void styleset_markup(ScintillaObject *sci)
{
	if (types[GEANY_FILETYPES_XML] == NULL) styleset_markup_init();


	SSM(sci, SCI_SETKEYWORDS, 0, (sptr_t) types[GEANY_FILETYPES_XML]->keywords[0]);
	SSM(sci, SCI_SETKEYWORDS, 1, (sptr_t) types[GEANY_FILETYPES_XML]->keywords[1]);
	SSM(sci, SCI_SETKEYWORDS, 2, (sptr_t) types[GEANY_FILETYPES_XML]->keywords[2]);
	SSM(sci, SCI_SETKEYWORDS, 3, (sptr_t) types[GEANY_FILETYPES_XML]->keywords[3]);
	SSM(sci, SCI_SETKEYWORDS, 4, (sptr_t) types[GEANY_FILETYPES_XML]->keywords[4]);
	SSM(sci, SCI_SETKEYWORDS, 5, (sptr_t) types[GEANY_FILETYPES_XML]->keywords[5]);

	SSM(sci, SCI_SETWHITESPACEFORE, 1, 0xc0c0c0);

	// hotspotting, nice thing
	SSM(sci, SCI_SETHOTSPOTACTIVEFORE, 1, 0xff0000);
	SSM(sci, SCI_SETHOTSPOTACTIVEUNDERLINE, 1, 0);
	SSM(sci, SCI_SETHOTSPOTSINGLELINE, 1, 0);
	SSM(sci, SCI_STYLESETHOTSPOT, SCE_H_QUESTION, 1);


	styleset_set_style(sci, SCE_H_DEFAULT, GEANY_FILETYPES_XML, 0);
	styleset_set_style(sci, SCE_H_TAG, GEANY_FILETYPES_XML, 1);
	styleset_set_style(sci, SCE_H_TAGUNKNOWN, GEANY_FILETYPES_XML, 2);
	styleset_set_style(sci, SCE_H_ATTRIBUTE, GEANY_FILETYPES_XML, 3);
	styleset_set_style(sci, SCE_H_ATTRIBUTEUNKNOWN, GEANY_FILETYPES_XML, 4);
	styleset_set_style(sci, SCE_H_NUMBER, GEANY_FILETYPES_XML, 5);
	styleset_set_style(sci, SCE_H_DOUBLESTRING, GEANY_FILETYPES_XML, 6);
	styleset_set_style(sci, SCE_H_SINGLESTRING, GEANY_FILETYPES_XML, 7);
	styleset_set_style(sci, SCE_H_OTHER, GEANY_FILETYPES_XML, 8);
	styleset_set_style(sci, SCE_H_COMMENT, GEANY_FILETYPES_XML, 9);
	styleset_set_style(sci, SCE_H_ENTITY, GEANY_FILETYPES_XML, 10);
	styleset_set_style(sci, SCE_H_TAGEND, GEANY_FILETYPES_XML, 11);
	SSM(sci, SCI_STYLESETEOLFILLED, SCE_H_XMLSTART, 1);
	styleset_set_style(sci, SCE_H_XMLSTART, GEANY_FILETYPES_XML, 12);
	styleset_set_style(sci, SCE_H_XMLEND, GEANY_FILETYPES_XML, 13);
	styleset_set_style(sci, SCE_H_SCRIPT, GEANY_FILETYPES_XML, 14);
	SSM(sci, SCI_STYLESETEOLFILLED, SCE_H_ASP, 1);
	styleset_set_style(sci, SCE_H_ASP, GEANY_FILETYPES_XML, 15);
	SSM(sci, SCI_STYLESETEOLFILLED, SCE_H_ASPAT, 1);
	styleset_set_style(sci, SCE_H_ASPAT, GEANY_FILETYPES_XML, 16);
	styleset_set_style(sci, SCE_H_CDATA, GEANY_FILETYPES_XML, 17);
	styleset_set_style(sci, SCE_H_QUESTION, GEANY_FILETYPES_XML, 18);
	styleset_set_style(sci, SCE_H_VALUE, GEANY_FILETYPES_XML, 19);
	styleset_set_style(sci, SCE_H_XCCOMMENT, GEANY_FILETYPES_XML, 20);

	styleset_set_style(sci, SCE_H_SGML_DEFAULT, GEANY_FILETYPES_XML, 21);
	styleset_set_style(sci, SCE_H_SGML_COMMENT, GEANY_FILETYPES_XML, 22);
	styleset_set_style(sci, SCE_H_SGML_SPECIAL, GEANY_FILETYPES_XML, 23);
	styleset_set_style(sci, SCE_H_SGML_COMMAND, GEANY_FILETYPES_XML, 24);
	styleset_set_style(sci, SCE_H_SGML_DOUBLESTRING, GEANY_FILETYPES_XML, 25);
	styleset_set_style(sci, SCE_H_SGML_SIMPLESTRING, GEANY_FILETYPES_XML, 26);
	styleset_set_style(sci, SCE_H_SGML_1ST_PARAM, GEANY_FILETYPES_XML, 27);
	styleset_set_style(sci, SCE_H_SGML_ENTITY, GEANY_FILETYPES_XML, 28);
	styleset_set_style(sci, SCE_H_SGML_BLOCK_DEFAULT, GEANY_FILETYPES_XML, 29);
	styleset_set_style(sci, SCE_H_SGML_1ST_PARAM_COMMENT, GEANY_FILETYPES_XML, 30);
	styleset_set_style(sci, SCE_H_SGML_ERROR, GEANY_FILETYPES_XML, 31);

	SSM(sci, SCI_STYLESETFORE, SCE_HB_DEFAULT, 0x000000);
	SSM(sci, SCI_STYLESETBACK, SCE_HB_DEFAULT, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HB_COMMENTLINE, 0x808080);
	SSM(sci, SCI_STYLESETBACK, SCE_HB_COMMENTLINE, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HB_NUMBER, 0x008080);
	SSM(sci, SCI_STYLESETBACK, SCE_HB_NUMBER, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HB_WORD, 0x008080);
	SSM(sci, SCI_STYLESETBACK, SCE_HB_WORD, 0xffffff);
	SSM(sci, SCI_STYLESETBOLD, SCE_HB_WORD, 1);

	SSM(sci, SCI_STYLESETFORE, SCE_HB_STRING, 0x008000);
	SSM(sci, SCI_STYLESETBACK, SCE_HB_STRING, 0x008000);

	SSM(sci, SCI_STYLESETFORE, SCE_HB_IDENTIFIER, 0x103000);
	SSM(sci, SCI_STYLESETBACK, SCE_HB_IDENTIFIER, 0xffffff);
//~ #define SCE_HB_START 70

	// Show the whole section of VBScript
/*	for (bstyle=SCE_HB_DEFAULT; bstyle<=SCE_HB_STRINGEOL; bstyle++) {
		SSM(sci, SCI_STYLESETBACK, bstyle, 0xf5f5f5);
		// This call extends the backround colour of the last style on the line to the edge of the window
		SSM(sci, SCI_STYLESETEOLFILLED, bstyle, 1);
	}
*/
	SSM(sci,SCI_STYLESETBACK, SCE_HB_STRINGEOL, 0x7F7FFF);

	SSM(sci, SCI_STYLESETFORE, SCE_HBA_DEFAULT, 0x000000);
	SSM(sci, SCI_STYLESETBACK, SCE_HBA_DEFAULT, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HBA_COMMENTLINE, 0x808080);
	SSM(sci, SCI_STYLESETBACK, SCE_HBA_COMMENTLINE, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HBA_NUMBER, 0x008080);
	SSM(sci, SCI_STYLESETBACK, SCE_HBA_NUMBER, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HBA_WORD, 0x008080);
	SSM(sci, SCI_STYLESETBACK, SCE_HBA_WORD, 0xffffff);
	SSM(sci, SCI_STYLESETBOLD, SCE_HBA_WORD, 1);

	SSM(sci, SCI_STYLESETFORE, SCE_HBA_STRING, 0x008000);
	SSM(sci, SCI_STYLESETBACK, SCE_HBA_STRING, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HBA_IDENTIFIER, 0x103000);
	SSM(sci, SCI_STYLESETBACK, SCE_HBA_IDENTIFIER, 0xffffff);

	styleset_set_style(sci, SCE_HJ_START, GEANY_FILETYPES_XML, 43);
	styleset_set_style(sci, SCE_HJ_DEFAULT, GEANY_FILETYPES_XML, 44);
	styleset_set_style(sci, SCE_HJ_COMMENT, GEANY_FILETYPES_XML, 45);
	styleset_set_style(sci, SCE_HJ_COMMENTLINE, GEANY_FILETYPES_XML, 46);
	styleset_set_style(sci, SCE_HJ_COMMENTDOC, GEANY_FILETYPES_XML, 47);
	styleset_set_style(sci, SCE_HJ_NUMBER, GEANY_FILETYPES_XML, 48);
	styleset_set_style(sci, SCE_HJ_WORD, GEANY_FILETYPES_XML, 49);
	styleset_set_style(sci, SCE_HJ_KEYWORD, GEANY_FILETYPES_XML, 50);
	styleset_set_style(sci, SCE_HJ_DOUBLESTRING, GEANY_FILETYPES_XML, 51);
	styleset_set_style(sci, SCE_HJ_SINGLESTRING, GEANY_FILETYPES_XML, 52);
	styleset_set_style(sci, SCE_HJ_SYMBOLS, GEANY_FILETYPES_XML, 53);
	styleset_set_style(sci, SCE_HJ_STRINGEOL, GEANY_FILETYPES_XML, 54);


	SSM(sci, SCI_STYLESETFORE, SCE_HP_START, 0x808000);
	SSM(sci, SCI_STYLESETBACK, SCE_HP_START, 0xf0f0f0);
	SSM(sci, SCI_STYLESETEOLFILLED, SCE_HP_START, 1);

	SSM(sci, SCI_STYLESETFORE, SCE_HP_DEFAULT, 0x000000);
	SSM(sci, SCI_STYLESETBACK, SCE_HP_DEFAULT, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HP_COMMENTLINE, 0x808080);
	SSM(sci, SCI_STYLESETBACK, SCE_HP_COMMENTLINE, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HP_NUMBER, 0x008080);
	SSM(sci, SCI_STYLESETBACK, SCE_HP_NUMBER, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HP_WORD, 0x990000);
	SSM(sci, SCI_STYLESETBACK, SCE_HP_WORD, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HP_STRING, 0x008000);
	SSM(sci, SCI_STYLESETBACK, SCE_HP_STRING, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HP_CHARACTER, 0x006060);
	SSM(sci, SCI_STYLESETBACK, SCE_HP_CHARACTER, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HP_TRIPLE, 0x002060);
	SSM(sci, SCI_STYLESETBACK, SCE_HP_TRIPLE, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HP_TRIPLEDOUBLE, 0x002060);
	SSM(sci, SCI_STYLESETBACK, SCE_HP_TRIPLEDOUBLE, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HP_CLASSNAME, 0x202010);
	SSM(sci, SCI_STYLESETBACK, SCE_HP_CLASSNAME, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HP_CLASSNAME, 0x102020);
	SSM(sci, SCI_STYLESETBACK, SCE_HP_CLASSNAME, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HP_OPERATOR, 0x602020);
	SSM(sci, SCI_STYLESETBACK, SCE_HP_OPERATOR, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HP_IDENTIFIER, 0x001060);
	SSM(sci, SCI_STYLESETBACK, SCE_HP_IDENTIFIER, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HPA_START, 0x808000);
	SSM(sci, SCI_STYLESETBACK, SCE_HPA_START, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HPA_DEFAULT, 0x000000);
	SSM(sci, SCI_STYLESETBACK, SCE_HPA_DEFAULT, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HPA_COMMENTLINE, 0x808080);
	SSM(sci, SCI_STYLESETBACK, SCE_HPA_COMMENTLINE, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HPA_NUMBER, 0x408000);
	SSM(sci, SCI_STYLESETBACK, SCE_HPA_NUMBER, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HPA_STRING, 0x008080);
	SSM(sci, SCI_STYLESETBACK, SCE_HPA_STRING, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HPA_CHARACTER, 0x505080);
	SSM(sci, SCI_STYLESETBACK, SCE_HPA_CHARACTER, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HPA_WORD, 0x990000);
	SSM(sci, SCI_STYLESETBACK, SCE_HPA_WORD, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HPA_TRIPLE, 0x002060);
	SSM(sci, SCI_STYLESETBACK, SCE_HPA_TRIPLE, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HPA_TRIPLEDOUBLE, 0x002060);
	SSM(sci, SCI_STYLESETBACK, SCE_HPA_TRIPLEDOUBLE, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HPA_CLASSNAME, 0x202010);
	SSM(sci, SCI_STYLESETBACK, SCE_HPA_CLASSNAME, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HPA_DEFNAME, 0x102020);
	SSM(sci, SCI_STYLESETBACK, SCE_HPA_DEFNAME, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HPA_OPERATOR, 0x601010);
	SSM(sci, SCI_STYLESETBACK, SCE_HPA_OPERATOR, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HPA_IDENTIFIER, 0x105010);
	SSM(sci, SCI_STYLESETBACK, SCE_HPA_IDENTIFIER, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HP_COMMENTLINE, 0x808080);
	SSM(sci, SCI_STYLESETBACK, SCE_HP_COMMENTLINE, 0xffffff);

	SSM(sci, SCI_STYLESETFORE, SCE_HP_NUMBER, 0x408000);
	SSM(sci, SCI_STYLESETBACK, SCE_HP_NUMBER, 0xffffff);

	styleset_set_style(sci, SCE_HPHP_DEFAULT, GEANY_FILETYPES_XML, 32);
	styleset_set_style(sci, SCE_HPHP_SIMPLESTRING, GEANY_FILETYPES_XML, 33);
	styleset_set_style(sci, SCE_HPHP_HSTRING, GEANY_FILETYPES_XML, 34);
	styleset_set_style(sci, SCE_HPHP_NUMBER, GEANY_FILETYPES_XML, 35);
	styleset_set_style(sci, SCE_HPHP_WORD, GEANY_FILETYPES_XML, 36);
	styleset_set_style(sci, SCE_HPHP_VARIABLE, GEANY_FILETYPES_XML, 37);
	styleset_set_style(sci, SCE_HPHP_COMMENT, GEANY_FILETYPES_XML, 38);
	styleset_set_style(sci, SCE_HPHP_COMMENTLINE, GEANY_FILETYPES_XML, 39);
	styleset_set_style(sci, SCE_HPHP_OPERATOR, GEANY_FILETYPES_XML, 40);
	styleset_set_style(sci, SCE_HPHP_HSTRING_VARIABLE, GEANY_FILETYPES_XML, 41);
	styleset_set_style(sci, SCE_HPHP_COMPLEX_VARIABLE, GEANY_FILETYPES_XML, 42);
}


static void styleset_java_init(void)
{
	GKeyFile *config = g_key_file_new();

	style_set_load_file(config, GEANY_DATA_DIR "/filetypes.java", G_KEY_FILE_KEEP_COMMENTS, NULL);

	types[GEANY_FILETYPES_JAVA] = g_new(style_set, 1);
	styleset_get_hex(config, "styling", "default", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_JAVA]->styling[0]);
	styleset_get_hex(config, "styling", "comment", "0x0000ff", "0xffffff", "false", types[GEANY_FILETYPES_JAVA]->styling[1]);
	styleset_get_hex(config, "styling", "commentline", "0x0000ff", "0xffffff", "false", types[GEANY_FILETYPES_JAVA]->styling[2]);
	styleset_get_hex(config, "styling", "commentdoc", "0x0000ff", "0xffffff", "false", types[GEANY_FILETYPES_JAVA]->styling[3]);
	styleset_get_hex(config, "styling", "number", "0x007f00", "0xffffff", "false", types[GEANY_FILETYPES_JAVA]->styling[4]);
	styleset_get_hex(config, "styling", "word", "0x991111", "0xffffff", "true", types[GEANY_FILETYPES_JAVA]->styling[5]);
	styleset_get_hex(config, "styling", "word2", "0x00007F", "0xffffff", "true", types[GEANY_FILETYPES_JAVA]->styling[6]);
	styleset_get_hex(config, "styling", "string", "0x1E90FF", "0xffffff", "false", types[GEANY_FILETYPES_JAVA]->styling[7]);
	styleset_get_hex(config, "styling", "character", "0x004040", "0xffffff", "false", types[GEANY_FILETYPES_JAVA]->styling[8]);
	styleset_get_hex(config, "styling", "uuid", "0x804040", "0xffffff", "false", types[GEANY_FILETYPES_JAVA]->styling[9]);
	styleset_get_hex(config, "styling", "preprocessor", "0x7F7F00", "0xffffff", "false", types[GEANY_FILETYPES_JAVA]->styling[10]);
	styleset_get_hex(config, "styling", "operator", "0x101030", "0xffffff", "false", types[GEANY_FILETYPES_JAVA]->styling[11]);
	styleset_get_hex(config, "styling", "identifier", "0x100000", "0xffffff", "false", types[GEANY_FILETYPES_JAVA]->styling[12]);
	styleset_get_hex(config, "styling", "stringeol", "0x000000", "0xe0c0e0", "false", types[GEANY_FILETYPES_JAVA]->styling[13]);
	styleset_get_hex(config, "styling", "verbatim", "0x906040", "0x0000ff", "false", types[GEANY_FILETYPES_JAVA]->styling[14]);
	styleset_get_hex(config, "styling", "regex", "0x905010", "0xffffff", "false", types[GEANY_FILETYPES_JAVA]->styling[15]);
	styleset_get_hex(config, "styling", "commentlinedoc", "0x0000ff", "0xffffff", "true", types[GEANY_FILETYPES_JAVA]->styling[16]);
	styleset_get_hex(config, "styling", "commentdockeyword", "0x0000ff", "0xffffff", "true", types[GEANY_FILETYPES_JAVA]->styling[17]);
	styleset_get_hex(config, "styling", "globalclass", "0x109040", "0xffffff", "true", types[GEANY_FILETYPES_JAVA]->styling[18]);

	types[GEANY_FILETYPES_JAVA]->keywords = g_new(gchar*, 5);
	types[GEANY_FILETYPES_JAVA]->keywords[0] = styleset_get_string(config, "keywords", "primary");
	if (types[GEANY_FILETYPES_JAVA]->keywords[0] == NULL)
			types[GEANY_FILETYPES_JAVA]->keywords[0] = g_strdup("	abstract assert break case catch class \
										const continue default do else extends final finally for future \
										generic goto if implements import inner instanceof interface \
										native new outer package private protected public rest \
										return static super switch synchronized this throw throws \
										transient try var volatile while");
	types[GEANY_FILETYPES_JAVA]->keywords[1] = styleset_get_string(config, "keywords", "secondary");
	if (types[GEANY_FILETYPES_JAVA]->keywords[1] == NULL)
			types[GEANY_FILETYPES_JAVA]->keywords[1] = g_strdup("boolean byte char double float int long null short void NULL");
	types[GEANY_FILETYPES_JAVA]->keywords[2] = styleset_get_string(config, "keywords", "doccomment");
	if (types[GEANY_FILETYPES_JAVA]->keywords[2] == NULL)
			types[GEANY_FILETYPES_JAVA]->keywords[2] = g_strdup("return param author");
	types[GEANY_FILETYPES_JAVA]->keywords[3] = styleset_get_string(config, "keywords", "typedefs");
	if (types[GEANY_FILETYPES_JAVA]->keywords[3] == NULL)
			types[GEANY_FILETYPES_JAVA]->keywords[3] = g_strdup("");
	types[GEANY_FILETYPES_JAVA]->keywords[4] = NULL;

	g_key_file_free(config);
}


void styleset_java(ScintillaObject *sci)
{
	if (types[GEANY_FILETYPES_JAVA] == NULL) styleset_java_init();

	styleset_common(sci, 5);
	SSM (sci, SCI_SETLEXER, SCLEX_CPP, 0);

	SSM (sci, SCI_SETWORDCHARS, 0, (sptr_t) GEANY_WORDCHARS);
	SSM (sci, SCI_AUTOCSETMAXHEIGHT, 8, 0);

	SSM (sci, SCI_SETCONTROLCHARSYMBOL, 32, 0);

	SSM(sci, SCI_SETKEYWORDS, 0, (sptr_t) types[GEANY_FILETYPES_JAVA]->keywords[0]);
	SSM(sci, SCI_SETKEYWORDS, 1, (sptr_t) types[GEANY_FILETYPES_JAVA]->keywords[1]);
	SSM(sci, SCI_SETKEYWORDS, 2, (sptr_t) types[GEANY_FILETYPES_JAVA]->keywords[2]);
	SSM(sci, SCI_SETKEYWORDS, 4, (sptr_t) types[GEANY_FILETYPES_JAVA]->keywords[3]);

	styleset_set_style(sci, SCE_C_DEFAULT, GEANY_FILETYPES_JAVA, 0);
	styleset_set_style(sci, SCE_C_COMMENT, GEANY_FILETYPES_JAVA, 1);
	styleset_set_style(sci, SCE_C_COMMENTLINE, GEANY_FILETYPES_JAVA, 2);
	styleset_set_style(sci, SCE_C_COMMENTDOC, GEANY_FILETYPES_JAVA, 3);
	styleset_set_style(sci, SCE_C_DEFAULT, GEANY_FILETYPES_JAVA, 4);
	styleset_set_style(sci, SCE_C_WORD, GEANY_FILETYPES_JAVA, 5);
	styleset_set_style(sci, SCE_C_WORD2, GEANY_FILETYPES_JAVA, 6);
	styleset_set_style(sci, SCE_C_STRING, GEANY_FILETYPES_JAVA, 7);
	styleset_set_style(sci, SCE_C_CHARACTER, GEANY_FILETYPES_JAVA, 8);
	styleset_set_style(sci, SCE_C_UUID, GEANY_FILETYPES_JAVA, 9);
	styleset_set_style(sci, SCE_C_PREPROCESSOR, GEANY_FILETYPES_JAVA, 10);
	styleset_set_style(sci, SCE_C_OPERATOR, GEANY_FILETYPES_JAVA, 11);
	styleset_set_style(sci, SCE_C_IDENTIFIER, GEANY_FILETYPES_JAVA, 12);
	styleset_set_style(sci, SCE_C_STRINGEOL, GEANY_FILETYPES_JAVA, 13);
	styleset_set_style(sci, SCE_C_VERBATIM, GEANY_FILETYPES_JAVA, 14);
	styleset_set_style(sci, SCE_C_REGEX, GEANY_FILETYPES_JAVA, 15);
	SSM(sci, SCI_STYLESETITALIC, SCE_C_COMMENTLINEDOC, TRUE);
	styleset_set_style(sci, SCE_C_COMMENTLINEDOC, GEANY_FILETYPES_JAVA, 16);
	SSM(sci, SCI_STYLESETITALIC, SCE_C_COMMENTDOCKEYWORD, TRUE);
	styleset_set_style(sci, SCE_C_COMMENTDOCKEYWORD, GEANY_FILETYPES_JAVA, 17);

	SSM(sci, SCI_STYLESETFORE, SCE_C_COMMENTDOCKEYWORDERROR, 0x0000ff);
	SSM(sci, SCI_STYLESETBACK, SCE_C_COMMENTDOCKEYWORDERROR, 0xFFFFFF);
	SSM(sci, SCI_STYLESETITALIC, SCE_C_COMMENTDOCKEYWORDERROR, TRUE);

	styleset_set_style(sci, SCE_C_GLOBALCLASS, GEANY_FILETYPES_JAVA, 18);

	SSM(sci, SCI_SETWHITESPACEFORE, 1, 0xc0c0c0);
}


static void styleset_perl_init(void)
{
	GKeyFile *config = g_key_file_new();

	style_set_load_file(config, GEANY_DATA_DIR "/filetypes.perl", G_KEY_FILE_KEEP_COMMENTS, NULL);

	types[GEANY_FILETYPES_PERL] = g_new(style_set, 1);
	styleset_get_hex(config, "styling", "default", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_PERL]->styling[0]);
	styleset_get_hex(config, "styling", "error", "0x0000ff", "0xffffff", "false", types[GEANY_FILETYPES_PERL]->styling[1]);
	styleset_get_hex(config, "styling", "commentline", "0x0000ff", "0xffffff", "false", types[GEANY_FILETYPES_PERL]->styling[2]);
	styleset_get_hex(config, "styling", "number", "0x007F00", "0xffffff", "false", types[GEANY_FILETYPES_PERL]->styling[3]);
	styleset_get_hex(config, "styling", "word", "0x991111", "0xffffff", "true", types[GEANY_FILETYPES_PERL]->styling[4]);
	styleset_get_hex(config, "styling", "string", "0x1E90FF", "0xffffff", "false", types[GEANY_FILETYPES_PERL]->styling[5]);
	styleset_get_hex(config, "styling", "character", "0x1E90FF", "0xffffff", "false", types[GEANY_FILETYPES_PERL]->styling[6]);
	styleset_get_hex(config, "styling", "preprocessor", "0x7F7F00", "0xffffff", "false", types[GEANY_FILETYPES_PERL]->styling[7]);
	styleset_get_hex(config, "styling", "operator", "0x101030", "0xffffff", "false", types[GEANY_FILETYPES_PERL]->styling[8]);
	styleset_get_hex(config, "styling", "identifier", "0x100000", "0xffffff", "false", types[GEANY_FILETYPES_PERL]->styling[9]);
	styleset_get_hex(config, "styling", "scalar", "0x00007F", "0xffffff", "false", types[GEANY_FILETYPES_PERL]->styling[10]);
	styleset_get_hex(config, "styling", "pod", "0x000000", "0xe0c0e0", "false", types[GEANY_FILETYPES_PERL]->styling[11]);
	styleset_get_hex(config, "styling", "regex", "0x905010", "0xffffff", "false", types[GEANY_FILETYPES_PERL]->styling[12]);
	styleset_get_hex(config, "styling", "array", "0x905010", "0xffffff", "false", types[GEANY_FILETYPES_PERL]->styling[13]);
	styleset_get_hex(config, "styling", "hash", "0x905010", "0xffffff", "false", types[GEANY_FILETYPES_PERL]->styling[14]);
	styleset_get_hex(config, "styling", "symboltable", "0x905010", "0xffffff", "false", types[GEANY_FILETYPES_PERL]->styling[15]);
	styleset_get_hex(config, "styling", "backticks", "0x000000", "0xe0c0e0", "false", types[GEANY_FILETYPES_PERL]->styling[16]);

	types[GEANY_FILETYPES_PERL]->keywords = g_new(gchar*, 2);
	types[GEANY_FILETYPES_PERL]->keywords[0] = styleset_get_string(config, "keywords", "primary");
	if (types[GEANY_FILETYPES_PERL]->keywords[0] == NULL)
			types[GEANY_FILETYPES_PERL]->keywords[0] = g_strdup(
									"NULL __FILE__ __LINE__ __PACKAGE__ __DATA__ __END__ AUTOLOAD \
									BEGIN CORE DESTROY END EQ GE GT INIT LE LT NE CHECK abs accept \
									alarm and atan2 bind binmode bless caller chdir chmod chomp chop \
									chown chr chroot close closedir cmp connect continue cos crypt \
									dbmclose dbmopen defined delete die do dump each else elsif endgrent \
									endhostent endnetent endprotoent endpwent endservent eof eq eval \
									exec exists exit exp fcntl fileno flock for foreach fork format \
									formline ge getc getgrent getgrgid getgrnam gethostbyaddr gethostbyname \
									gethostent getlogin getnetbyaddr getnetbyname getnetent getpeername \
									getpgrp getppid getpriority getprotobyname getprotobynumber getprotoent \
									getpwent getpwnam getpwuid getservbyname getservbyport getservent \
									getsockname getsockopt glob gmtime goto grep gt hex if index \
									int ioctl join keys kill last lc lcfirst le length link listen \
									local localtime lock log lstat lt m map mkdir msgctl msgget msgrcv \
									msgsnd my ne next no not oct open opendir or ord our pack package \
									pipe pop pos print printf prototype push q qq qr quotemeta qu \
									qw qx rand read readdir readline readlink readpipe recv redo \
									ref rename require reset return reverse rewinddir rindex rmdir \
									s scalar seek seekdir select semctl semget semop send setgrent \
									sethostent setnetent setpgrp setpriority setprotoent setpwent \
									setservent setsockopt shift shmctl shmget shmread shmwrite shutdown \
									sin sleep socket socketpair sort splice split sprintf sqrt srand \
									stat study sub substr symlink syscall sysopen sysread sysseek \
									system syswrite tell telldir tie tied time times tr truncate \
									uc ucfirst umask undef unless unlink unpack unshift untie until \
									use utime values vec wait waitpid wantarray warn while write \
									x xor y");
	types[GEANY_FILETYPES_PERL]->keywords[1] = NULL;

	g_key_file_free(config);
}


void styleset_perl(ScintillaObject *sci)
{
	if (types[GEANY_FILETYPES_PERL] == NULL) styleset_perl_init();

	styleset_common(sci, 5);
	SSM (sci, SCI_SETLEXER, SCLEX_PERL, 0);

	SSM(sci, SCI_SETPROPERTY, (sptr_t) "styling.within.preprocessor", (sptr_t) "1");

	SSM (sci, SCI_SETWORDCHARS, 0, (sptr_t) GEANY_WORDCHARS);
	SSM (sci, SCI_AUTOCSETMAXHEIGHT, 8, 0);

	SSM(sci, SCI_SETKEYWORDS, 0, (sptr_t) types[GEANY_FILETYPES_PERL]->keywords[0]);

	styleset_set_style(sci, SCE_PL_DEFAULT, GEANY_FILETYPES_PERL, 0);
	styleset_set_style(sci, SCE_PL_ERROR, GEANY_FILETYPES_PERL, 1);
	styleset_set_style(sci, SCE_PL_COMMENTLINE, GEANY_FILETYPES_PERL, 2);
	styleset_set_style(sci, SCE_PL_NUMBER, GEANY_FILETYPES_PERL, 3);
	styleset_set_style(sci, SCE_PL_WORD, GEANY_FILETYPES_PERL, 4);
	styleset_set_style(sci, SCE_PL_STRING, GEANY_FILETYPES_PERL, 5);
	styleset_set_style(sci, SCE_PL_CHARACTER, GEANY_FILETYPES_PERL, 6);
	styleset_set_style(sci, SCE_PL_PREPROCESSOR, GEANY_FILETYPES_PERL, 7);
	styleset_set_style(sci, SCE_PL_OPERATOR, GEANY_FILETYPES_PERL, 8);
	styleset_set_style(sci, SCE_PL_IDENTIFIER, GEANY_FILETYPES_PERL, 9);
	styleset_set_style(sci, SCE_P_DEFAULT, GEANY_FILETYPES_PERL, 10);
	styleset_set_style(sci, SCE_PL_POD, GEANY_FILETYPES_PERL, 11);
	styleset_set_style(sci, SCE_PL_REGEX, GEANY_FILETYPES_PERL, 12);
	styleset_set_style(sci, SCE_PL_ARRAY, GEANY_FILETYPES_PERL, 13);
	styleset_set_style(sci, SCE_PL_BACKTICKS, GEANY_FILETYPES_PERL, 14);
	styleset_set_style(sci, SCE_PL_HASH, GEANY_FILETYPES_PERL, 15);
	styleset_set_style(sci, SCE_PL_SYMBOLTABLE, GEANY_FILETYPES_PERL, 16);

	SSM(sci, SCI_SETWHITESPACEFORE, 1, 0xc0c0c0);
}


static void styleset_python_init(void)
{
	GKeyFile *config = g_key_file_new();

	style_set_load_file(config, GEANY_DATA_DIR "/filetypes.python", G_KEY_FILE_KEEP_COMMENTS, NULL);

	types[GEANY_FILETYPES_PYTHON] = g_new(style_set, 1);
	styleset_get_hex(config, "styling", "default", "0xFF0000", "0xffffff", "false", types[GEANY_FILETYPES_PYTHON]->styling[0]);
	styleset_get_hex(config, "styling", "commentline", "0xFF0000", "0xffffff", "false", types[GEANY_FILETYPES_PYTHON]->styling[1]);
	styleset_get_hex(config, "styling", "number", "0x800040", "0xffffff", "false", types[GEANY_FILETYPES_PYTHON]->styling[2]);
	styleset_get_hex(config, "styling", "string", "0x008000", "0xffffff", "false", types[GEANY_FILETYPES_PYTHON]->styling[3]);
	styleset_get_hex(config, "styling", "character", "0x008000", "0xffffff", "false", types[GEANY_FILETYPES_PYTHON]->styling[4]);
	styleset_get_hex(config, "styling", "word", "0x800060", "0xffffff", "true", types[GEANY_FILETYPES_PYTHON]->styling[5]);
	styleset_get_hex(config, "styling", "triple", "0x208000", "0xffffff", "false", types[GEANY_FILETYPES_PYTHON]->styling[6]);
	styleset_get_hex(config, "styling", "tripledouble", "0x004040", "0xffffff", "false", types[GEANY_FILETYPES_PYTHON]->styling[7]);
	styleset_get_hex(config, "styling", "classname", "0x303000", "0xffffff", "false", types[GEANY_FILETYPES_PYTHON]->styling[8]);
	styleset_get_hex(config, "styling", "defname", "0x800000", "0xffffff", "false", types[GEANY_FILETYPES_PYTHON]->styling[9]);
	styleset_get_hex(config, "styling", "operator", "0x800030", "0xffffff", "false", types[GEANY_FILETYPES_PYTHON]->styling[10]);
	styleset_get_hex(config, "styling", "identifier", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_PYTHON]->styling[11]);
	styleset_get_hex(config, "styling", "commentblock", "0x808080", "0xffffff", "false", types[GEANY_FILETYPES_PYTHON]->styling[12]);
	styleset_get_hex(config, "styling", "stringeol", "0x000000", "0xe0c0e0", "false", types[GEANY_FILETYPES_PYTHON]->styling[13]);

	types[GEANY_FILETYPES_PYTHON]->keywords = g_new(gchar*, 2);
	types[GEANY_FILETYPES_PYTHON]->keywords[0] = styleset_get_string(config, "keywords", "primary");
	if (types[GEANY_FILETYPES_PYTHON]->keywords[0] == NULL)
			types[GEANY_FILETYPES_PYTHON]->keywords[0] = g_strdup("and assert break class continue def del elif else except exec finally for from global if import in is lambda not or pass print raise return try while yield");
	types[GEANY_FILETYPES_PYTHON]->keywords[1] = NULL;

	g_key_file_free(config);
}


void styleset_python(ScintillaObject *sci)
{
	if (types[GEANY_FILETYPES_PYTHON] == NULL) styleset_python_init();

	styleset_common(sci, 5);
	SSM (sci, SCI_SETLEXER, SCLEX_PYTHON, 0);

	SSM(sci, SCI_SETKEYWORDS, 0, (sptr_t) types[GEANY_FILETYPES_PYTHON]->keywords[0]);

	styleset_set_style(sci, SCE_P_DEFAULT, GEANY_FILETYPES_PYTHON, 0);
	styleset_set_style(sci, SCE_P_COMMENTLINE, GEANY_FILETYPES_PYTHON, 1);
	styleset_set_style(sci, SCE_P_NUMBER, GEANY_FILETYPES_PYTHON, 2);
	styleset_set_style(sci, SCE_P_STRING, GEANY_FILETYPES_PYTHON, 3);
	styleset_set_style(sci, SCE_P_CHARACTER, GEANY_FILETYPES_PYTHON, 4);
	styleset_set_style(sci, SCE_P_WORD, GEANY_FILETYPES_PYTHON, 5);
	styleset_set_style(sci, SCE_P_TRIPLE, GEANY_FILETYPES_PYTHON, 6);
	styleset_set_style(sci, SCE_P_TRIPLEDOUBLE, GEANY_FILETYPES_PYTHON, 7);
	styleset_set_style(sci, SCE_P_CLASSNAME, GEANY_FILETYPES_PYTHON, 8);
	styleset_set_style(sci, SCE_P_DEFNAME, GEANY_FILETYPES_PYTHON, 9);
	styleset_set_style(sci, SCE_P_OPERATOR, GEANY_FILETYPES_PYTHON, 10);
	styleset_set_style(sci, SCE_P_IDENTIFIER, GEANY_FILETYPES_PYTHON, 11);
	styleset_set_style(sci, SCE_P_COMMENTBLOCK, GEANY_FILETYPES_PYTHON, 12);
	styleset_set_style(sci, SCE_P_STRINGEOL, GEANY_FILETYPES_PYTHON, 13);
}


static void styleset_sh_init(void)
{
	GKeyFile *config = g_key_file_new();

	style_set_load_file(config, GEANY_DATA_DIR "/filetypes.sh", G_KEY_FILE_KEEP_COMMENTS, NULL);

	types[GEANY_FILETYPES_SH] = g_new(style_set, 1);
	styleset_get_hex(config, "styling", "default", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_SH]->styling[0]);
	styleset_get_hex(config, "styling", "commentline", "0x0000ff", "0xffffff", "false", types[GEANY_FILETYPES_SH]->styling[1]);
	styleset_get_hex(config, "styling", "number", "0x007F00", "0xffffff", "false", types[GEANY_FILETYPES_SH]->styling[2]);
	styleset_get_hex(config, "styling", "word", "0x991111", "0xffffff", "true", types[GEANY_FILETYPES_SH]->styling[3]);
	styleset_get_hex(config, "styling", "string", "0x1E90FF", "0xffffff", "false", types[GEANY_FILETYPES_SH]->styling[4]);
	styleset_get_hex(config, "styling", "character", "0x004040", "0xffffff", "false", types[GEANY_FILETYPES_SH]->styling[5]);
	styleset_get_hex(config, "styling", "operator", "0x101030", "0xffffff", "false", types[GEANY_FILETYPES_SH]->styling[6]);
	styleset_get_hex(config, "styling", "identifier", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_SH]->styling[7]);
	styleset_get_hex(config, "styling", "backticks", "0x000000", "0xe0c0e0", "false", types[GEANY_FILETYPES_SH]->styling[8]);
	styleset_get_hex(config, "styling", "param", "0xF38A12", "0x0000ff", "false", types[GEANY_FILETYPES_SH]->styling[9]);
	styleset_get_hex(config, "styling", "scalar", "0x905010", "0xffffff", "false", types[GEANY_FILETYPES_SH]->styling[10]);

	types[GEANY_FILETYPES_SH]->keywords = g_new(gchar*, 2);
	types[GEANY_FILETYPES_SH]->keywords[0] = styleset_get_string(config, "keywords", "primary");
	if (types[GEANY_FILETYPES_SH]->keywords[0] == NULL)
			types[GEANY_FILETYPES_SH]->keywords[0] = g_strdup("break case continue do done else esac eval exit export fi for goto if in integer return set shift then while");
	types[GEANY_FILETYPES_SH]->keywords[1] = NULL;

	g_key_file_free(config);
}


void styleset_sh(ScintillaObject *sci)
{
	if (types[GEANY_FILETYPES_SH] == NULL) styleset_sh_init();

	styleset_common(sci, 5);
	SSM (sci, SCI_SETLEXER, SCLEX_BASH, 0);

	SSM (sci, SCI_SETWORDCHARS, 0, (sptr_t) GEANY_WORDCHARS);
	SSM (sci, SCI_AUTOCSETMAXHEIGHT, 8, 0);

	SSM (sci, SCI_SETCONTROLCHARSYMBOL, 32, 0);

	SSM(sci, SCI_SETKEYWORDS, 0, (sptr_t) types[GEANY_FILETYPES_SH]->keywords[0]);

	styleset_set_style(sci, SCE_SH_DEFAULT, GEANY_FILETYPES_SH, 0);
	styleset_set_style(sci, SCE_SH_COMMENTLINE, GEANY_FILETYPES_SH, 1);
	styleset_set_style(sci, SCE_SH_NUMBER, GEANY_FILETYPES_SH, 2);
	styleset_set_style(sci, SCE_SH_WORD, GEANY_FILETYPES_SH, 3);
	styleset_set_style(sci, SCE_SH_STRING, GEANY_FILETYPES_SH, 4);
	styleset_set_style(sci, SCE_SH_CHARACTER, GEANY_FILETYPES_SH, 5);
	styleset_set_style(sci, SCE_SH_OPERATOR, GEANY_FILETYPES_SH, 6);
	styleset_set_style(sci, SCE_SH_IDENTIFIER, GEANY_FILETYPES_SH, 7);
	styleset_set_style(sci, SCE_SH_BACKTICKS, GEANY_FILETYPES_SH, 8);
	styleset_set_style(sci, SCE_SH_PARAM, GEANY_FILETYPES_SH, 9);
	styleset_set_style(sci, SCE_SH_SCALAR, GEANY_FILETYPES_SH, 10);

	SSM(sci, SCI_SETWHITESPACEFORE, 1, 0xc0c0c0);
}


void styleset_xml(ScintillaObject *sci)
{
	styleset_common(sci, 7);

	SSM (sci, SCI_SETLEXER, SCLEX_XML, 0);

	// use the same colouring for HTML; XML and so on
	styleset_markup(sci);
}


static void styleset_docbook_init(void)
{
	GKeyFile *config = g_key_file_new();

	style_set_load_file(config, GEANY_DATA_DIR "/filetypes.docbook", G_KEY_FILE_KEEP_COMMENTS, NULL);

	types[GEANY_FILETYPES_DOCBOOK] = g_new(style_set, 1);
	styleset_get_hex(config, "styling", "default", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[0]);
	styleset_get_hex(config, "styling", "tag", "0x990000", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[1]);
	styleset_get_hex(config, "styling", "tagunknown", "0x0000ff", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[2]);
	styleset_get_hex(config, "styling", "attribute", "0x007F00", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[3]);
	styleset_get_hex(config, "styling", "attributeunknown", "0x0000ff", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[4]);
	styleset_get_hex(config, "styling", "number", "0x800080", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[5]);
	styleset_get_hex(config, "styling", "doublestring", "0x1E90FF", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[6]);
	styleset_get_hex(config, "styling", "singlestring", "0x1E90FF", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[7]);
	styleset_get_hex(config, "styling", "other", "0x800080", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[8]);
	styleset_get_hex(config, "styling", "comment", "0x808080", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[9]);
	styleset_get_hex(config, "styling", "entity", "0x800080", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[10]);
	styleset_get_hex(config, "styling", "tagend", "0x990000", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[11]);
	styleset_get_hex(config, "styling", "xmlstart", "0x990000", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[12]);
	styleset_get_hex(config, "styling", "xmlend", "0x990000", "0xf0f0f0", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[13]);
	styleset_get_hex(config, "styling", "cdata", "0x990066", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[14]);
	styleset_get_hex(config, "styling", "question", "0xff0000", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[15]);
	styleset_get_hex(config, "styling", "value", "0x990066", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[16]);
	styleset_get_hex(config, "styling", "xccomment", "0x990066", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[17]);
	styleset_get_hex(config, "styling", "sgml_default", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[18]);
	styleset_get_hex(config, "styling", "sgml_comment", "0x808080", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[19]);
	styleset_get_hex(config, "styling", "sgml_special", "0x007F00", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[20]);
	styleset_get_hex(config, "styling", "sgml_command", "0x991111", "0xffffff", "true", types[GEANY_FILETYPES_DOCBOOK]->styling[21]);
	styleset_get_hex(config, "styling", "sgml_doublestring", "0x1E90FF", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[22]);
	styleset_get_hex(config, "styling", "sgml_simplestring", "0x004040", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[23]);
	styleset_get_hex(config, "styling", "sgml_1st_param", "0x804040", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[24]);
	styleset_get_hex(config, "styling", "sgml_entity", "0x101030", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[25]);
	styleset_get_hex(config, "styling", "sgml_block_default", "0x100000", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[26]);
	styleset_get_hex(config, "styling", "sgml_1st_param_comment", "0x906040", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[27]);
	styleset_get_hex(config, "styling", "sgml_error", "0x0000ff", "0xffffff", "false", types[GEANY_FILETYPES_DOCBOOK]->styling[28]);

	types[GEANY_FILETYPES_DOCBOOK]->keywords = g_new(gchar*, 2);
	types[GEANY_FILETYPES_DOCBOOK]->keywords[0] = styleset_get_string(config, "keywords", "elements");
	if (types[GEANY_FILETYPES_DOCBOOK]->keywords[0] == NULL)
			types[GEANY_FILETYPES_DOCBOOK]->keywords[0] = g_strdup("\
			abbrev abstract accel ackno acronym action address affiliation alt anchor \
			answer appendix appendixinfo application area areaset areaspec arg article \
			articleinfo artpagenums attribution audiodata audioobject author authorblurb \
			authorgroup authorinitials beginpage bibliocoverage bibliodiv biblioentry \
			bibliography bibliographyinfo biblioid bibliomisc bibliomixed bibliomset \
			bibliorelation biblioset bibliosource blockinfo blockquote book bookinfo \
			bridgehead callout calloutlist caption caution chapter chapterinfo citation \
			citebiblioid citerefentry citetitle city classname classsynopsis classsynopsisinfo \
			cmdsynopsis co collab collabname colophon nameend namest colname colspec command computeroutput \
			confdates confgroup confnum confsponsor conftitle constant constraint \
			constraintdef constructorsynopsis contractnum contractsponsor contrib \
			copyright coref corpauthor corpname country database date dedication \
			destructorsynopsis edition editor email emphasis entry entrytbl envar \
			epigraph equation errorcode errorname errortext errortype example \
			exceptionname fax fieldsynopsis figure filename fileref firstname firstterm \
			footnote footnoteref foreignphrase formalpara frame funcdef funcparams \
			funcprototype funcsynopsis funcsynopsisinfo function glossary glossaryinfo \
			glossdef glossdiv glossentry glosslist glosssee glossseealso glossterm \
			graphic graphicco group guibutton guiicon guilabel guimenu guimenuitem \
			guisubmenu hardware highlights holder honorific htm imagedata imageobject \
			imageobjectco important index indexdiv indexentry indexinfo indexterm \
			informalequation informalexample informalfigure informaltable initializer \
			inlineequation inlinegraphic inlinemediaobject interface interfacename \
			invpartnumber isbn issn issuenum itemizedlist itermset jobtitle keycap \
			keycode keycombo keysym keyword keywordset label legalnotice lhs lineage \
			lineannotation link listitem iteral literallayout lot lotentry manvolnum \
			markup medialabel mediaobject mediaobjectco member menuchoice methodname \
			methodparam methodsynopsis mm modespec modifier ousebutton msg msgaud \
			msgentry msgexplan msginfo msglevel msgmain msgorig msgrel msgset msgsub \
			msgtext nonterminal note objectinfo olink ooclass ooexception oointerface \
			option optional orderedlist orgdiv orgname otheraddr othercredit othername \
			pagenums para paramdef parameter part partinfo partintro personblurb \
			personname phone phrase pob postcode preface prefaceinfo primary primaryie \
			printhistory procedure production productionrecap productionset productname \
			productnumber programlisting programlistingco prompt property pubdate publisher \
			publishername pubsnumber qandadiv qandaentry qandaset question quote refclass \
			refdescriptor refentry refentryinfo refentrytitle reference referenceinfo \
			refmeta refmiscinfo refname refnamediv refpurpose refsect1 refsect1info refsect2 \
			refsect2info refsect3 refsect3info refsection refsectioninfo refsynopsisdiv \
			refsynopsisdivinfo releaseinfo remark replaceable returnvalue revdescription \
			revhistory revision revnumber revremark rhs row sbr screen screenco screeninfo \
			screenshot secondary secondaryie sect1 sect1info sect2 sect2info sect3 sect3info \
			sect4 sect4info sect5 sect5info section sectioninfo see seealso seealsoie \
			seeie seg seglistitem segmentedlist segtitle seriesvolnums set setindex \
			setindexinfo setinfo sgmltag shortaffil shortcut sidebar sidebarinfo simpara \
			simplelist simplemsgentry simplesect spanspec state step street structfield \
			structname subject subjectset subjectterm subscript substeps subtitle \
			superscript surname sv symbol synopfragment synopfragmentref synopsis \
			systemitem table tbody term tertiary tertiaryie textdata textobject tfoot \
			tgroup thead tip title titleabbrev toc tocback tocchap tocentry tocfront \
			toclevel1 toclevel2 toclevel3 toclevel4 toclevel5 tocpart token trademark \
			type ulink userinput varargs variablelist varlistentry varname videodata \
			videoobject void volumenum warning wordasword xref year cols colnum align spanname\
			arch condition conformance id lang os remap role revision revisionflag security \
			userlevel url vendor xreflabel status label endterm linkend space width");
	types[GEANY_FILETYPES_DOCBOOK]->keywords[1] = styleset_get_string(config, "keywords", "dtd");
	if (types[GEANY_FILETYPES_DOCBOOK]->keywords[1] == NULL)
			types[GEANY_FILETYPES_DOCBOOK]->keywords[1] = g_strdup("ELEMENT DOCTYPE ATTLIST ENTITY NOTATION");
	types[GEANY_FILETYPES_DOCBOOK]->keywords[2] = NULL;

	g_key_file_free(config);
}


void styleset_docbook(ScintillaObject *sci)
{
	if (types[GEANY_FILETYPES_DOCBOOK] == NULL) styleset_docbook_init();

	styleset_common(sci, 7);
	SSM (sci, SCI_SETLEXER, SCLEX_XML, 0);

	SSM(sci, SCI_SETKEYWORDS, 0, (sptr_t) types[GEANY_FILETYPES_DOCBOOK]->keywords[0]);
	SSM(sci, SCI_SETKEYWORDS, 5, (sptr_t) types[GEANY_FILETYPES_DOCBOOK]->keywords[1]);

	SSM (sci, SCI_SETWORDCHARS, 0, (sptr_t) GEANY_WORDCHARS);
	SSM (sci, SCI_AUTOCSETMAXHEIGHT, 8, 0);

	// Unknown tags and attributes are highlighed in red.
	// If a tag is actually OK, it should be added in lower case to the htmlKeyWords string.

	styleset_set_style(sci, SCE_H_DEFAULT, GEANY_FILETYPES_DOCBOOK, 0);
	styleset_set_style(sci, SCE_H_TAG, GEANY_FILETYPES_DOCBOOK, 1);
	styleset_set_style(sci, SCE_H_TAGUNKNOWN, GEANY_FILETYPES_DOCBOOK, 2);
	styleset_set_style(sci, SCE_H_ATTRIBUTE, GEANY_FILETYPES_DOCBOOK, 3);
	styleset_set_style(sci, SCE_H_ATTRIBUTEUNKNOWN, GEANY_FILETYPES_DOCBOOK, 4);
	styleset_set_style(sci, SCE_H_NUMBER, GEANY_FILETYPES_DOCBOOK, 5);
	styleset_set_style(sci, SCE_H_DOUBLESTRING, GEANY_FILETYPES_DOCBOOK, 6);
	styleset_set_style(sci, SCE_H_SINGLESTRING, GEANY_FILETYPES_DOCBOOK, 7);
	styleset_set_style(sci, SCE_H_OTHER, GEANY_FILETYPES_DOCBOOK, 8);
	styleset_set_style(sci, SCE_H_COMMENT, GEANY_FILETYPES_DOCBOOK, 9);
	styleset_set_style(sci, SCE_H_ENTITY, GEANY_FILETYPES_DOCBOOK, 10);
	styleset_set_style(sci, SCE_H_TAGEND, GEANY_FILETYPES_DOCBOOK, 11);
	SSM(sci, SCI_STYLESETEOLFILLED, SCE_H_XMLSTART, 1);
	styleset_set_style(sci, SCE_H_XMLSTART, GEANY_FILETYPES_DOCBOOK, 12);
	styleset_set_style(sci, SCE_H_XMLEND, GEANY_FILETYPES_DOCBOOK, 13);
	styleset_set_style(sci, SCE_H_CDATA, GEANY_FILETYPES_DOCBOOK, 14);
	styleset_set_style(sci, SCE_H_QUESTION, GEANY_FILETYPES_DOCBOOK, 15);
	styleset_set_style(sci, SCE_H_VALUE, GEANY_FILETYPES_DOCBOOK, 16);
	styleset_set_style(sci, SCE_H_XCCOMMENT, GEANY_FILETYPES_DOCBOOK, 17);
	styleset_set_style(sci, SCE_H_SGML_DEFAULT, GEANY_FILETYPES_DOCBOOK, 18);
	styleset_set_style(sci, SCE_H_DEFAULT, GEANY_FILETYPES_DOCBOOK, 19);
	styleset_set_style(sci, SCE_H_SGML_SPECIAL, GEANY_FILETYPES_DOCBOOK, 20);
	styleset_set_style(sci, SCE_H_SGML_COMMAND, GEANY_FILETYPES_DOCBOOK, 21);
	styleset_set_style(sci, SCE_H_SGML_DOUBLESTRING, GEANY_FILETYPES_DOCBOOK, 22);
	styleset_set_style(sci, SCE_H_SGML_SIMPLESTRING, GEANY_FILETYPES_DOCBOOK, 23);
	styleset_set_style(sci, SCE_H_SGML_1ST_PARAM, GEANY_FILETYPES_DOCBOOK, 24);
	styleset_set_style(sci, SCE_H_SGML_ENTITY, GEANY_FILETYPES_DOCBOOK, 25);
	styleset_set_style(sci, SCE_H_SGML_BLOCK_DEFAULT, GEANY_FILETYPES_DOCBOOK, 26);
	styleset_set_style(sci, SCE_H_SGML_1ST_PARAM_COMMENT, GEANY_FILETYPES_DOCBOOK, 27);
	styleset_set_style(sci, SCE_H_SGML_ERROR, GEANY_FILETYPES_DOCBOOK, 28);
}


void styleset_none(ScintillaObject *sci)
{
	styleset_common(sci, 5);
	SSM (sci, SCI_SETLEXER, SCLEX_NULL, 0);
}


static void styleset_css_init(void)
{
	GKeyFile *config = g_key_file_new();

	style_set_load_file(config, GEANY_DATA_DIR "/filetypes.css", G_KEY_FILE_KEEP_COMMENTS, NULL);

	types[GEANY_FILETYPES_CSS] = g_new(style_set, 1);
	styleset_get_hex(config, "styling", "default", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_CSS]->styling[0]);
	styleset_get_hex(config, "styling", "comment", "0x808080", "0xffffff", "false", types[GEANY_FILETYPES_CSS]->styling[1]);
	styleset_get_hex(config, "styling", "tag", "0x00007F", "0xffffff", "true", types[GEANY_FILETYPES_CSS]->styling[2]);
	styleset_get_hex(config, "styling", "class", "0x991111", "0xffffff", "true", types[GEANY_FILETYPES_CSS]->styling[3]);
	styleset_get_hex(config, "styling", "pseudoclass", "0x991111", "0xffffff", "true", types[GEANY_FILETYPES_CSS]->styling[4]);
	styleset_get_hex(config, "styling", "unknown_pseudoclass", "0x0000ff", "0xffffff", "false", types[GEANY_FILETYPES_CSS]->styling[5]);
	styleset_get_hex(config, "styling", "unknown_identifier", "0x0000ff", "0xffffff", "false", types[GEANY_FILETYPES_CSS]->styling[6]);
	styleset_get_hex(config, "styling", "operator", "0x101030", "0xffffff", "false", types[GEANY_FILETYPES_CSS]->styling[7]);
	styleset_get_hex(config, "styling", "identifier", "0x100000", "0xffffff", "false", types[GEANY_FILETYPES_CSS]->styling[8]);
	styleset_get_hex(config, "styling", "doublestring", "0x1E90FF", "0xffffff", "false", types[GEANY_FILETYPES_CSS]->styling[9]);
	styleset_get_hex(config, "styling", "singlestring", "0x1E90FF", "0xffffff", "false", types[GEANY_FILETYPES_CSS]->styling[10]);
	styleset_get_hex(config, "styling", "attribute", "0x007F00", "0xffffff", "false", types[GEANY_FILETYPES_CSS]->styling[11]);
	styleset_get_hex(config, "styling", "value", "0x7F7F00", "0xffffff", "false", types[GEANY_FILETYPES_CSS]->styling[12]);

	types[GEANY_FILETYPES_CSS]->keywords = g_new(gchar*, 4);
	types[GEANY_FILETYPES_CSS]->keywords[0] = styleset_get_string(config, "keywords", "primary");
	if (types[GEANY_FILETYPES_CSS]->keywords[0] == NULL)
			types[GEANY_FILETYPES_CSS]->keywords[0] = g_strdup("color background-color background-image background-repeat background-attachment background-position background \
								font-family font-style font-variant font-weight font-size font \
								word-spacing letter-spacing text-decoration vertical-align text-transform text-align text-indent line-height \
								margin-top margin-right margin-bottom margin-left margin \
								padding-top padding-right padding-bottom padding-left padding \
								border-top-width border-right-width border-bottom-width border-left-width border-width \
								border-top border-right border-bottom border-left border \
								border-color border-style width height float clear \
								display white-space list-style-type list-style-image list-style-position list-style");
	types[GEANY_FILETYPES_CSS]->keywords[1] = styleset_get_string(config, "keywords", "secondary");
	if (types[GEANY_FILETYPES_CSS]->keywords[1] == NULL)
			types[GEANY_FILETYPES_CSS]->keywords[1] = g_strdup("border-top-color border-right-color border-bottom-color border-left-color border-color \
								border-top-style border-right-style border-bottom-style border-left-style border-style \
								top right bottom left position z-index direction unicode-bidi \
								min-width max-width min-height max-height overflow clip visibility content quotes \
								counter-reset counter-increment marker-offset \
								size marks page-break-before page-break-after page-break-inside page orphans widows \
								font-stretch font-size-adjust unicode-range units-per-em src \
								panose-1 stemv stemh slope cap-height x-height ascent descent widths bbox definition-src \
								baseline centerline mathline topline text-shadow \
								caption-side table-layout border-collapse border-spacing empty-cells speak-header \
								cursor outline outline-width outline-style outline-color \
								volume speak pause-before pause-after pause cue-before cue-after cue \
								play-during azimuth elevation speech-rate voice-family pitch pitch-range stress richness \
								speak-punctuation speak-numeral");
	types[GEANY_FILETYPES_CSS]->keywords[2] = styleset_get_string(config, "keywords", "pseudoclasses");
	if (types[GEANY_FILETYPES_CSS]->keywords[2] == NULL)
			types[GEANY_FILETYPES_CSS]->keywords[2] = g_strdup("first-letter first-line link active visited lang first-child focus hover before after left right first");
	types[GEANY_FILETYPES_CSS]->keywords[3] = NULL;

	g_key_file_free(config);
}


void styleset_css(ScintillaObject *sci)
{
	if (types[GEANY_FILETYPES_CSS] == NULL) styleset_css_init();

	styleset_common(sci, 5);

	SSM(sci, SCI_SETWORDCHARS, 0, (sptr_t) GEANY_WORDCHARS);
	SSM(sci, SCI_AUTOCSETMAXHEIGHT, 8, 0);

	SSM(sci, SCI_SETLEXER, SCLEX_CSS, 0);

	styleset_set_style(sci, SCE_CSS_DEFAULT, GEANY_FILETYPES_CSS, 0);
	styleset_set_style(sci, SCE_CSS_COMMENT, GEANY_FILETYPES_CSS, 1);
	styleset_set_style(sci, SCE_CSS_TAG, GEANY_FILETYPES_CSS, 2);
	styleset_set_style(sci, SCE_CSS_CLASS, GEANY_FILETYPES_CSS, 3);
	SSM(sci, SCI_STYLESETITALIC, SCE_CSS_PSEUDOCLASS, TRUE);
	styleset_set_style(sci, SCE_CSS_PSEUDOCLASS, GEANY_FILETYPES_CSS, 4);
	styleset_set_style(sci, SCE_CSS_UNKNOWN_PSEUDOCLASS, GEANY_FILETYPES_CSS, 5);
	styleset_set_style(sci, SCE_CSS_UNKNOWN_IDENTIFIER, GEANY_FILETYPES_CSS, 6);
	styleset_set_style(sci, SCE_CSS_OPERATOR, GEANY_FILETYPES_CSS, 7);
	styleset_set_style(sci, SCE_CSS_IDENTIFIER, GEANY_FILETYPES_CSS, 8);
	styleset_set_style(sci, SCE_CSS_DOUBLESTRING, GEANY_FILETYPES_CSS, 9);
	styleset_set_style(sci, SCE_CSS_SINGLESTRING, GEANY_FILETYPES_CSS, 10);
	styleset_set_style(sci, SCE_CSS_ATTRIBUTE, GEANY_FILETYPES_CSS, 11);
	styleset_set_style(sci, SCE_CSS_VALUE, GEANY_FILETYPES_CSS, 12);
}


static void styleset_conf_init(void)
{
	GKeyFile *config = g_key_file_new();

	style_set_load_file(config, GEANY_DATA_DIR "/filetypes.conf", G_KEY_FILE_KEEP_COMMENTS, NULL);

	types[GEANY_FILETYPES_CONF] = g_new(style_set, 1);
	styleset_get_hex(config, "styling", "default", "0x00007f", "0xffffff", "false", types[GEANY_FILETYPES_CONF]->styling[0]);
	styleset_get_hex(config, "styling", "comment", "0x808080", "0xffffff", "false", types[GEANY_FILETYPES_CONF]->styling[1]);
	styleset_get_hex(config, "styling", "section", "0x900000", "0xffffff", "true", types[GEANY_FILETYPES_CONF]->styling[2]);
	styleset_get_hex(config, "styling", "assignment", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_CONF]->styling[3]);
	styleset_get_hex(config, "styling", "defval", "0x7f0000", "0xffffff", "false", types[GEANY_FILETYPES_CONF]->styling[4]);

	types[GEANY_FILETYPES_CONF]->keywords = NULL;

	g_key_file_free(config);
}


void styleset_conf(ScintillaObject *sci)
{
	if (types[GEANY_FILETYPES_CONF] == NULL) styleset_conf_init();

	styleset_common(sci, 5);
	SSM (sci, SCI_SETLEXER, SCLEX_PROPERTIES, 0);

	SSM (sci, SCI_SETWORDCHARS, 0, (sptr_t) GEANY_WORDCHARS);
	SSM (sci, SCI_AUTOCSETMAXHEIGHT, 8, 0);

	SSM (sci, SCI_SETCONTROLCHARSYMBOL, 32, 0);

	styleset_set_style(sci, SCE_PROPS_DEFAULT, GEANY_FILETYPES_CONF, 0);
	styleset_set_style(sci, SCE_PROPS_COMMENT, GEANY_FILETYPES_CONF, 1);
	styleset_set_style(sci, SCE_PROPS_SECTION, GEANY_FILETYPES_CONF, 2);
	styleset_set_style(sci, SCE_PROPS_ASSIGNMENT, GEANY_FILETYPES_CONF, 3);
	styleset_set_style(sci, SCE_PROPS_DEFVAL, GEANY_FILETYPES_CONF, 4);
}


static void styleset_asm_init(void)
{
	GKeyFile *config = g_key_file_new();

	style_set_load_file(config, GEANY_DATA_DIR "/filetypes.asm", G_KEY_FILE_KEEP_COMMENTS, NULL);

	types[GEANY_FILETYPES_ASM] = g_new(style_set, 1);
	styleset_get_hex(config, "styling", "default", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_ASM]->styling[0]);
	styleset_get_hex(config, "styling", "comment", "0x808080", "0xffffff", "false", types[GEANY_FILETYPES_ASM]->styling[1]);
	styleset_get_hex(config, "styling", "number", "0x007f00", "0xffffff", "false", types[GEANY_FILETYPES_ASM]->styling[2]);
	styleset_get_hex(config, "styling", "string", "0x1E90FF", "0xffffff", "false", types[GEANY_FILETYPES_ASM]->styling[3]);
	styleset_get_hex(config, "styling", "operator", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_ASM]->styling[4]);
	styleset_get_hex(config, "styling", "identifier", "0x000088", "0xffffff", "false", types[GEANY_FILETYPES_ASM]->styling[5]);
	styleset_get_hex(config, "styling", "cpuinstruction", "0x991111", "0xffffff", "true", types[GEANY_FILETYPES_ASM]->styling[6]);
	styleset_get_hex(config, "styling", "mathinstruction", "0x00007F", "0xffffff", "true", types[GEANY_FILETYPES_ASM]->styling[7]);
	styleset_get_hex(config, "styling", "register", "0x100000", "0xffffff", "false", types[GEANY_FILETYPES_ASM]->styling[8]);
	styleset_get_hex(config, "styling", "directive", "0x0F673D", "0xffffff", "true", types[GEANY_FILETYPES_ASM]->styling[9]);
	styleset_get_hex(config, "styling", "directiveoperand", "0x1E90FF", "0xffffff", "false", types[GEANY_FILETYPES_ASM]->styling[10]);
	styleset_get_hex(config, "styling", "commentblock", "0x808080", "0xffffff", "false", types[GEANY_FILETYPES_ASM]->styling[11]);
	styleset_get_hex(config, "styling", "character", "0x1E90FF", "0xffffff", "false", types[GEANY_FILETYPES_ASM]->styling[12]);
	styleset_get_hex(config, "styling", "stringeol", "0x000000", "0xe0c0e0", "false", types[GEANY_FILETYPES_ASM]->styling[13]);
	styleset_get_hex(config, "styling", "extinstruction", "0x7F7F00", "0xffffff", "false", types[GEANY_FILETYPES_ASM]->styling[14]);

	types[GEANY_FILETYPES_ASM]->keywords = g_new(gchar*, 4);
	types[GEANY_FILETYPES_ASM]->keywords[0] = styleset_get_string(config, "keywords", "instructions");
	if (types[GEANY_FILETYPES_ASM]->keywords[0] == NULL)
			types[GEANY_FILETYPES_ASM]->keywords[0] = g_strdup("HLT LAD SPI ADD SUB MUL DIV JMP JEZ JGZ JLZ SWAP JSR RET PUSHAC POPAC ADDST SUBST MULST DIVST LSA LDS PUSH POP CLI LDI INK LIA DEK LDX");
	types[GEANY_FILETYPES_ASM]->keywords[1] = styleset_get_string(config, "keywords", "registers");
	if (types[GEANY_FILETYPES_ASM]->keywords[1] == NULL)
			types[GEANY_FILETYPES_ASM]->keywords[1] = g_strdup("");
	types[GEANY_FILETYPES_ASM]->keywords[2] = styleset_get_string(config, "keywords", "directives");
	if (types[GEANY_FILETYPES_ASM]->keywords[2] == NULL)
			types[GEANY_FILETYPES_ASM]->keywords[2] = g_strdup("ORG LIST NOLIST PAGE EQUIVALENT WORD TEXT");
	types[GEANY_FILETYPES_ASM]->keywords[3] = NULL;

	g_key_file_free(config);
}


void styleset_asm(ScintillaObject *sci)
{
	if (types[GEANY_FILETYPES_ASM] == NULL) styleset_asm_init();

	styleset_common(sci, 5);

	SSM(sci, SCI_SETWORDCHARS, 0, (sptr_t) GEANY_WORDCHARS);
	SSM(sci, SCI_AUTOCSETMAXHEIGHT, 8, 0);

	SSM(sci, SCI_SETLEXER, SCLEX_ASM, 0);

	SSM(sci, SCI_SETKEYWORDS, 0, (sptr_t) types[GEANY_FILETYPES_ASM]->keywords[0]);
	//SSM(sci, SCI_SETKEYWORDS, 1, (sptr_t) types[GEANY_FILETYPES_ASM]->keywords[0]);
	SSM(sci, SCI_SETKEYWORDS, 2, (sptr_t) types[GEANY_FILETYPES_ASM]->keywords[1]);
	SSM(sci, SCI_SETKEYWORDS, 3, (sptr_t) types[GEANY_FILETYPES_ASM]->keywords[2]);
	//SSM(sci, SCI_SETKEYWORDS, 5, (sptr_t) types[GEANY_FILETYPES_ASM]->keywords[0]);

	styleset_set_style(sci, SCE_ASM_DEFAULT, GEANY_FILETYPES_ASM, 0);
	styleset_set_style(sci, SCE_ASM_COMMENT, GEANY_FILETYPES_ASM, 1);
	styleset_set_style(sci, SCE_ASM_NUMBER, GEANY_FILETYPES_ASM, 2);
	styleset_set_style(sci, SCE_ASM_STRING, GEANY_FILETYPES_ASM, 3);
	styleset_set_style(sci, SCE_ASM_OPERATOR, GEANY_FILETYPES_ASM, 4);
	styleset_set_style(sci, SCE_ASM_IDENTIFIER, GEANY_FILETYPES_ASM, 5);
	styleset_set_style(sci, SCE_ASM_CPUINSTRUCTION, GEANY_FILETYPES_ASM, 6);
	styleset_set_style(sci, SCE_ASM_MATHINSTRUCTION, GEANY_FILETYPES_ASM, 7);
	styleset_set_style(sci, SCE_ASM_REGISTER, GEANY_FILETYPES_ASM, 8);
	styleset_set_style(sci, SCE_ASM_DIRECTIVE, GEANY_FILETYPES_ASM, 9);
	styleset_set_style(sci, SCE_ASM_DIRECTIVEOPERAND, GEANY_FILETYPES_ASM, 10);
	styleset_set_style(sci, SCE_ASM_COMMENTBLOCK, GEANY_FILETYPES_ASM, 11);
	styleset_set_style(sci, SCE_ASM_CHARACTER, GEANY_FILETYPES_ASM, 12);
	styleset_set_style(sci, SCE_ASM_STRINGEOL, GEANY_FILETYPES_ASM, 13);
	styleset_set_style(sci, SCE_ASM_EXTINSTRUCTION, GEANY_FILETYPES_ASM, 14);
}


static void styleset_sql_init(void)
{
	GKeyFile *config = g_key_file_new();

	style_set_load_file(config, GEANY_DATA_DIR "/filetypes.sql", G_KEY_FILE_KEEP_COMMENTS, NULL);

	types[GEANY_FILETYPES_SQL] = g_new(style_set, 1);
	styleset_get_hex(config, "styling", "default", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_SQL]->styling[0]);
	styleset_get_hex(config, "styling", "comment", "0x808080", "0xffffff", "false", types[GEANY_FILETYPES_SQL]->styling[1]);
	styleset_get_hex(config, "styling", "commentline", "0x808080", "0xffffff", "false", types[GEANY_FILETYPES_SQL]->styling[2]);
	styleset_get_hex(config, "styling", "commentdoc", "0x808080", "0xffffff", "false", types[GEANY_FILETYPES_SQL]->styling[3]);
	styleset_get_hex(config, "styling", "number", "0x007f7f", "0xffffff", "false", types[GEANY_FILETYPES_SQL]->styling[4]);
	styleset_get_hex(config, "styling", "word", "0x7f1a00", "0xffffff", "true", types[GEANY_FILETYPES_SQL]->styling[5]);
	styleset_get_hex(config, "styling", "word2", "0x00007f", "0xffffff", "true", types[GEANY_FILETYPES_SQL]->styling[6]);
	styleset_get_hex(config, "styling", "string", "0x7f007f", "0xffffff", "false", types[GEANY_FILETYPES_SQL]->styling[7]);
	styleset_get_hex(config, "styling", "character", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_SQL]->styling[8]);
	styleset_get_hex(config, "styling", "operator", "0x000000", "0xffffff", "true", types[GEANY_FILETYPES_SQL]->styling[9]);
	styleset_get_hex(config, "styling", "identifier", "0x991111", "0xffffff", "false", types[GEANY_FILETYPES_SQL]->styling[10]);
	styleset_get_hex(config, "styling", "sqlplus", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_SQL]->styling[11]);
	styleset_get_hex(config, "styling", "sqlplus_prompt", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_SQL]->styling[12]);
	styleset_get_hex(config, "styling", "sqlplus_comment", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_SQL]->styling[13]);
	styleset_get_hex(config, "styling", "quotedidentifier", "0x991111", "0xffffff", "false", types[GEANY_FILETYPES_SQL]->styling[14]);

	types[GEANY_FILETYPES_SQL]->keywords = g_new(gchar*, 2);
	types[GEANY_FILETYPES_SQL]->keywords[0] = styleset_get_string(config, "keywords", "keywords");
	if (types[GEANY_FILETYPES_SQL]->keywords[0] == NULL)
			types[GEANY_FILETYPES_SQL]->keywords[0] = g_strdup("absolute action add admin after aggregate \
						alias all allocate alter and any are array as asc \
						assertion at authorization before begin binary bit blob boolean both breadth by \
						call cascade cascaded case cast catalog char character check class clob close collate \
						collation column commit completion connect connection constraint constraints \
						constructor continue corresponding create cross cube current \
						current_date current_path current_role current_time current_timestamp \
						current_user cursor cycle data date day deallocate dec decimal declare default \
						deferrable deferred delete depth deref desc describe descriptor destroy destructor \
						deterministic dictionary diagnostics disconnect distinct domain double drop dynamic \
						each else end end-exec equals escape every except exception exec execute external \
						false fetch first float for foreign found from free full function general get global \
						go goto grant group grouping having host hour identity if ignore immediate in indicator \
						initialize initially inner inout input insert int integer intersect interval \
						into is isolation iterate join key language large last lateral leading left less level like \
						limit local localtime localtimestamp locator map match minute modifies modify module month \
						names national natural nchar nclob new next no none not null numeric object of off old on only \
						open operation option or order ordinality out outer output pad parameter parameters partial path \
						postfix precision prefix preorder prepare preserve primary prior privileges procedure public \
						read reads real recursive ref references referencing relative restrict result return returns \
						revoke right role rollback rollup routine row rows savepoint schema scroll scope search \
						second section select sequence session session_user set sets size smallint some space \
						specific specifictype sql sqlexception sqlstate sqlwarning start state statement static \
						structure system_user table temporary terminate than then time timestamp \
						timezone_hour timezone_minute to trailing transaction translation year zone\
						treat trigger true under union unique unknown unnest update usage user using \
						value values varchar variable varying view when whenever where with without work write");
	types[GEANY_FILETYPES_SQL]->keywords[1] = NULL;

	g_key_file_free(config);
}


void styleset_sql(ScintillaObject *sci)
{
	if (types[GEANY_FILETYPES_SQL] == NULL) styleset_sql_init();

	styleset_common(sci, 5);

	SSM(sci, SCI_SETWORDCHARS, 0, (sptr_t) GEANY_WORDCHARS);
	SSM(sci, SCI_AUTOCSETMAXHEIGHT, 8, 0);

	SSM(sci, SCI_SETLEXER, SCLEX_SQL, 0);

	SSM(sci, SCI_SETKEYWORDS, 0, (sptr_t) types[GEANY_FILETYPES_SQL]->keywords[0]);

	styleset_set_style(sci, SCE_SQL_DEFAULT, GEANY_FILETYPES_SQL, 0);
	styleset_set_style(sci, SCE_SQL_COMMENT, GEANY_FILETYPES_SQL, 1);
	styleset_set_style(sci, SCE_SQL_COMMENTLINE, GEANY_FILETYPES_SQL, 2);
	styleset_set_style(sci, SCE_SQL_COMMENTDOC, GEANY_FILETYPES_SQL, 3);
	styleset_set_style(sci, SCE_SQL_NUMBER, GEANY_FILETYPES_SQL, 4);
	styleset_set_style(sci, SCE_SQL_WORD, GEANY_FILETYPES_SQL, 5);
	styleset_set_style(sci, SCE_SQL_WORD2, GEANY_FILETYPES_SQL, 6);
	styleset_set_style(sci, SCE_SQL_STRING, GEANY_FILETYPES_SQL, 7);
	styleset_set_style(sci, SCE_SQL_CHARACTER, GEANY_FILETYPES_SQL, 8);
	styleset_set_style(sci, SCE_SQL_OPERATOR, GEANY_FILETYPES_SQL, 9);
	styleset_set_style(sci, SCE_SQL_IDENTIFIER, GEANY_FILETYPES_SQL, 10);
	styleset_set_style(sci, SCE_SQL_SQLPLUS, GEANY_FILETYPES_SQL, 11);
	styleset_set_style(sci, SCE_SQL_SQLPLUS_PROMPT, GEANY_FILETYPES_SQL, 12);
	styleset_set_style(sci, SCE_SQL_SQLPLUS_COMMENT, GEANY_FILETYPES_SQL, 13);
	styleset_set_style(sci, SCE_SQL_QUOTEDIDENTIFIER, GEANY_FILETYPES_SQL, 14);
}


static void styleset_caml_init(void)
{
	GKeyFile *config = g_key_file_new();

	style_set_load_file(config, GEANY_DATA_DIR "/filetypes.caml", G_KEY_FILE_KEEP_COMMENTS, NULL);

	types[GEANY_FILETYPES_CAML] = g_new(style_set, 1);

	styleset_get_hex(config, "styling", "default", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_CAML]->styling[0]);
	styleset_get_hex(config, "styling", "comment", "0x808080", "0xffffff", "false", types[GEANY_FILETYPES_CAML]->styling[1]);
	styleset_get_hex(config, "styling", "comment1", "0x808080", "0xffffff", "false", types[GEANY_FILETYPES_CAML]->styling[2]);
	styleset_get_hex(config, "styling", "comment2", "0x808080", "0xffffff", "false", types[GEANY_FILETYPES_CAML]->styling[3]);
	styleset_get_hex(config, "styling", "comment3", "0x808080", "0xffffff", "false", types[GEANY_FILETYPES_CAML]->styling[4]);
	styleset_get_hex(config, "styling", "number", "0x007f7f", "0xffffff", "false", types[GEANY_FILETYPES_CAML]->styling[5]);
	styleset_get_hex(config, "styling", "keyword", "0x7f1a00", "0xffffff", "true", types[GEANY_FILETYPES_CAML]->styling[6]);
	styleset_get_hex(config, "styling", "keyword2", "0x00007f", "0xffffff", "true", types[GEANY_FILETYPES_CAML]->styling[7]);
	styleset_get_hex(config, "styling", "string", "0x7f007f", "0xffffff", "false", types[GEANY_FILETYPES_CAML]->styling[8]);
	styleset_get_hex(config, "styling", "char", "0x7f007f", "0xffffff", "false", types[GEANY_FILETYPES_CAML]->styling[9]);
	styleset_get_hex(config, "styling", "operator", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_CAML]->styling[10]);
	styleset_get_hex(config, "styling", "identifier", "0x991111", "0xffffff", "false", types[GEANY_FILETYPES_CAML]->styling[11]);
	styleset_get_hex(config, "styling", "tagname", "0x000000", "0xffe0ff", "true", types[GEANY_FILETYPES_CAML]->styling[12]);
	styleset_get_hex(config, "styling", "linenum", "0x000000", "0xC0C0C0", "false", types[GEANY_FILETYPES_CAML]->styling[13]);

	types[GEANY_FILETYPES_CAML]->keywords = g_new(gchar*, 3);
	types[GEANY_FILETYPES_CAML]->keywords[0] = styleset_get_string(config, "keywords", "keywords");
	if (types[GEANY_FILETYPES_CAML]->keywords[0] == NULL)
			types[GEANY_FILETYPES_CAML]->keywords[0] = g_strdup("and as assert asr begin class constraint do \
			done downto else end exception external false for fun function functor if in include inherit \
			initializer land lazy let lor lsl lsr lxor match method mod module mutable new object of open \
			or private rec sig struct then to true try type val virtual when while with");
	types[GEANY_FILETYPES_CAML]->keywords[1] = styleset_get_string(config, "keywords", "keywords_optional");
	if (types[GEANY_FILETYPES_CAML]->keywords[1] == NULL)
			types[GEANY_FILETYPES_CAML]->keywords[1] = g_strdup("option Some None ignore ref");
	types[GEANY_FILETYPES_CAML]->keywords[2] = NULL;

	g_key_file_free(config);
}


void styleset_caml(ScintillaObject *sci)
{
	if (types[GEANY_FILETYPES_CAML] == NULL) styleset_caml_init();

	styleset_common(sci, 5);

	SSM(sci, SCI_SETWORDCHARS, 0, (sptr_t) GEANY_WORDCHARS);
	SSM(sci, SCI_AUTOCSETMAXHEIGHT, 8, 0);

	SSM(sci, SCI_SETLEXER, SCLEX_CAML, 0);

	SSM(sci, SCI_SETKEYWORDS, 0, (sptr_t) types[GEANY_FILETYPES_CAML]->keywords[0]);
	SSM(sci, SCI_SETKEYWORDS, 1, (sptr_t) types[GEANY_FILETYPES_CAML]->keywords[1]);

	styleset_set_style(sci, SCE_CAML_DEFAULT, GEANY_FILETYPES_CAML, 0);
	styleset_set_style(sci, SCE_CAML_COMMENT, GEANY_FILETYPES_CAML, 1);
	styleset_set_style(sci, SCE_CAML_COMMENT1, GEANY_FILETYPES_CAML, 2);
	styleset_set_style(sci, SCE_CAML_COMMENT2, GEANY_FILETYPES_CAML, 3);
	styleset_set_style(sci, SCE_CAML_COMMENT3, GEANY_FILETYPES_CAML, 4);
	styleset_set_style(sci, SCE_CAML_NUMBER, GEANY_FILETYPES_CAML, 5);
	styleset_set_style(sci, SCE_CAML_KEYWORD, GEANY_FILETYPES_CAML, 6);
	styleset_set_style(sci, SCE_CAML_KEYWORD2, GEANY_FILETYPES_CAML, 7);
	styleset_set_style(sci, SCE_CAML_STRING, GEANY_FILETYPES_CAML, 8);
	styleset_set_style(sci, SCE_CAML_CHAR, GEANY_FILETYPES_CAML, 9);
	styleset_set_style(sci, SCE_CAML_OPERATOR, GEANY_FILETYPES_CAML, 10);
	styleset_set_style(sci, SCE_CAML_IDENTIFIER, GEANY_FILETYPES_CAML, 11);
	styleset_set_style(sci, SCE_CAML_TAGNAME, GEANY_FILETYPES_CAML, 12);
	styleset_set_style(sci, SCE_CAML_LINENUM, GEANY_FILETYPES_CAML, 13);
}


static void styleset_oms_init(void)
{
	GKeyFile *config = g_key_file_new();

	style_set_load_file(config, GEANY_DATA_DIR "/filetypes.oms", G_KEY_FILE_KEEP_COMMENTS, NULL);

	types[GEANY_FILETYPES_OMS] = g_new(style_set, 1);
	styleset_get_hex(config, "styling", "default", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_OMS]->styling[0]);
	styleset_get_hex(config, "styling", "commentline", "0x909090", "0xffffff", "false", types[GEANY_FILETYPES_OMS]->styling[1]);
	styleset_get_hex(config, "styling", "number", "0x007f00", "0xffffff", "false", types[GEANY_FILETYPES_OMS]->styling[2]);
	styleset_get_hex(config, "styling", "word", "0x111199", "0xffffff", "false", types[GEANY_FILETYPES_OMS]->styling[3]);
	styleset_get_hex(config, "styling", "string", "0x1e90ff", "0xffffff", "false", types[GEANY_FILETYPES_OMS]->styling[4]);
	styleset_get_hex(config, "styling", "character", "0x004040", "0xffffff", "false", types[GEANY_FILETYPES_OMS]->styling[5]);
	styleset_get_hex(config, "styling", "operator", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_OMS]->styling[6]);
	styleset_get_hex(config, "styling", "identifier", "0x000000", "0xffffff", "false", types[GEANY_FILETYPES_OMS]->styling[7]);
	styleset_get_hex(config, "styling", "backticks", "0x000000", "0xe0c0e0", "false", types[GEANY_FILETYPES_OMS]->styling[8]);
	styleset_get_hex(config, "styling", "param", "0x111199", "0x0000ff", "false", types[GEANY_FILETYPES_OMS]->styling[9]);
	styleset_get_hex(config, "styling", "scalar", "0xff0000", "0xffffff", "false", types[GEANY_FILETYPES_OMS]->styling[10]);

	types[GEANY_FILETYPES_OMS]->keywords = g_new(gchar*, 2);
	types[GEANY_FILETYPES_OMS]->keywords[0] = styleset_get_string(config, "keywords", "primary");
	if (types[GEANY_FILETYPES_OMS]->keywords[0] == NULL)
			types[GEANY_FILETYPES_OMS]->keywords[0] = g_strdup("clear seq fillcols fillrowsgaspect gaddview \
			gtitle gxaxis gyaxis max contour gcolor gplot gaddview gxaxis gyaxis gcolor fill coldim gplot \
			gtitle clear arcov dpss fspec cos gxaxis gyaxis gtitle gplot gupdate rowdim fill print for to begin \
			end write cocreate coinvoke codispsave cocreate codispset copropput colsum sqrt adddialog \
			addcontrol addcontrol delwin fillrows function gaspect conjdir");
	types[GEANY_FILETYPES_OMS]->keywords[1] = NULL;

	g_key_file_free(config);
}


void styleset_oms(ScintillaObject *sci)
{
	if (types[GEANY_FILETYPES_OMS] == NULL) styleset_oms_init();

	styleset_common(sci, 5);
	SSM (sci, SCI_SETLEXER, SCLEX_OMS, 0);

	SSM (sci, SCI_SETWORDCHARS, 0, (sptr_t) GEANY_WORDCHARS);
	SSM (sci, SCI_AUTOCSETMAXHEIGHT, 8, 0);

	SSM (sci, SCI_SETCONTROLCHARSYMBOL, 32, 0);

	SSM(sci, SCI_SETKEYWORDS, 0, (sptr_t) types[GEANY_FILETYPES_OMS]->keywords[0]);

	styleset_set_style(sci, SCE_SH_DEFAULT, GEANY_FILETYPES_OMS, 0);
	styleset_set_style(sci, SCE_SH_COMMENTLINE, GEANY_FILETYPES_OMS, 1);
	styleset_set_style(sci, SCE_SH_NUMBER, GEANY_FILETYPES_OMS, 2);
	styleset_set_style(sci, SCE_SH_WORD, GEANY_FILETYPES_OMS, 3);
	styleset_set_style(sci, SCE_SH_STRING, GEANY_FILETYPES_OMS, 4);
	styleset_set_style(sci, SCE_SH_CHARACTER, GEANY_FILETYPES_OMS, 5);
	styleset_set_style(sci, SCE_SH_OPERATOR, GEANY_FILETYPES_OMS, 6);
	styleset_set_style(sci, SCE_SH_IDENTIFIER, GEANY_FILETYPES_OMS, 7);
	styleset_set_style(sci, SCE_SH_BACKTICKS, GEANY_FILETYPES_OMS, 8);
	styleset_set_style(sci, SCE_SH_PARAM, GEANY_FILETYPES_OMS, 9);
	styleset_set_style(sci, SCE_SH_SCALAR, GEANY_FILETYPES_OMS, 10);

	SSM(sci, SCI_SETWHITESPACEFORE, 1, 0xc0c0c0);
}




