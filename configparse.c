#include "common.h"
#include <gst/gst.h>
#include <glib.h> 
#include <string.h>

static gboolean config_decode
(GKeyFile *file, struct config_head *config);

static gboolean config_set_default(struct config_head *config);

struct config_head *config_init(const gchar *path)
{
	GKeyFile *file = NULL;
	file = g_key_file_new();

	struct config_head *head = NULL;
	head = g_malloc(sizeof(struct config_head));
	head->general = g_malloc(sizeof(struct stream_config));
	head->stream_list = g_sequence_new(NULL);

	if (NULL == path) {
		if (g_key_file_load_from_file
		    (file, "/etc/mpe4r.conf", G_KEY_FILE_NONE,
		     NULL))
			config_decode(file, head);

		if (g_key_file_load_from_file
		     (file, "~/.mpe4r.conf", G_KEY_FILE_NONE,
		     NULL))
			config_decode(file, head);

		if (g_key_file_load_from_file
		    (file, "mpe4r.conf", G_KEY_FILE_NONE,
		     NULL))
			config_decode(file, head);

		return head;
	} else {
		g_key_file_load_from_file
			(file, path, G_KEY_FILE_NONE, NULL);
		config_decode(file, head);
		return head;
	}

	return NULL;
}

static gboolean config_decode(GKeyFile *file, struct config_head *config){
	if (NULL == file || NULL == config)
		return FALSE;
	gchar **groups = NULL;
	gsize length;

	GRegex *regex;

	groups = g_key_file_get_groups(file, &length);
	if (0 == length)
		return FALSE;

	if (g_key_file_has_group(file, "general")){
		memset(config->general, 0, sizeof(struct stream_config));
		config->general->address =
			g_key_file_get_value
			(file, "general", "address", NULL);
		config->general->service =
			g_key_file_get_value
			(file, "general", "service", NULL);
		config->general->video_encoder =
			g_key_file_get_value
			(file, "general", "video_encoder", NULL);
		config->general->sound_encoder =
			g_key_file_get_value
			(file, "general", "sound_encoder", NULL);
		config->general->width =
			g_key_file_get_integer
			(file, "general", "width", NULL);
		config->general->height =
			g_key_file_get_integer
			(file, "general", "height", NULL);
		config->general->enable = TRUE;

		config->general->name = "general";
	}

	regex = g_regex_new("camera[0-9]{1,}", 0, 0, NULL);
	do {
		struct stream_config *data =
		g_malloc0(sizeof(struct stream_config));

		if (g_regex_match(regex, *groups, 0, 0)){
			data->name = g_strdup(*groups);
			data->dev_path =
				g_key_file_get_value
				(file, *groups, "camera_path", NULL);
			if (NULL == data->dev_path) {
				g_free(data);
				groups++;
				continue;
			}
			data->mount =
				g_key_file_get_value
				(file, *groups, "mount", NULL);
			if (NULL == data->mount) {
				g_free(data);
				groups++;
				continue;
			}

			data->enable = 
				g_key_file_get_integer
				(file, *groups, "enable", NULL);
			data->address =
				g_key_file_get_value
				(file, *groups, "address",
				 NULL);
			data->video_encoder =
				g_key_file_get_value
				(file, *groups, "video_encoder", 
				 NULL);
			data->sound_encoder =
				g_key_file_get_value
				(file, *groups, "sound_encoder", 
				 NULL);
			data->width =
				g_key_file_get_integer
				(file, *groups, "width", 
				 NULL);
			data->height =
				g_key_file_get_integer
				(file, *groups, "height", 
				 NULL);
			data->sound_device =
				g_key_file_get_value
				(file, *groups, "sound_device", NULL);

			data->r.pipeline = NULL;

			g_sequence_append(config->stream_list, (gpointer)data);
		}
		else {
			g_free(data);
		}

		groups++;
	} while(NULL != *groups);

	config_set_default(config);

	g_regex_unref(regex);

	return TRUE;
}

static gboolean config_set_default(struct config_head *config){
        GSequenceIter *begin, *current, *next;

        begin = g_sequence_get_begin_iter(config->stream_list);
        current = begin;
        next = begin;
	struct stream_config *data = NULL;

        while(!g_sequence_iter_is_end(next)){
        	current = next;
                data = g_sequence_get(current);

		if (NULL == data->video_encoder)
			data->video_encoder = config->general->video_encoder;
		if (NULL == data->sound_encoder)
			data->sound_encoder = config->general->sound_encoder;
		if (NULL == data->address)
			data->address = config->general->address;
		if (NULL == data->service)
			data->service = config->general->service;
		if (0 == data->width)
			data->width = config->general->width;
		if (0 == data->height)
			data->height = config->general->height;

                next = g_sequence_iter_next(current);
	}
	return TRUE;
}
