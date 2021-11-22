#ifndef __AGENT_OBJECT_H__
#define __AGETN_OBJECT_H__
#include <glib.h>


#include <agent-type.h>
#include <agent-device.h>
 
#include <Windows.h>




/// <summary>
/// start websocket connection with host, 
/// invoke during initialization of agent object
/// run recursively until connection has been established
/// </summary>
/// <param name="self"></param>
/// <returns></returns>
void											agent_connect_to_host				(AgentServer* self);


/// <summary>
/// agent disconnect host connection,
/// </summary>
/// <param name="self"></param>
/// <returns></returns>
void											agent_disconnect_host				(AgentServer* self);


/// <summary>
/// session initialize, invoke when request from server is received,
/// wrap aroun session initialize function from agent-ipc
/// </summary>
/// <param name="self"></param>
/// <returns></returns>
void											agent_session_initialize			(AgentServer* self);


/// <summary>
/// agent session terminate session, 
/// wrap around session_terminate function from agent-ipc
/// </summary>
/// <param name="self"></param>
/// <returns></returns>
void											agent_session_terminate				(AgentServer* self);

/// <summary>
/// agent remote control disconnect function, 
/// wrap around remote control disconnect function
/// </summary>
/// <param name="self"></param>
/// <returns></returns>
void											agent_remote_control_disconnect		(AgentServer* self);

/// <summary>
/// agent remote control reconnect function,
/// wrap around remote control reconnect function
/// </summary>
/// <param name="self"></param>
/// <returns></returns>
void											agent_remote_control_reconnect		(AgentServer* self);



/// <summary>
/// handle cmd process termination, based on current connection state with host,
/// if in connection, agent will report to host and cmd process termination event will be recorded
/// </summary>
/// <param name="self"></param>
void         									agent_on_shell_process_terminate	(AgentServer* self, 
																					 gint process_id);

/// <summary>
/// agent_finalize, end main loop and all related thread, 
/// close websocket connection and close agent process
/// </summary>
/// <param name="object"></param>
void											agent_finalize						(AgentServer* object);
/// <summary>
/// Create new agent object based on information of host
/// this initialize step will include allocate heap and connect to host
/// </summary>
/// <param name="Host_URL"></param>
/// <param name="Host_ID"></param>
/// <returns></returns>
AgentServer*									agent_new							();

/// <summary>
/// handle message from host
/// </summary>
/// <param name="self"></param>
void											handle_host_connection				(AgentServer* self);


/// <summary>
/// send message to arbitrary device in system
/// </summary>
/// <param name="self"></param>
/// <param name="message"></param>
/// <returns></returns>
void											agent_send_message_to_host			(AgentServer* self,
																					gchar* message);
/// <summary>
/// 
/// </summary>
/// <param name="self"></param>
/// <param name="message"></param>
void											agent_send_message_to_session_core	(AgentServer* self,
																					 gchar* message);


/// <summary>
/// 
/// </summary>
/// <param name="server_commmand"></param>
void											agent_start_file_transfer			(gchar* server_commmand);


/// <summary>
/// register slave device with host by sending SLAVE_REGISTER message,
/// only invoke when agent in attemp to reconnect state
/// </summary>
/// <param name="self"></param>
void											agent_register_with_host			(AgentServer* self);



/// <summary>
/// handle session core exit event, depend on agent state, 
/// it will be reported to server to handle remote control disconnected 
/// otherwise just ignored 
/// </summary>
/// <param name="self"></param>
void											agent_on_session_core_exit		(AgentServer* self);


/*get-set function for agent object*/
Socket*											agent_get_socket					(AgentServer* self);




void											agent_set_main_loop					(AgentServer* agent,
																					 GMainLoop* loop);


gchar*											agent_get_current_state_string		(AgentServer* self);

ChildProcess*									agent_get_child_process				(AgentServer* self, 
																					 gint position);

void											agent_set_child_process				(AgentServer* self,
																				     gint postion,
																					 ChildProcess* process);

void											agent_set_file_transfer_service		(AgentServer* self,
																					 gint position,
																					 FileTransferSession* session);

RemoteSession*									agent_get_remote_session			(AgentServer* self);

void											agent_set_remote_session			(AgentServer* self, 
																					 RemoteSession* session);

GMainLoop*										agent_get_main_loop					(AgentServer* self);

void											agent_server_set_token				(AgentServer* self,
																					 gchar* token);
/*END get-set function for agent object*/


#endif // !__AGENT_OBJECT__