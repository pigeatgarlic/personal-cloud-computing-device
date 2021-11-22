#include <session-core-data-channel.h>
#include <session-core.h>
#include <session-core-type.h>
#include <session-core-pipeline.h>
#include <session-core-remote-config.h>


#include <logging.h>
#include <human-interface-opcode.h>
#include <key-convert.h>

#include <gst/gst.h>
#include <glib-2.0/glib.h>
#include <gst/webrtc/webrtc_fwd.h>
#include <Windows.h>
#include <libsoup/soup.h>












struct _WebRTCHub
{
    /// <summary>
    /// hid datachannel responsible for receive human interface device input from user,
    /// </summary>
    GObject* hid;
    /// <summary>
    /// control datachannel responsible for receive other command from user include update current qoe metric
    /// to adjust video stream quality
    /// </summary>
    GObject* control;

    /// <summary>
    ///  
    /// </summary>
    gboolean relative_mouse;
};

static WebRTCHub hub_init = {0};

WebRTCHub* 
webrtchub_initialize()
{
    hub_init.relative_mouse = FALSE;
    return &hub_init;
}















/// <summary>
/// handle data message from client,
/// if destination is not session core, forward it using shared memory
/// </summary>
/// <param name="datachannel"></param>
/// <param name="byte"></param>
/// <param name="core"></param>
static void
control_channel_on_message_data(GObject* datachannel,
    GBytes* byte,
    SessionCore* core)
{
    return;
}

/// <summary>
/// handle data from hid byte data channel,
/// this channel only responsible for parsing HID device 
/// </summary>
/// <param name="datachannel"></param>
/// <param name="data"></param>
/// <param name="core"></param>
static void
hid_channel_on_message_data(GObject* datachannel,
    GBytes* data,
    SessionCore* core)
{
    return;
}



static void
convert_mouse_input(INPUT* input, 
                    Message* message,
                    SessionCore* core)
{
    QoE* qoe = session_core_get_qoe(core);
    WebRTCHub* hub = session_core_get_rtc_hub(core);

    static gfloat screenwidth, screenheight; 
    screenwidth =   qoe_get_screen_width(qoe);
    screenheight =  qoe_get_screen_height(qoe);

    if(hub->relative_mouse)
    {
        input->mi.dx = (LONG)
        ((gfloat)json_object_get_int_member(message, "dX"));
        input->mi.dy = (LONG)
        ((gfloat)json_object_get_int_member(message, "dY"));
    }
    else
    {
        input->mi.dx = (LONG)
        ((((gfloat)json_object_get_int_member(message, "dX"))/screenwidth)*65535);
        input->mi.dy = (LONG)
        ((((gfloat)json_object_get_int_member(message, "dY"))/screenheight)*65535);
    }

}


/// <summary>
/// handle message from hid datachannel  
/// </summary>
/// <param name="dc"></param>
/// <param name="message"></param>
/// <param name="core"></param>
static void
hid_channel_on_message_string(GObject* dc,
    gchar* message,
    SessionCore* core)
{
    WebRTCHub* hub = session_core_get_rtc_hub(core); 

    GError* error = NULL;
    JsonParser* parser = json_parser_new();
    Message* object = get_json_object_from_string(message,&error,parser);
	if(!error == NULL || object == NULL) {return;}

    Opcode opcode = json_object_get_int_member(object, "Opcode");

    if (opcode == MOUSE_UP)
    {
        INPUT mouse;
        memset(&mouse,0, sizeof(mouse));
        mouse.type = INPUT_MOUSE;
        gint button = json_object_get_int_member(object, "button");
        if(hub->relative_mouse)
        {
            if(button == 0){
                mouse.mi.dwFlags =  MOUSEEVENTF_LEFTUP | MOUSEEVENTF_VIRTUALDESK;
            }else if(button == 1){
                mouse.mi.dwFlags =  MOUSEEVENTF_MIDDLEUP | MOUSEEVENTF_VIRTUALDESK;
            }else if (button == 2) {
                mouse.mi.dwFlags =  MOUSEEVENTF_RIGHTUP | MOUSEEVENTF_VIRTUALDESK;
            }
        }
        else
        {
            convert_mouse_input(&mouse,object,core);
            if(button == 0){
                mouse.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP | MOUSEEVENTF_VIRTUALDESK;
            }else if(button == 1){
                mouse.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MIDDLEUP | MOUSEEVENTF_VIRTUALDESK;
            }else if (button == 2) {
                mouse.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTUP | MOUSEEVENTF_VIRTUALDESK;
            }
        }
        mouse.mi.mouseData = 0;
        mouse.mi.time = 0;
        SendInput(1, &mouse, sizeof(mouse));
    }
    else if (opcode == MOUSE_DOWN)
    {
        INPUT mouse;
        memset(&mouse,0, sizeof(mouse));
        mouse.type = INPUT_MOUSE;
        gint button = json_object_get_int_member(object, "button");
        if(hub->relative_mouse)
        {
            if(button == 0){
                mouse.mi.dwFlags =  MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_VIRTUALDESK;
            }else if(button == 1){
                mouse.mi.dwFlags =  MOUSEEVENTF_MIDDLEDOWN | MOUSEEVENTF_VIRTUALDESK;
            }else if (button == 2) {
                mouse.mi.dwFlags =  MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_VIRTUALDESK;
            }
        }
        else
        {
            convert_mouse_input(&mouse,object,core);
            if(button == 0){
                mouse.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_VIRTUALDESK;
            }else if(button == 1){
                mouse.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MIDDLEDOWN | MOUSEEVENTF_VIRTUALDESK;
            }else if (button == 2) {
                mouse.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_VIRTUALDESK;
            }
        }
        mouse.mi.mouseData = 0;
        mouse.mi.time = 0;
        SendInput(1, &mouse, sizeof(mouse));
    }
    else if (opcode == MOUSE_MOVE)
    {
        INPUT mouse;
        memset(&mouse,0, sizeof(mouse));
        mouse.type = INPUT_MOUSE;
        convert_mouse_input(&mouse,object,core);
        if(hub->relative_mouse)
        {
            mouse.mi.dwFlags = MOUSEEVENTF_MOVE;
        }
        else
        {
            mouse.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
        }
        SendInput(1, &mouse, sizeof(mouse));
    }
    else if(opcode == MOUSE_WHEEL)
    {
        INPUT mouse;
        memset(&mouse,0, sizeof(mouse));
        mouse.type = INPUT_MOUSE;
        convert_mouse_input(&mouse,object,core);
        mouse.mi.dwFlags = MOUSEEVENTF_WHEEL;
        mouse.mi.mouseData = (LONG)json_object_get_int_member(object, "WheeldY");
        mouse.mi.time = 0;
        SendInput(1, &mouse, sizeof(mouse));
    }
    else if (opcode == KEYUP)
    {
        INPUT keyboard;
        memset(&keyboard,0, sizeof(keyboard));
        keyboard.type = INPUT_KEYBOARD;
        keyboard.ki.wVk = convert_javascript_key_to_window_key(
            json_object_get_string_member(object, "wVk"));
        keyboard.ki.dwFlags = KEYEVENTF_KEYUP;
        keyboard.ki.time = 0;
        SendInput(1, &keyboard, sizeof(keyboard));
    }
    else if (opcode == KEYDOWN)
    {
        INPUT keyboard;
        memset(&keyboard,0, sizeof(keyboard));
        keyboard.type =  INPUT_KEYBOARD;
        keyboard.ki.wVk = convert_javascript_key_to_window_key(
            json_object_get_string_member(object, "wVk"));
        keyboard.ki.time = 0;
        SendInput(1, &keyboard, sizeof(keyboard));
    }
    else if (opcode == POINTER_LOCK)
    {
        gboolean relative = json_object_get_boolean_member(object,"Value");
        hub->relative_mouse = relative; 
        toggle_pointer(relative,core);
    }
    else if(opcode == QOE_REPORT)
    {
        JsonObject* object = json_object_get_string_member(object,"Value");
        if(!object){return;}
        const gchar* http_aliases[] = { "http", NULL };

        GString* string = g_string_new("http://");
        g_string_append(string,"220");
        g_string_append(string,":2220/session/SessionMetric");
        gchar* uri = g_string_free(string,FALSE);


        SoupSession* session = soup_session_new_with_options(
                SOUP_SESSION_SSL_STRICT, FALSE,
                SOUP_SESSION_SSL_USE_SYSTEM_CA_FILE, TRUE,
                SOUP_SESSION_HTTPS_ALIASES, http_aliases, NULL);

        SoupMessage* message = soup_message_new(SOUP_METHOD_POST,uri);

        gchar* msgjson = get_string_from_json_object(object);
        soup_message_set_request(message,"application/text",SOUP_MEMORY_COPY
            ,msgjson,strlen(msgjson));
        soup_session_send_message(session,message);
    }
    g_object_unref(parser);
}


/// <summary>
/// handle message from user through control data channel
/// </summary>
/// <param name="dc"></param>
/// <param name="message"></param>
/// <param name="core"></param>
static void
control_channel_on_message_string(GObject* dc,
    gchar* message,
    SessionCore* core)
{
    return;
}




static void
channel_on_open(GObject* dc,
                SessionCore* core)
{
    return;
}


static void
channel_on_close_and_error(GObject* dc,
    SessionCore* core)
{
    return;
}









/// Connect webrtcbin to data channel, connect data channel signal to callback function
/// </summary>
/// <param name="core"></param>
/// <param name="user_data"></param>
/// <returns></returns>
gboolean
connect_data_channel_signals(SessionCore* core)
{

    WebRTCHub* hub = session_core_get_rtc_hub(core);
    Pipeline* pipe = session_core_get_pipeline(core);
    GstElement* webrtcbin = pipeline_get_webrtc_bin(pipe);


    // connect data channel source
    g_signal_emit_by_name(webrtcbin, "create-data-channel", "Control", 
        NULL, &hub->control);
    g_signal_emit_by_name(webrtcbin, "create-data-channel", "HID", 
        NULL, &hub->hid);
    
    g_signal_connect(hub->hid, "on-error",
        G_CALLBACK(channel_on_close_and_error), core);
    g_signal_connect(hub->hid, "on-open",
        G_CALLBACK(channel_on_open), core);
    g_signal_connect(hub->hid, "on-close",
        G_CALLBACK(channel_on_close_and_error), core);
    g_signal_connect(hub->hid, "on-message-string",
        G_CALLBACK(hid_channel_on_message_string), core);
    g_signal_connect(hub->hid, "on-message-data",
        G_CALLBACK(hid_channel_on_message_data), core);

    g_signal_connect(hub->control, "on-error",
        G_CALLBACK(channel_on_close_and_error), core);
    g_signal_connect(hub->control, "on-open",
        G_CALLBACK(channel_on_open), core);
    g_signal_connect(hub->control, "on-close",
        G_CALLBACK(channel_on_close_and_error), core);
    g_signal_connect(hub->control, "on-message-string",
        G_CALLBACK(control_channel_on_message_string), core);
    g_signal_connect(hub->control, "on-message-data",
        G_CALLBACK(control_channel_on_message_data), core);
    return TRUE;
}



GstWebRTCDataChannel*
webrtc_hub_get_control_data_channel(WebRTCHub* hub)
{
    return hub->control;
}