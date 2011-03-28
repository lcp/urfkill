/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2011 Gary Ching-Pang Lin <glin@novell.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <string.h>
#include <expat.h>
#include <sys/stat.h>
#include "urf-utils.h"
#include "urf-config.h"

#define URFKILL_PROFILE_DIR URFKILL_CONFIG_DIR"profile/"
#define URFKILL_CONFIGURED_PROFILE URFKILL_CONFIG_DIR"hardware.conf"

enum
{
	OPT_NONE,
	OPT_KEY_CONTROL,
	OPT_MASTER_KEY,
	OPT_FORCE_SYNC,
	OPT_UNKNOWN,
};

enum
{
	OPT_TYPE_NONE,
	OPT_TYPE_BOOLEAN,
	OPT_TYPE_UNKNOWN,
};

enum
{
	OPER_STRING,
	OPER_STRING_OUTOF,
	OPER_CONTAINS,
	OPER_CONTAINS_NCASE,
	OPER_CONTAINS_NOT,
	OPER_CONTAINS_OUTOF,
	OPER_PREFIX,
	OPER_PREFIX_NCASE,
	OPER_PREFIX_OUTOF,
	OPER_SUFFIX,
	OPER_SUFFIX_NCASE,
	OPER_SUFFIX_OUTOF,
	OPER_UNKNOWN,
};

typedef struct {
	gboolean key_control;
	gboolean master_key;
	gboolean force_sync;
} Options;

typedef struct {
	int xml_depth;
	int xml_bound;
	int opt;
	int opt_type;
	Options options;
	DmiInfo *hardware_info;
} ParseInfo;

#define URF_CONFIG_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
                                     URF_TYPE_CONFIG, UrfConfigPrivate))
struct UrfConfigPrivate {
	char *user;
	Options options;
};

G_DEFINE_TYPE(UrfConfig, urf_config, G_TYPE_OBJECT)

static int
get_option (const char *option)
{
	if (g_strcmp0 (option, "key_control") == 0)
		return OPT_KEY_CONTROL;
	else if (g_strcmp0 (option, "master_key") == 0)
		return OPT_MASTER_KEY;
	else if (g_strcmp0 (option, "force_sync") == 0)
		return OPT_FORCE_SYNC;
	return OPT_UNKNOWN;
}

static int
get_option_type (const char *type)
{
	if (g_strcmp0 (type, "bool") == 0)
		return OPT_TYPE_BOOLEAN;
	return OPT_TYPE_UNKNOWN;
}

static char *
get_match_key (DmiInfo    *hardware_info,
	       const char *key)
{
	if (hardware_info == NULL)
		return NULL;

	if (g_strcmp0 (key, "sys_vendor") == 0)
		return hardware_info->sys_vendor;
	else if (g_strcmp0 (key, "bios_date") == 0)
		return hardware_info->bios_date;
	else if (g_strcmp0 (key, "bios_vendor") == 0)
		return hardware_info->bios_vendor;
	else if (g_strcmp0 (key, "bios_version") == 0)
		return hardware_info->bios_version;
	else if (g_strcmp0 (key, "product_name") == 0)
		return hardware_info->product_name;
	else if (g_strcmp0 (key, "product_version") == 0)
		return hardware_info->product_version;
	return NULL;
}

static int
get_operator (const char *operator)
{
	if (g_strcmp0 (operator, "string") == 0)
		return OPER_STRING;
	else if (g_strcmp0 (operator, "string_outof") == 0)
		return OPER_STRING_OUTOF;
	else if (g_strcmp0 (operator, "contains") == 0)
		return OPER_CONTAINS;
	else if (g_strcmp0 (operator, "contains_ncase") == 0)
		return OPER_CONTAINS_NCASE;
	else if (g_strcmp0 (operator, "contains_not") == 0)
		return OPER_CONTAINS_NOT;
	else if (g_strcmp0 (operator, "contains_outof") == 0)
		return OPER_CONTAINS_OUTOF;
	else if (g_strcmp0 (operator, "prefix") == 0)
		return OPER_PREFIX;
	else if (g_strcmp0 (operator, "prefix_ncase") == 0)
		return OPER_PREFIX_NCASE;
	else if (g_strcmp0 (operator, "prefix_outof") == 0)
		return OPER_PREFIX_OUTOF;
	else if (g_strcmp0 (operator, "suffix") == 0)
		return OPER_SUFFIX;
	else if (g_strcmp0 (operator, "suffix_ncase") == 0)
		return OPER_SUFFIX_NCASE;
	else if (g_strcmp0 (operator, "suffix_outof") == 0)
		return OPER_SUFFIX_OUTOF;
	return OPER_UNKNOWN;
}

static gboolean
match_rule (const char *str1,
	    const int   operator,
	    const char *str2)
{
	gboolean match = FALSE;
	char **token;
	char *str1_lower;
	char *str2_lower;
	int i;

	if (strlen (str1) < 1 || strlen (str2) < 1)
		return FALSE;

	switch (operator) {
	case OPER_STRING:
		if (g_strcmp0 (str1, str2) == 0)
			match = TRUE;
		break;
	case OPER_STRING_OUTOF:
		token = g_strsplit (str2, ";", 0);
		for (i = 0; token[i]; i++) {
			if (g_strcmp0 (str1, token[i]) == 0) {
				match = TRUE;
				break;
			}
		}
		g_strfreev (token);
		break;
	case OPER_CONTAINS:
		if (g_strrstr (str1, str2))
			match = TRUE;
		break;
	case OPER_CONTAINS_NCASE:
		str1_lower = g_ascii_strdown (str1, -1);
		str2_lower = g_ascii_strdown (str2, -1);
		if (g_strrstr (str1_lower, str2_lower))
			match = TRUE;
		g_free (str1_lower);
		g_free (str2_lower);
		break;
	case OPER_CONTAINS_NOT:
		if (g_strrstr (str1, str2) == NULL)
			match = TRUE;
		break;
	case OPER_CONTAINS_OUTOF:
		token = g_strsplit (str2, ";", 0);
		for (i = 0; token[i]; i++) {
			if (strlen (token[i]) < 1) {
				continue;
			} else if (g_strrstr (str1, token[i])) {
				match = TRUE;
				break;
			}
		}
		g_strfreev (token);
		break;
	case OPER_PREFIX:
		if (g_str_has_prefix (str1, str2))
			match = TRUE;
		break;
	case OPER_PREFIX_NCASE:
		str1_lower = g_ascii_strdown (str1, -1);
		str2_lower = g_ascii_strdown (str2, -1);
		if (g_str_has_prefix (str1_lower, str2_lower))
			match = TRUE;
		g_free (str1_lower);
		g_free (str2_lower);
		break;
	case OPER_PREFIX_OUTOF:
		token = g_strsplit (str2, ";", 0);
		for (i = 0; token[i]; i++) {
			if (strlen (token[i]) < 1) {
				continue;
			} else if (g_str_has_prefix (str1, token[i])) {
				match = TRUE;
				break;
			}
		}
		g_strfreev (token);
		break;
	case OPER_SUFFIX:
		if (g_str_has_suffix (str1, str2))
			match = TRUE;
		break;
	case OPER_SUFFIX_NCASE:
		str1_lower = g_ascii_strdown (str1, -1);
		str2_lower = g_ascii_strdown (str2, -1);
		if (g_str_has_suffix (str1_lower, str2_lower))
			match = TRUE;
		g_free (str1_lower);
		g_free (str2_lower);
		break;
	case OPER_SUFFIX_OUTOF:
		token = g_strsplit (str2, ";", 0);
		for (i = 0; token[i]; i++) {
			if (strlen (token[i]) < 1) {
				continue;
			} else if (g_str_has_suffix (str1, token[i])) {
				match = TRUE;
				break;
			}
		}
		g_strfreev (token);
		break;
	default:
		match = FALSE;
		break;
	}

	return match;
}

static void
parse_xml_cdata_handler (void       *data,
			 const char *cdata,
			 int         len)
{
	ParseInfo *info = (ParseInfo *)data;
	char *str = g_strndup (cdata, len);

	if (info->opt == OPT_NONE ||
	    info->opt == OPT_UNKNOWN) {
		return;
	}

	switch (info->opt) {
	case OPT_KEY_CONTROL:
		if (g_ascii_strcasecmp (str, "TRUE") == 0)
			info->options.key_control = TRUE;
		else if (g_ascii_strcasecmp (str, "FALSE") == 0)
			info->options.key_control = FALSE;
		break;
	case OPT_MASTER_KEY:
		if (g_ascii_strcasecmp (str, "TRUE") == 0)
			info->options.master_key = TRUE;
		else if (g_ascii_strcasecmp (str, "FALSE") == 0)
			info->options.master_key = FALSE;
		break;
	case OPT_FORCE_SYNC:
		if (g_ascii_strcasecmp (str, "TRUE") == 0)
			info->options.force_sync = TRUE;
		else if (g_ascii_strcasecmp (str, "FALSE") == 0)
			info->options.force_sync = FALSE;
		break;
	default:
		break;
	}
}

static void
parse_xml_start_element (void       *data,
			 const char *name,
			 const char **atts)
{
	ParseInfo *info = (ParseInfo *)data;
	const char *key = NULL;
	const char *type = NULL;
	const char *match_key = NULL;
	const char *match_body = NULL;
	int operator;
	int i;

	info->xml_depth++;

	if (info->xml_depth > info->xml_bound)
		return;
	else if (info->xml_depth < info->xml_bound)
		info->xml_bound = info->xml_depth + 1;

	info->opt = OPT_NONE;
	info->opt_type = OPT_TYPE_NONE;

	if (g_strcmp0 (name, "match") == 0) {
		for (i = 0; atts[i]; i++) {
			if (g_strcmp0 (atts[i], "key") == 0) {
				if (!atts[i+1])
					continue;
				key = atts[i+1];
				i++;
			} else if ((operator = get_operator (atts[i])) != OPER_UNKNOWN) {
				if (!atts[i+1])
					continue;
				match_body = atts[i+1];
				i++;
			}
		}

		match_key = get_match_key (info->hardware_info, key);
		if (!match_rule (match_key, operator, match_body))
			return;
	} else if (g_strcmp0 (name, "option") == 0) {
		for (i = 0; atts[i]; i++) {
			if (g_strcmp0 (atts[i], "key") == 0) {
				if (!atts[i+1])
					continue;
				key = atts[i+1];
				i++;
			} else if (g_strcmp0 (atts[i], "type") == 0) {
				if (!atts[i+1])
					continue;
				type = atts[i+1];
				i++;
			}
		}

		info->opt = get_option (key);
		info->opt_type = get_option_type (type);
	}

	info->xml_bound++;
}

static void
parse_xml_end_element (void       *data,
		       const char *name)
{
	ParseInfo *info = (ParseInfo *)data;

	if (info->xml_bound > info->xml_depth)
		info->xml_bound = info->xml_depth;

	info->opt = OPT_NONE;
	info->opt_type = OPT_TYPE_NONE;

	info->xml_depth--;
}

static gboolean
profile_xml_parse (DmiInfo    *hardware_info,
		   Options    *options,
		   const char *filename)
{
	ParseInfo *info;
	XML_Parser parser;
	char *content;
	gsize length;
	int len;

	if (!g_file_get_contents (filename, &content, &length, NULL)) {
		g_debug ("Failed to read profile: %s", filename);
		return FALSE;
	}

	info = g_new0 (ParseInfo, 1);
	info->hardware_info = hardware_info;
	info->xml_depth = 0;
	info->xml_bound = 1;
	info->opt = OPT_NONE;
	info->opt_type = OPT_TYPE_NONE;
	info->options.key_control = options->key_control;
	info->options.master_key = options->master_key;
	info->options.force_sync = options->force_sync;

	parser = XML_ParserCreate (NULL);
	XML_SetUserData (parser, (void *)info);
	XML_SetElementHandler (parser,
			       parse_xml_start_element,
			       parse_xml_end_element);
	XML_SetCharacterDataHandler (parser,
				     parse_xml_cdata_handler);
	len = strlen (content);

	if (XML_Parse (parser, content, len, 1) == XML_STATUS_ERROR) {
		g_warning ("Profile Parse error: %s", filename);
		XML_ParserFree (parser);
		g_free (info);
		return FALSE;
	}

	XML_ParserFree (parser);

	options->key_control = info->options.key_control;
	options->master_key = info->options.master_key;
	options->force_sync = info->options.force_sync;

	g_free (info);
	return TRUE;
}

static gboolean
load_configured_settings (UrfConfig *config)
{
	UrfConfigPrivate *priv = config->priv;
	GKeyFile *profile = g_key_file_new ();
	gboolean ret = FALSE;
	GError *error;

	ret = g_key_file_load_from_file (profile,
					 URFKILL_CONFIGURED_PROFILE,
					 G_KEY_FILE_NONE,
					 NULL);
	if (!ret) {
		g_debug ("No configured profile found");
		g_key_file_free (profile);
		return FALSE;
	}

	if (!g_key_file_has_group (profile, "Profile")) {
		g_debug ("No valid group in the configured profile");
		return FALSE;
	}

	error = NULL;
	ret = g_key_file_get_boolean (profile, "Profile", "key_control", &error);
	if (!error)
		priv->options.key_control = ret;

	error = NULL;
	ret = g_key_file_get_boolean (profile, "Profile", "master_key", &error);
	if (!error)
		priv->options.master_key = ret;

	error = NULL;
	ret = g_key_file_get_boolean (profile, "Profile", "force_sync", &error);
	if (!error)
		priv->options.force_sync = ret;

	g_key_file_free (profile);

	return TRUE;
}

static void
save_configured_profile (UrfConfig *config)
{
	UrfConfigPrivate *priv = config->priv;
	GKeyFile *profile;
	gboolean ret, value;
	const char *header = "# DO NOT EDIT! This file is created by urfkilld automatically.\n";
	char *content = NULL;

	/* Remove the existed profile */
	if (g_file_test (URFKILL_CONFIGURED_PROFILE, G_FILE_TEST_IS_REGULAR))
		g_unlink (URFKILL_CONFIGURED_PROFILE);

	profile = g_key_file_new ();
	ret = g_key_file_load_from_data (profile,
					 header,
					 strlen (header),
					 G_KEY_FILE_KEEP_COMMENTS,
					 NULL);
	if (!ret) {
		g_key_file_free (profile);
		return;
	}

	value = priv->options.key_control;
	g_key_file_set_value (profile, "Profile", "key_control",
			      value?"true":"false");

	value = priv->options.master_key;
	g_key_file_set_value (profile, "Profile", "master_key",
			      value?"true":"false");

	value = priv->options.force_sync;
	g_key_file_set_value (profile, "Profile", "force_sync",
			      value?"true":"false");

	content = g_key_file_to_data (profile, NULL, NULL);
	g_key_file_free (profile);

	/* Write back the configured profile */
	if (content) {
		ret = g_file_set_contents (URFKILL_CONFIGURED_PROFILE,
					   content, -1, NULL);
		if (!ret)
			g_debug ("Failed to save configured profile: %s",
				 URFKILL_CONFIGURED_PROFILE);
		g_free (content);
		g_chmod (URFKILL_CONFIGURED_PROFILE,
			 S_IRUSR | S_IRGRP | S_IROTH);
	}
}

static gint
string_sorter (gconstpointer str1,
	       gconstpointer str2)
{
	return g_strcmp0 ((const char*)str1, (const char*)str2);
}

/**
 * urf_config_load_profile:
 **/
void
urf_config_load_profile (UrfConfig *config)
{
	UrfConfigPrivate *priv = config->priv;
	DmiInfo *hardware_info;
	Options *options;
	GList *profile_list = NULL;
	GList *lptr;
	GDir *profile_dir = NULL;
	const char *file;
	char *profile, *full;

	if (load_configured_settings (config))
		return;

	hardware_info = get_dmi_info ();
	if (hardware_info == NULL || hardware_info->sys_vendor == NULL) {
		g_debug ("Failed to get DMI information");
		return;
	}

	options = g_new0 (Options, 1);
	options->key_control = priv->options.key_control;
	options->master_key = priv->options.master_key;
	options->force_sync = priv->options.force_sync;

	profile_dir = g_dir_open (URFKILL_PROFILE_DIR, 0, NULL);
	while (file = g_dir_read_name (profile_dir)) {
		if (file[0] == '.' || !g_str_has_suffix (file, ".xml"))
			continue;

		full = g_build_filename( URFKILL_PROFILE_DIR, file, NULL );
		if (g_file_test (full, G_FILE_TEST_IS_REGULAR))
			profile_list = g_list_append (profile_list, g_strdup (file));
		g_free (full);
	}
	g_dir_close (profile_dir);

	profile_list = g_list_sort (profile_list, string_sorter);

	for (lptr = profile_list; lptr; lptr = lptr->next) {
		profile = g_build_filename (URFKILL_PROFILE_DIR,
					    (const char*)lptr->data,
					    NULL);
		profile_xml_parse (hardware_info, options, profile);
		g_free (profile);
	}
	g_list_free_full (profile_list, g_free);

	priv->options.key_control = options->key_control;
	priv->options.master_key = options->master_key;
	priv->options.force_sync = options->force_sync;

	save_configured_profile (config);

	dmi_info_free (hardware_info);
	g_free (options);
}

/**
 * urf_config_load_from_file:
 **/
void
urf_config_load_from_file (UrfConfig  *config,
			   const char *filename)
{
	UrfConfigPrivate *priv = config->priv;
	GKeyFile *key_file = g_key_file_new ();
	gboolean ret = FALSE;
	GError *error = NULL;

	ret = g_key_file_load_from_file (key_file, filename, G_KEY_FILE_NONE, NULL);

	if (!ret) {
		g_warning ("Failed to load config file: %s", filename);
		g_key_file_free (key_file);
		return;
	}

	/* Parse the key file and store to private variables*/
	priv->user = g_key_file_get_value (key_file, "general", "user", NULL);

	error = NULL;
	ret = g_key_file_get_boolean (key_file, "general", "key_control", &error);
	if (!error)
		priv->options.key_control = ret;

	error = NULL;
	ret = g_key_file_get_boolean (key_file, "general", "master_key", &error);
	if (!error)
		priv->options.master_key = ret;

	error = NULL;
	ret = g_key_file_get_boolean (key_file, "general", "force_sync", &error);
	if (!error)
		priv->options.force_sync = ret;

	g_key_file_free (key_file);
}

/**
 * urf_config_get_user:
 **/
const char *
urf_config_get_user (UrfConfig *config)
{
	return (const char *)config->priv->user;
}

/**
 * urf_config_get_key_control:
 **/
gboolean
urf_config_get_key_control (UrfConfig *config)
{
	return config->priv->options.key_control;
}

/**
 * urf_config_get_master_key:
 **/
gboolean
urf_config_get_master_key (UrfConfig *config)
{
	return config->priv->options.master_key;
}

/**
 * urf_config_get_master_key:
 **/
gboolean
urf_config_get_force_sync (UrfConfig *config)
{
	return config->priv->options.force_sync;
}

/**
 * urf_config_init:
 **/
static void
urf_config_init (UrfConfig *config)
{
	UrfConfigPrivate *priv = URF_CONFIG_GET_PRIVATE (config);
	priv->user = NULL;
	priv->options.key_control = TRUE;
	priv->options.master_key = FALSE;
	priv->options.force_sync = FALSE;
	config->priv = priv;
}

/**
 * urf_config_finalize:
 **/
static void
urf_config_finalize (GObject *object)
{
	UrfConfigPrivate *priv = URF_CONFIG(object)->priv;

	g_free (priv->user);

	G_OBJECT_CLASS(urf_config_parent_class)->finalize(object);
}

/**
 * urf_config_class_init:
 **/
static void
urf_config_class_init(UrfConfigClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	g_type_class_add_private(klass, sizeof(UrfConfigPrivate));
	object_class->finalize = urf_config_finalize;
}

/**
 * urf_config_new:
 **/
UrfConfig *
urf_config_new (void)
{
	UrfConfig *config;
	config = URF_CONFIG(g_object_new (URF_TYPE_CONFIG, NULL));
	return config;
}
