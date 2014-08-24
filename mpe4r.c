#include <gst/gst.h>
#include <glib-unix.h>
#include <gst/rtsp-server/rtsp-server.h>
#include "common.h"
#include "configparse.h"

GST_DEBUG_CATEGORY_STATIC (mpe4r_debug);
#define GST_CAT_DEFAULT mpe4r_debug

gpointer rtsp_server_instance(gpointer data)
{
	GstRTSPServer *server;
	GstRTSPMountPoints *mounts;
	GstRTSPMediaFactory *factory;
	gchar *str;

	struct stream_config *priv = NULL;
	priv = (struct stream_config *)data;

	/* create a server instance */
	server = gst_rtsp_server_new();
	/* set bind ip */
	gst_rtsp_server_set_address(server, priv->address);
	/* set bind port */
	gst_rtsp_server_set_service(server, priv->service);

	/* get the mount points for this server, every server has a default object
	 * that be used to map uri mount points to media factories */
	mounts = gst_rtsp_server_get_mount_points(server);

	str = g_strdup_printf("( "
			      "v4l2src device=%s ! video/x-raw, width=%d, height=%d ! "
			      " videoconvert ! %s ! h264parse ! rtph264pay pt=96 name=pay0 "
			      ")", priv->dev_path, priv->width, priv->height, priv->encoder);
	/* make a media factory for a test stream. The default media factory can use
	 * gst-launch syntax to create pipelines. 
	 * any launch line works as long as it contains elements named pay%d. Each
	 * element with pay%d names will be a stream */
	factory = gst_rtsp_media_factory_new();

	GST_INFO_OBJECT(factory, "%s", str);

	gst_rtsp_media_factory_set_launch(factory, str);
	g_free(str);

	/* attach the test factory to the /test url */
	gst_rtsp_mount_points_add_factory(mounts, priv->mount, factory);

	/* don't need the ref to the mapper anymore */
	g_object_unref(mounts);

	/* attach the server to the default maincontext */
	gst_rtsp_server_attach(server, NULL);

	/* start serving */
	GST_INFO_OBJECT(factory, "stream ready at rtsp://%s:%s%s",
			priv->address, priv->service, priv->mount);
	return NULL;
}

gpointer start_rtsp_server(gpointer data)
{
	GThread *thread;
	thread = g_thread_new("stream", rtsp_server_instance, data);
	return g_thread_join(thread);
}
inline static gint traverse_seq
    (GSequence * seq, GThreadFunc func) {
	GSequenceIter *begin, *current, *next;
	gint traversed = 0;
	struct stream_config *data;

	begin = g_sequence_get_begin_iter(seq);
	current = begin;
	next = current;

	while (!g_sequence_iter_is_end(next)) {
		current = next;

		data = (struct stream_config *) g_sequence_get(current);
		func(data);
		traversed++;

		next = g_sequence_iter_next(current);
	}
	return traversed;
}

static gpointer clean_up_stream (gpointer data)
{
	struct stream_config *priv =  NULL;
	priv = (struct stream_config *)data;

	gst_element_set_state(priv->r.pipeline, GST_STATE_NULL);

	g_free(data);
	
	return NULL;
}

int main(int argc, char *argv[])
{
	GMainLoop *loop;
	struct config_head *config;

	gst_init(&argc, &argv);

	GST_DEBUG_CATEGORY_INIT(mpe4r_debug, "mpe4r", 0,
				"Multi Planes Encoder for RTSP");

	loop = g_main_loop_new(NULL, FALSE);

	config = config_init(NULL);
	g_unix_signal_add(SIGTERM, (GSourceFunc)g_main_loop_quit, loop);
	g_unix_signal_add(SIGINT, (GSourceFunc)g_main_loop_quit, loop);

	traverse_seq(config->stream_list, start_rtsp_server);

	g_main_loop_run(loop);

	traverse_seq(config->stream_list, clean_up_stream);
	g_sequence_free(config->stream_list);
	g_free(config->general);
	g_free(config);

	g_main_loop_unref(loop);

	return 0;
}
