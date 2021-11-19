#include <session-core.h>
#include <session-core-type.h>

#include <gst/gst.h>
#include <glib-2.0/glib.h>

#define GST_USE_UNSTABLE_API

#define GST_DEBUG       4


static gint agent_port = 2520;
static gint session_core_port = 2250;
static gint worker_manager_port = 5220;
static gchar worker_ip[20] = "192.168.1.12";

static gchar signalling_url[50] = "wss://host.thinkmay.net/Handshake";
static gchar turn[100] = "turn://thinkmaycoturn:thinkmaycoturn_password@turn:stun.thinkmay.net:3478";

static gint  session_id = 0;

static gchar video_codec[50] = {0};
static gchar audio_codec[50] = {0}; 

static gchar connection_string[200] = {0};

static gint  screen_width = 1920;
static gint  screen_height = 1080;


static GOptionEntry entries[] = {
  {"agentport", 0, 0, G_OPTION_ARG_INT, &agent_port,
      "String ID of the peer to connect to", "ID"},
  {"sessioncoreport", 0, 0, G_OPTION_ARG_INT, &session_core_port,
      "Signalling server to connect to", "URL"},
  {"managerport", 0, 0, G_OPTION_ARG_INT, &worker_manager_port,
      "Signalling server to connect to", "URL"},
  {"workerip", 0, 0, G_OPTION_ARG_STRING, &worker_ip,
      "Request that the peer generate the offer and we'll answer", "URL"},  
  {"sessionid", 0, 0, G_OPTION_ARG_INT, &session_id,
      "String ID of the peer to connect to", "ID"},
  {"signalling", 0, 0, G_OPTION_ARG_STRING, &signalling_url,
      "Signalling server to connect to", "URL"},
  {"turn", 0, 0, G_OPTION_ARG_STRING, &turn,
      "Request that the peer generate the offer and we'll answer", "URL"},
  {"audiocodec", 0, 0, G_OPTION_ARG_STRING, &audio_codec,
      "audio codec use for decode bin", "codec"},
  {"videocodec", 0, 0, G_OPTION_ARG_STRING, &video_codec,
      "video codec use for decode bin", "codec"},
  {"connection", 0, 0, G_OPTION_ARG_STRING, &connection_string,
      "connection ", "codec"},
  {NULL},
};

int
main(int argc, char* argv[])
{
    GOptionContext *context;
    GError *error = NULL;

    context = g_option_context_new ("- thinkmay gstreamer client");
    g_option_context_add_main_entries (context, entries, NULL);
    g_option_context_add_group (context, gst_init_get_option_group ());
    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_printerr ("Error initializing: %s\n", error->message);
        return -1;
    }

    session_core_initialize();

    return;
}
