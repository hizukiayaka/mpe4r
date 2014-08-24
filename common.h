#ifndef _COMMON_H_
#define _COMMON_H_
#include <glib.h>

struct config_head;
struct stream_config;

struct config_head{
	struct stream_config *general;
	GSequence *stream_list;
};

struct stream_config{
	/* get from config file*/
	gboolean enable;
	gchar *name;
	gchar *dev_path;
	gchar *encoder;
	gchar *address;
	gchar *service;
	gchar *mount;
	gint  width;
	gint  height;
	/* some runtime data */
	struct {
		void *pipeline;
	}r;
};
#endif
