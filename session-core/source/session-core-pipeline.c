﻿#include <session-core-pipeline.h>
#include <session-core-type.h>
#include <session-core-data-channel.h>
#include <session-core-signalling.h>
#include <session-core-remote-config.h>


#include <general-constant.h>
#include <logging.h>
#include <qoe.h>
#include <exit-code.h>

#include <gst/gst.h>
#include <glib-2.0/glib.h>
#include <gst/webrtc\webrtc.h>
#include <gst/rtp\gstrtppayloads.h>






/// <summary>
/// gstreamer video element enumaration,
/// the order of element in enum must follow the 
/// </summary>
enum
{
    /*screen capture source*/
    DIRECTX_SCREEN_CAPTURE_SOURCE,

    /*preprocess before encoding*/
    CUDA_UPLOAD,
    CUDA_CONVERT,
    VIDEO_CONVERT,
    /*video encoder*/

    NVIDIA_H264_ENCODER,
    NVIDIA_H265_ENCODER,
    H264_MEDIA_FOUNDATION,
    H265_MEDIA_FOUNDATION,

    VP9_ENCODER,
    VP8_ENCODER,


    /*payload packetize*/
    RTP_H264_PAYLOAD,
    RTP_H265_PAYLOAD,
    RTP_VP9_PAYLOAD,
    RTP_VP8_PAYLOAD,

    VIDEO_ELEMENT_LAST
};

/// <summary>
/// gstreamer audio element enumaration,
/// the order of element in enum must follow the 
/// </summary>
enum
{
    /*audio capture source*/
    PULSE_SOURCE_SOUND,
    WASAPI_SOURCE_SOUND,

    /*audio encoder*/
    OPUS_ENCODER,
    MP3_ENCODER,
    AAC_ENCODER,

    /*rtp packetize and queue*/
    RTP_OPUS_PAYLOAD,
    RTP_RTX_QUEUE,

    AUDIO_ELEMENT_LAST
};


struct _Pipeline
{
    PipelineState state;

	GstElement* pipeline;
	GstElement* webrtcbin;

    GstElement* video_element[VIDEO_ELEMENT_LAST];
    GstElement* audio_element[AUDIO_ELEMENT_LAST];

    GstCaps* video_caps[VIDEO_ELEMENT_LAST];
    GstCaps* audio_caps[AUDIO_ELEMENT_LAST];
};

Pipeline*
pipeline_initialize(SessionCore* core)
{
    SignallingHub* hub = session_core_get_signalling_hub(core);

    static Pipeline pipeline;
    memset(&pipeline,0,sizeof(pipeline));
    

    pipeline.state = PIPELINE_NOT_READY;
    return &pipeline;
}

static gboolean
start_pipeline(SessionCore* core)
{
    GstStateChangeReturn ret;
    Pipeline* pipe = session_core_get_pipeline(core);

    ret = GST_IS_ELEMENT(pipe->pipeline);    

    ret = gst_element_set_state(GST_ELEMENT(pipe->pipeline), GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        GError error;
        error.message = "Fail to start pipeline, this may due to pipeline setup failure";
        session_core_finalize(core, PIPELINE_ERROR_EXIT,&error);
    }
    write_to_log_file(SESSION_CORE_GENERAL_LOG,"Starting pipeline");
    return TRUE;
}


#define DIRECTX_PAD "video/x-raw(memory:D3D11Memory)"
#define RTP_CAPS_OPUS "application/x-rtp,media=audio,payload=96,encoding-name="
#define RTP_CAPS_VIDEO "application/x-rtp,media=video,payload=97,encoding-name="




static void
setup_element_factory(SessionCore* core,
                      Codec video, 
                      Codec audio)
{
    Pipeline* pipe = session_core_get_pipeline(core);
    GError* error = NULL;
    
    if (video == CODEC_H264)
    {
        if (audio == OPUS_ENC) 
        {
            // setup default nvenc encoder (nvidia encoder)
            pipe->pipeline =
                gst_parse_launch("webrtcbin bundle-policy=max-bundle name=sendrecv "
                    "d3d11desktopdupsrc name=screencap ! "DIRECTX_PAD",framerate=60/1 ! "
                    "queue max-size-time=0 max-size-bytes=0 max-size-buffers=3 ! "
                    "d3d11convert ! "DIRECTX_PAD",format=NV12 ! "
                    "queue max-size-time=0 max-size-bytes=0 max-size-buffers=3 ! "
                    "mfh264enc name=videoencoder ! video/x-h264,profile=high ! "
                    "queue max-size-time=0 max-size-bytes=0 max-size-buffers=3 ! "
                    "rtph264pay name=rtp ! "
                    "queue max-size-time=0 max-size-bytes=0 max-size-buffers=3 ! " 
                    RTP_CAPS_VIDEO "H264 ! sendrecv. "
                    "wasapisrc name=audiocapsrc name=audiocapsrc ! audioconvert ! audioresample ! queue ! "
                    "opusenc name=audioencoder ! rtpopuspay ! "
                    "queue ! " RTP_CAPS_OPUS "OPUS ! sendrecv. ", &error);

            pipe->audio_element[WASAPI_SOURCE_SOUND] = 
                gst_bin_get_by_name(GST_BIN(pipe->pipeline), "audiocapsrc");
            pipe->video_element[H264_MEDIA_FOUNDATION] = 
                gst_bin_get_by_name(GST_BIN(pipe->pipeline), "videoencoder");
            pipe->video_element[RTP_H264_PAYLOAD] = 
                gst_bin_get_by_name(GST_BIN(pipe->pipeline), "rtp");
            pipe->video_element[DIRECTX_SCREEN_CAPTURE_SOURCE] = 
                gst_bin_get_by_name(GST_BIN(pipe->pipeline), "screencap");
            pipe->audio_element[OPUS_ENCODER] = 
                gst_bin_get_by_name(GST_BIN(pipe->pipeline), "audioencoder");
        }
    }
    else if (video == CODEC_H265)
    {
        if (audio == OPUS_ENC)
        {
            // setup default nvenc encoder (nvidia encoder)
            pipe->pipeline =
                gst_parse_launch("webrtcbin bundle-policy=max-bundle name=sendrecv "
                    "d3d11desktopdupsrc name=screencap ! "DIRECTX_PAD",framerate=60/1 ! "
                    "queue max-size-time=0 max-size-bytes=0 max-size-buffers=3 ! "
                    "d3d11convert ! "DIRECTX_PAD",format=NV12 ! "
                    "queue max-size-time=0 max-size-bytes=0 max-size-buffers=3 ! "
                    "mfh265enc name=videoencoder ! "
                    "queue max-size-time=0 max-size-bytes=0 max-size-buffers=3 ! "
                    "rtph265pay name=rtp ! "
                    "queue max-size-time=0 max-size-bytes=0 max-size-buffers=3 ! " 
                    RTP_CAPS_VIDEO "H265 ! sendrecv. "
                    "wasapisrc name=audiocapsrc name=audiocapsrc ! audioconvert ! audioresample ! queue ! "
                    "opusenc name=audioencoder ! rtpopuspay ! "
                    "queue ! " RTP_CAPS_OPUS "OPUS ! sendrecv. ", &error);

            pipe->audio_element[WASAPI_SOURCE_SOUND] = 
                gst_bin_get_by_name(GST_BIN(pipe->pipeline), "audiocapsrc");
            pipe->video_element[H265_MEDIA_FOUNDATION] = 
                gst_bin_get_by_name(GST_BIN(pipe->pipeline), "videoencoder");
            pipe->video_element[RTP_H265_PAYLOAD] = 
                gst_bin_get_by_name(GST_BIN(pipe->pipeline), "rtp");
            pipe->video_element[DIRECTX_SCREEN_CAPTURE_SOURCE] = 
                gst_bin_get_by_name(GST_BIN(pipe->pipeline), "screencap");
            pipe->audio_element[OPUS_ENCODER] = 
                gst_bin_get_by_name(GST_BIN(pipe->pipeline), "audioencoder");
        }
    }
    else if (video == CODEC_VP9)
    {
        if (audio == OPUS_ENC)
        {
            pipe->pipeline =
                gst_parse_launch("webrtcbin bundle-policy=max-bundle name=sendrecv "
                    "d3d11desktopdupsrc name=screencap ! "DIRECTX_PAD",framerate=120/1 ! "
                    "queue max-size-time=0 max-size-bytes=0 max-size-buffers=3 ! "
                    "d3d11convert ! "
                    "queue max-size-time=0 max-size-bytes=0 max-size-buffers=3 ! "
                    "rtpvp9enc name=rtp ! "
                    "queue max-size-time=0 max-size-bytes=0 max-size-buffers=3 ! " 
                    RTP_CAPS_VIDEO "VP9 ! sendrecv. "
                    "wasapisrc name=audiocapsrc name=audiocapsrc ! audioconvert ! audioresample ! queue ! "
                    "opusenc name=audioencoder ! rtpopuspay ! "
                    "queue ! " RTP_CAPS_OPUS "OPUS ! sendrecv. ", &error);


            pipe->audio_element[WASAPI_SOURCE_SOUND] = 
                gst_bin_get_by_name(GST_BIN(pipe->pipeline), "audiocapsrc");
            pipe->video_element[VP9_ENCODER] = 
                gst_bin_get_by_name(GST_BIN(pipe->pipeline), "videoencoder");
            pipe->video_element[RTP_VP9_PAYLOAD] = 
                gst_bin_get_by_name(GST_BIN(pipe->pipeline), "rtp");
            pipe->video_element[DIRECTX_SCREEN_CAPTURE_SOURCE] = 
                gst_bin_get_by_name(GST_BIN(pipe->pipeline), "screencap");
            pipe->audio_element[OPUS_ENCODER] = 
                gst_bin_get_by_name(GST_BIN(pipe->pipeline), "audioencoder");
        }
    }
    else if (video == CODEC_VP8)
    {
        if (audio == OPUS_ENC)
        {
            pipe->pipeline =
                gst_parse_launch("webrtcbin bundle-policy=max-bundle name=sendrecv "
                    "dxgiscreencapsrc name=screencap ! "
                    " ! queue ! videoconvert ! queue ! "
                    "vp8enc name=videoencoder ! rtpvp8pay name=rtp ! "
                    "queue ! " RTP_CAPS_VIDEO "VP8 ! sendrecv. "
                    "wasapisrc name=audiocapsrc ! audioconvert ! audioresample ! queue ! "
                    "opusenc name=audioencoder ! rtpopuspay ! "
                    "queue ! " RTP_CAPS_OPUS "OPUS ! sendrecv. ", &error);


            pipe->audio_element[WASAPI_SOURCE_SOUND] = 
                gst_bin_get_by_name(GST_BIN(pipe->pipeline), "audiocapsrc");
            pipe->video_element[VP8_ENCODER] = 
                gst_bin_get_by_name(GST_BIN(pipe->pipeline), "videoencoder");
            pipe->video_element[RTP_VP8_PAYLOAD] =
                gst_bin_get_by_name(GST_BIN(pipe->pipeline), "rtp");
            pipe->video_element[DIRECTX_SCREEN_CAPTURE_SOURCE] = 
                gst_bin_get_by_name(GST_BIN(pipe->pipeline), "screencap");
            pipe->audio_element[OPUS_ENCODER] = 
                gst_bin_get_by_name(GST_BIN(pipe->pipeline), "audioencoder");
        }
    }
    if (!error == NULL) {
        session_core_finalize(core,PIPELINE_ERROR_EXIT,error);
    }
    pipe->webrtcbin =
        gst_bin_get_by_name(GST_BIN(pipe->pipeline), "sendrecv");

}


static void
handle_media_stream (GstPad * pad, GstElement * pipe, const char *convert_name,
    const char *sink_name)
{
  GstPad *qpad;
  GstElement *q, *conv, *resample, *sink;
  GstPadLinkReturn ret;

  g_print ("Trying to handle stream with %s ! %s", convert_name, sink_name);

  q = gst_element_factory_make ("queue", NULL);
  g_assert_nonnull (q);
  conv = gst_element_factory_make (convert_name, NULL);
  g_assert_nonnull (conv);
  sink = gst_element_factory_make (sink_name, NULL);
  g_assert_nonnull (sink);

  if (g_strcmp0 (convert_name, "audioconvert") == 0) {
    /* Might also need to resample, so add it just in case.
     * Will be a no-op if it's not required. */
    resample = gst_element_factory_make ("audioresample", NULL);
    g_assert_nonnull (resample);
    gst_bin_add_many (GST_BIN (pipe), q, conv, resample, sink, NULL);
    gst_element_sync_state_with_parent (q);
    gst_element_sync_state_with_parent (conv);
    gst_element_sync_state_with_parent (resample);
    gst_element_sync_state_with_parent (sink);
    gst_element_link_many (q, conv, resample, sink, NULL);
  } else {
    gst_bin_add_many (GST_BIN (pipe), q, conv, sink, NULL);
    gst_element_sync_state_with_parent (q);
    gst_element_sync_state_with_parent (conv);
    gst_element_sync_state_with_parent (sink);
    gst_element_link_many (q, conv, sink, NULL);
  }

  qpad = gst_element_get_static_pad (q, "sink");

  ret = gst_pad_link (pad, qpad);
  g_assert_cmphex (ret, ==, GST_PAD_LINK_OK);
}

static void
on_incoming_decodebin_stream (GstElement * decodebin, GstPad * pad,
    GstElement * pipe)
{
  GstCaps *caps;
  const gchar *name;

  if (!gst_pad_has_current_caps (pad)) {
    g_printerr ("Pad '%s' has no caps, can't do anything, ignoring\n",
        GST_PAD_NAME (pad));
    return;
  }

  caps = gst_pad_get_current_caps (pad);
  name = gst_structure_get_name (gst_caps_get_structure (caps, 0));

  if (g_str_has_prefix (name, "video")) {
    handle_media_stream (pad, pipe, "videoconvert", "autovideosink");
  } else if (g_str_has_prefix (name, "audio")) {
    handle_media_stream (pad, pipe, "audioconvert", "autoaudiosink");
  } else {
    g_printerr ("Unknown pad %s, ignoring", GST_PAD_NAME (pad));
  }
}

static void
on_incoming_stream (GstElement * webrtc, GstPad * pad, GstElement * pipe)
{
  GstElement *decodebin;
  GstPad *sinkpad;

  if (GST_PAD_DIRECTION (pad) != GST_PAD_SRC)
    return;

  decodebin = gst_element_factory_make ("decodebin", NULL);
  g_signal_connect (decodebin, "pad-added",
      G_CALLBACK (on_incoming_decodebin_stream), pipe);
  gst_bin_add (GST_BIN (pipe), decodebin);
  gst_element_sync_state_with_parent (decodebin);

  sinkpad = gst_element_get_static_pad (decodebin, "sink");
  gst_pad_link (pad, sinkpad);
  gst_object_unref (sinkpad);
}




/// <summary>
/// connect webrtc bin to ice and sdp signal handler
/// </summary>
/// <param name="core"></param>
static void
connect_signalling_handler(SessionCore* core)
{
    Pipeline* pipe = session_core_get_pipeline(core);
    SignallingHub* hub = session_core_get_signalling_hub(core);

    g_main_context_push_thread_default(session_core_get_main_context(core));
    /* Add stun server */
    g_object_set(pipe->webrtcbin, "stun-server", 
       "stun://stun.thinkmay.net:3478", NULL);

    g_signal_emit_by_name (pipe->webrtcbin, "add-turn-server", 
        signalling_hub_get_turn_server(hub), NULL);


    /* This is the gstwebrtc entry point where we create the offer and so on. It
     * will be called when the pipeline goes to PLAYING. */
    g_signal_connect(pipe->webrtcbin, "on-negotiation-needed",
        G_CALLBACK(on_negotiation_needed), core);
    g_signal_connect(pipe->webrtcbin, "on-ice-candidate",
        G_CALLBACK(send_ice_candidate_message), core);
    g_signal_connect(pipe->webrtcbin, "notify::ice-gathering-state",
        G_CALLBACK(on_ice_gathering_state_notify), core);
    g_signal_connect(pipe->webrtcbin, "pad-added", 
       G_CALLBACK(on_incoming_stream),pipe->pipeline);
}






/// <summary>
/// setup proerty of gst element,
/// this function should be called after pipeline factory has been done,
/// each element are assigned to an element in pipeline
/// </summary>
/// <param name="core"></param>
static void
setup_element_property(SessionCore* core)
{
    Pipeline* pipe = session_core_get_pipeline(core);
    SignallingHub* hub = session_core_get_signalling_hub(core);
    QoE* qoe = session_core_get_qoe(core);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (pipe->video_element[DIRECTX_SCREEN_CAPTURE_SOURCE]) { g_object_set(pipe->video_element[DIRECTX_SCREEN_CAPTURE_SOURCE], "show-cursor", TRUE, NULL);}
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (pipe->video_element[NVIDIA_H264_ENCODER]) { g_object_set(pipe->video_element[NVIDIA_H264_ENCODER], "bframes", 0, NULL);}

    if (pipe->video_element[NVIDIA_H264_ENCODER]) { g_object_set(pipe->video_element[NVIDIA_H264_ENCODER], "zerolatency", TRUE, NULL);}

    if (pipe->video_element[NVIDIA_H264_ENCODER]) { g_object_set(pipe->video_element[NVIDIA_H264_ENCODER], "rc-mode", "cbr", NULL);}

    if (pipe->video_element[NVIDIA_H264_ENCODER]) { g_object_set(pipe->video_element[NVIDIA_H264_ENCODER], "qos", TRUE, NULL);}

    if (pipe->video_element[NVIDIA_H264_ENCODER]) { g_object_set(pipe->video_element[NVIDIA_H264_ENCODER], "preset", "low-latency", NULL);}
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (pipe->video_element[H264_MEDIA_FOUNDATION]) { g_object_set(pipe->video_element[H264_MEDIA_FOUNDATION], "rc-mode", 0, NULL);}

    if (pipe->video_element[H264_MEDIA_FOUNDATION]) { g_object_set(pipe->video_element[H264_MEDIA_FOUNDATION], "quality-vs-speed", 100, NULL);}
    
    if (pipe->video_element[H264_MEDIA_FOUNDATION]) { g_object_set(pipe->video_element[H264_MEDIA_FOUNDATION], "low-latency", TRUE, NULL);}
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
    
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (pipe->video_element[H265_MEDIA_FOUNDATION]) { g_object_set(pipe->video_element[H265_MEDIA_FOUNDATION], "rc-mode", 0, NULL); }

    if (pipe->video_element[H265_MEDIA_FOUNDATION]) { g_object_set(pipe->video_element[H265_MEDIA_FOUNDATION], "quality-vs-speed", 100, NULL); }

    if (pipe->video_element[H265_MEDIA_FOUNDATION]) { g_object_set(pipe->video_element[H265_MEDIA_FOUNDATION], "bitrate", 5000, NULL); }

    if (pipe->video_element[H265_MEDIA_FOUNDATION]) { g_object_set(pipe->video_element[H265_MEDIA_FOUNDATION], "low-latency", TRUE, NULL); }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (pipe->video_element[RTP_H264_PAYLOAD]) { g_object_set(pipe->video_element[RTP_H264_PAYLOAD], "aggregate-mode", 1, NULL);}
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (pipe->video_element[RTP_H265_PAYLOAD]) { g_object_set(pipe->video_element[RTP_H265_PAYLOAD], "aggregate-mode", 1, NULL); }
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (pipe->audio_element[WASAPI_SOURCE_SOUND]) { g_object_set(pipe->audio_element[WASAPI_SOURCE_SOUND], "low-latency", TRUE, NULL);}
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (pipe->audio_element[RTP_RTX_QUEUE]) { g_object_set(pipe->audio_element[RTP_RTX_QUEUE], "max-size-time", 16000000, NULL);}

    if (pipe->audio_element[RTP_RTX_QUEUE]) { g_object_set(pipe->audio_element[RTP_RTX_QUEUE], "max-size-packet", 0, NULL);}
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    g_object_set(pipe->webrtcbin,"latency",0,NULL);
}


void
toggle_pointer(gboolean toggle, SessionCore* core)
{
    Pipeline* pipe = session_core_get_pipeline(core);
    if (pipe->video_element[DIRECTX_SCREEN_CAPTURE_SOURCE]) 
    { 
        g_object_set(pipe->video_element[DIRECTX_SCREEN_CAPTURE_SOURCE], "show-cursor", toggle, NULL); 
    }
}




gpointer
setup_pipeline(SessionCore* core)
{
    SignallingHub* signalling = session_core_get_signalling_hub(core);
    Pipeline* pipe = session_core_get_pipeline(core);
    QoE* qoe= session_core_get_qoe(core);


    pipe->state = PIPELINE_CREATING_ELEMENT;

    

    setup_element_factory(core, 
        qoe_get_video_codec(qoe),
        qoe_get_audio_codec(qoe));

    pipe->state = PIPELINE_CONNECT_ELEMENT_SIGNAL;
    connect_signalling_handler(core);
    
    pipe->state = PIPELINE_SETTING_UP_ELEMENT;
    setup_element_property(core);



    gst_element_change_state(pipe->pipeline, GST_STATE_READY);
    pipe->state = PIPELINE_SETUP_DONE;
    connect_data_channel_signals(core);
    start_pipeline(core);
    session_core_set_state(core, REMOTE_CONNECT_STARTED);
    signalling_hub_set_peer_call_state(signalling, PEER_CALL_DONE);
}






GstElement*
pipeline_get_webrtc_bin(Pipeline* pipe)
{
    return pipe->webrtcbin;
}

GstElement*
pipeline_get_pipline(Pipeline* pipe)
{
    return pipe->pipeline;
}

PipelineState
pipeline_get_state(Pipeline* pipe)
{
    return pipe->state;
}

void
pipeline_set_state(Pipeline* pipe,
                    PipelineState state)
{
    pipe->state = state;
}

GstElement*
pipeline_get_video_encoder(Pipeline* pipe, Codec video)
{
    if (pipe->video_element[H264_MEDIA_FOUNDATION] != NULL) 
    { return pipe->video_element[H264_MEDIA_FOUNDATION];}
    if (pipe->video_element[NVIDIA_H264_ENCODER] != NULL) 
    { return pipe->video_element[NVIDIA_H264_ENCODER];}
    if (pipe->video_element[H265_MEDIA_FOUNDATION] != NULL) 
    { return pipe->video_element[H265_MEDIA_FOUNDATION];}
    if (pipe->video_element[NVIDIA_H265_ENCODER] != NULL) 
    { return pipe->video_element[NVIDIA_H265_ENCODER];}

    if (pipe->video_element[VP9_ENCODER] != NULL) 
    { return pipe->video_element[VP9_ENCODER];}
    if (pipe->video_element[VP8_ENCODER] != NULL) 
    { return pipe->video_element[VP8_ENCODER];}    
    return NULL;
}

GstElement*
pipeline_get_audio_encoder(Pipeline* pipe, Codec audio)
{
    
    if (audio == OPUS_ENC) { return pipe->audio_element[OPUS_ENCODER];}
    else if (audio == AAC_ENC) { return pipe->audio_element[AAC_ENCODER];}
    return NULL;
}
