
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

#include "common.h"
#include <dlog.h>
#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <bluetooth.h>

#undef LOG_TAG
#define LOG_TAG "REMOTE_KEY_FW"

static GMainLoop* gMainLoop = NULL;
static bt_adapter_visibility_mode_e gVisibilityMode = BT_ADAPTER_VISIBILITY_MODE_NON_DISCOVERABLE;
static int gSocketFd = -1;
static char* bt_address = NULL;
static char* server_name = "SIOR";
static bt_adapter_state_e gBtState = BT_ADAPTER_DISABLED;
static int bonding_state = BT_ERROR_OPERATION_FAILED;
static const char uuid[] = "00001101-0000-1000-8000-00805F9B34FB";

// Lifecycle of this framework
int rkf_initialize_bluetooth(void);
int rkf_finalize_bluetooth_socket(void);
int rkf_finalize_bluetooth(void);
int rkf_listen_connection(void);
int rkf_send_data(const char *, int);
void rkf_main_loop(void);

// Callbacks
void rkf_received_data_cb(bt_socket_received_data_s *, void *);
void rkf_socket_connection_state_changed_cb(int, bt_socket_connection_state_e, bt_socket_connection_s *, void *);
void rkf_state_changed_cb(int, bt_adapter_state_e, void *);
gboolean timeout_func_cb(gpointer);

void bt_device_bond_created_impl(int result, bt_device_info_s *device_info, void *user_data)
{
    if(device_info != NULL && !strcmp(device_info->remote_address, bt_address))
    {
        bonding_state = result;
        if(result == BT_ERROR_NONE)
        {
            ALOGI("[%s] Callback: A bond with chat_server is created.", __FUNCTION__);
            LOGI("[%s] Callback: The number of service - %d.", __FUNCTION__, device_info->service_count);

            int i = 0;
            for(i=0; i<device_info->service_count; i++)
            {
                ALOGI("[%s] Callback: service[%d] - %s", __FUNCTION__, i+1, device_info->service_uuid[i]);
            }
            ALOGI("[%s] Callback: is_bonded - %d.", __FUNCTION__, device_info->is_bonded);
            ALOGI("[%s] Callback: is_connected - %d.", __FUNCTION__, device_info->is_connected);
        }
        else
        {
            ALOGE("[%s] Callback: Creating a bond is failed.", __FUNCTION__);
        }
    }
    else
    {
        ALOGE("[%s] Callback: A bond with another device is created.", __FUNCTION__);
    }

    if(gMainLoop)
    {
        g_main_loop_quit(gMainLoop);
    }
}

void bt_socket_connection_state_changed_impl(int result, bt_socket_connection_state_e connection_state,
    bt_socket_connection_s *connection, void *user_data)
{
    if(result == BT_ERROR_NONE)
    {
        ALOGI("[%s] Callback: Result is BT_ERROR_NONE.", __FUNCTION__);
    }
    else
    {
        ALOGI("[%s] Callback: Result is not BT_ERROR_NONE.", __FUNCTION__);
    }

    if(connection_state == BT_SOCKET_CONNECTED)
    {
        ALOGI("[%s] Callback: Connected.", __FUNCTION__);
        if(result == BT_ERROR_NONE && connection != NULL)
        {
            gSocketFd = connection->socket_fd;
            ALOGI("[%s] Callback: Socket of connection - %d.", __FUNCTION__, gSocketFd);
            ALOGI("[%s] Callback: Role of connection - %d.", __FUNCTION__, connection->local_role);
            ALOGI("[%s] Callback: Address of connection - %s.", __FUNCTION__, connection->remote_address);
/*
            if(bt_socket_send_data(socket_fd, quit_command, strlen(quit_command)) == BT_ERROR_NONE)
            {
                LOGI("[%s] Callback: Send quit command.", __FUNCTION__);
            }
            else
            {
                LOGE("[%s] Callback: bt_socket_send_data() failed.", __FUNCTION__);
                if(g_mainloop)
                {
                    g_main_loop_quit(g_mainloop);
                }
            }
*/
        }
        else
        {
            ALOGI("[%s] Callback: Failed to connect", __FUNCTION__);
            if(gMainLoop)
            {
                g_main_loop_quit(gMainLoop);
            }
        }
    }
    else
    {
        ALOGI("[%s] Callback: Disconnected.", __FUNCTION__);
    }
}


void bt_adapter_device_discovery_state_changed_impl(int result, bt_adapter_device_discovery_state_e discovery_state,
    bt_adapter_device_discovery_info_s *discovery_info, void *user_data)
{
    if(discovery_state == BT_ADAPTER_DEVICE_DISCOVERY_FOUND)
    {
        if(discovery_info->remote_address != NULL && !strcmp(discovery_info->remote_name, server_name))
        {
            ALOGI("[%s] Callback: chat_server is found.", __FUNCTION__);
            ALOGI("[%s] Callback: Address of chat_server - %s.", __FUNCTION__, discovery_info->remote_address);
            ALOGI("[%s] Callback: Device major class - %d.", __FUNCTION__, discovery_info->bt_class.major_device_class);
            ALOGI("[%s] Callback: Device minor class - %d.", __FUNCTION__, discovery_info->bt_class.minor_device_class);
            ALOGI("[%s] Callback: Service major class - %d.", __FUNCTION__, discovery_info->bt_class.major_service_class_mask);
            bt_address = strdup(discovery_info->remote_address);
            ALOGI("[%s] Callback: is_bonded - %d.", __FUNCTION__, discovery_info->is_bonded);
            bt_adapter_stop_device_discovery();
        }
        else
        {
            LOGE("[%s] Callback: Another device is found.", __FUNCTION__);
        }
    }
    else if(discovery_state == BT_ADAPTER_DEVICE_DISCOVERY_FINISHED)
    {
        ALOGI("[%s] Callback: device discovery finished.", __FUNCTION__);
        if(gMainLoop)
        {
            g_main_loop_quit(gMainLoop);
        }
    }
}

void bt_socket_data_received_impl(bt_socket_received_data_s *data, void *user_data)
{
    if(gSocketFd == data->socket_fd)
    {
		char* message = (char*) data->data;
        if(data->data_size > 2)
        {
			ALOGI("Data received");
		//	ALOGI("Len1 : %x\n", message[0]);
		//	ALOGI("Len2 : %x\n", message[1]);
			ALOGI("Data1 : %x", message[2]);
			ALOGI("Data2 : %x", message[3]);
			ALOGI("Data3 : %x", message[4]);
			if (message[3] == 0x0E) {
				int sonarReadBytes
				ALOGI("Read LSGETSTATUS : %x", message[5]);
				sonarReadBytes = message[5];
				if (sonarReadBytes >= 1) {
					char data4[5] = { 0x03, 0x00, 0x00, 0x10, 0x03/*port*/};
					rkf_send_data(data4, 5);
				}
			}
			else if (message[3] == 0x10) {
				FILE* fp = fopen("/usr/share/nodejs/ehf_sonar.tail", "a+");
				ALOGI("Read Bytes : %d", (int)message[5]);
				ALOGI("Read Data1 : %d", (int)message[6]);
				if (fp) {				
					fprintf(fp, "%d\n", (int)message[6]);
					fclose(fp);
				}
				else
					ALOGI("Can not write /usr/share/nodejs/ehf_sonar.tail");
			} 
        }
        else
        {
            ALOGE("[%s] Callback: No data.", __FUNCTION__);
        }
    }
    else
    {
        ALOGI("[%s] Callback: Another socket - %d.", __FUNCTION__, data->socket_fd);
    }
}

bool bt_adapter_bonded_device_impl(bt_device_info_s *device_info, void *user_data)
{
    int i = 0;
    if(device_info != NULL)
    {
        if(device_info->remote_name != NULL && !strcmp(device_info->remote_name, (char*)user_data))
        {
            ALOGI("[%s] Callback: chat_server is found in bonded list.", __FUNCTION__);
            if( device_info->remote_address != NULL )
            {
                ALOGI("[%s] Callback: Address of chat_server - %s.", __FUNCTION__, device_info->remote_address);
                bt_address = strdup(device_info->remote_address);
                ALOGI("[%s] Callback: The number of service_count - %d.", __FUNCTION__, device_info->service_count);
                if(device_info->service_count <= 0)
                {
                    bonding_state = BT_ERROR_SERVICE_SEARCH_FAILED;
                }
                else
                {
                    bonding_state = BT_ERROR_NONE;
                    for(i=0; i<device_info->service_count; i++)
                    {
                        ALOGI("[%s] Callback: service[%d] - %s", __FUNCTION__, i+1, device_info->service_uuid[i]);
                    }
                    ALOGI("[%s] Callback: is_bonded - %d.", __FUNCTION__, device_info->is_bonded);
                    ALOGI("[%s] Callback: is_connected - %d.", __FUNCTION__, device_info->is_connected);
                    ALOGI("[%s] Callback: is_authorized - %d.", __FUNCTION__, device_info->is_authorized);
                }
            }
            else
            {
                ALOGE("[%s] Callback: Address of chat_server is NULL.", __FUNCTION__);
            }

            return false;
        }
    }

    return true;
}

void bt_device_service_searched_impl(int result, bt_device_sdp_info_s* sdp_info, void* user_data)
{
    if(sdp_info != NULL && !strcmp(sdp_info->remote_address, bt_address))
    {
        bonding_state = result;
        if(result == BT_ERROR_NONE)
        {
            ALOGI("[%s] Callback: Services of chat_service are found.", __FUNCTION__);
            ALOGI("[%s] Callback: The number of service - %d.", __FUNCTION__, sdp_info->service_count);

            int i = 0;
            for(i = 0; i < sdp_info->service_count; i++)
            {
                ALOGI("[%s] Callback: service[%d] - %s", __FUNCTION__, i+1, sdp_info->service_uuid[i]);
            }
        }
    }
    else
    {
        ALOGE("[%s] Callback: Services of another device are found.", __FUNCTION__);
    }

    if(gMainLoop)
    {
        g_main_loop_quit(gMainLoop);
    }
}


int rkf_initialize_bluetooth(const char *device_name) {
	// Initialize bluetooth and get adapter state
	int ret;
	ret = bt_initialize();
	if(ret != BT_ERROR_NONE) {
		ALOGD("Unknown exception is occured in bt_initialize(): %x", ret);
		return -1;
	}

	ret = bt_adapter_get_state(&gBtState);
	if(ret != BT_ERROR_NONE) {
		ALOGD("Unknown exception is occured in bt_adapter_get_state(): %x", ret);
		return -2;
	}

	// Enable bluetooth device manually
	if(gBtState == BT_ADAPTER_DISABLED)
	{
		if(bt_adapter_set_state_changed_cb(rkf_state_changed_cb, NULL) != BT_ERROR_NONE)
		{
			ALOGE("[%s] bt_adapter_set_state_changed_cb() failed.", __FUNCTION__);
			return -3;
		}

		if(bt_adapter_enable() == BT_ERROR_NONE)
		{
			int timeout_id = -1;
			ALOGE("[%s] bt_adapter_state_changed_cb will be called.", __FUNCTION__);
			timeout_id = g_timeout_add(60000, timeout_func_cb, gMainLoop);
			g_main_loop_run(gMainLoop);
			g_source_remove(timeout_id);
		}
		else
		{
			ALOGE("[%s] bt_adapter_enable() failed.", __FUNCTION__);
			return -4;
		}
	}
	else
	{
		ALOGI("[%s] BT was already enabled.", __FUNCTION__);
	}

	// Set adapter's name
	if(gBtState == BT_ADAPTER_ENABLED) {
		char *name = NULL;
//		ret = bt_adapter_get_name(&name);
//		if(name == NULL) {
//			ALOGD("NULL name exception is occured in bt_adapter_get_name(): %x", ret);
//			return -5;
//		}

//		if(strncmp(name, device_name, strlen(name)) != 0) {
//			if(bt_adapter_set_name(device_name) != BT_ERROR_NONE)
//			{   
//				if (NULL != name)
//					free(name);
//				ALOGD("Unknown exception is occured in bt_adapter_set_name : %x", ret);
//				return -6;
//			}   
//		}
//		free(name);

		if(bt_adapter_foreach_bonded_device(bt_adapter_bonded_device_impl, server_name) != BT_ERROR_NONE) {
            ALOGE("[%s] bt_adapter_foreach_bonded_device() failed.", __FUNCTION__);
            return -7;
        }	

		if(bt_address == NULL)
		{
			if(bt_adapter_set_device_discovery_state_changed_cb(bt_adapter_device_discovery_state_changed_impl, NULL)
							!= BT_ERROR_NONE )
			{
				ALOGE("[%s] bt_adapter_set_device_discovery_state_changed_cb() failed.", __FUNCTION__);
				return -1;
			}

			if(bt_adapter_start_device_discovery() == BT_ERROR_NONE)
			{
				ALOGI("[%s] bt_adapter_device_discovery_state_changed_cb will be called.", __FUNCTION__);
				g_main_loop_run(gMainLoop);
			}
			else
			{
				ALOGE("[%s] bt_adapter_start_device_discovery() failed.", __FUNCTION__);
				return -1;
			}
		}
		else
		{
			ALOGI("[%s] chat_server is found in bonded device list.", __FUNCTION__);
		}
	}
	else
	{
		ALOGE("[%s] BT is not enabled.", __FUNCTION__);
		return -1;
	}

	// Create bond with a server
    if(bonding_state == BT_ERROR_SERVICE_SEARCH_FAILED)
    {
        if(bt_device_set_service_searched_cb(bt_device_service_searched_impl, NULL) != BT_ERROR_NONE)
        {
            ALOGE("[%s] bt_device_set_service_searched_cb() failed.", __FUNCTION__);
            return -1;
        }

        if(bt_device_start_service_search(bt_address) == BT_ERROR_NONE)
        {
            ALOGI("[%s] bt_device_service_searched_cb will be called.", __FUNCTION__);
            g_main_loop_run(gMainLoop);
        }
        else
        {
            ALOGE("[%s] bt_device_start_service_search() failed.", __FUNCTION__);
            return -1;
        }
    }
    else if(bonding_state != BT_ERROR_NONE)
    {
        if(bt_device_set_bond_created_cb(bt_device_bond_created_impl, NULL) != BT_ERROR_NONE)
        {
            ALOGE("[%s] bt_device_set_bond_created_cb() failed.", __FUNCTION__);
            return -1;
        }

        if(bt_device_create_bond(bt_address) == BT_ERROR_NONE)
        {
            ALOGI("[%s] bt_device_bond_created_cb will be called.", __FUNCTION__);
            g_main_loop_run(gMainLoop);
        }
        else
        {
            ALOGE("[%s] bt_device_create_bond() failed.", __FUNCTION__);
            return -1;
        }
    }

	    //  Connecting socket as a client
    if( bonding_state == BT_ERROR_NONE )
    {
        if( bt_socket_set_connection_state_changed_cb(bt_socket_connection_state_changed_impl, NULL) != BT_ERROR_NONE )
        {
            ALOGE("[%s] bt_socket_set_connection_state_changed_cb() failed.", __FUNCTION__);
            return -1;
        }

        if( bt_socket_set_data_received_cb(bt_socket_data_received_impl, NULL) != BT_ERROR_NONE )
        {
            ALOGE("[%s] bt_socket_set_data_received_cb() failed.", __FUNCTION__);
            return -1;
        }

        if( bt_socket_connect_rfcomm(bt_address, uuid) == BT_ERROR_NONE )
        {
            ALOGI("[%s] bt_socket_connection_state_changed_cb will be called.", __FUNCTION__);
            g_main_loop_run(gMainLoop);
        }
        else
        {
            ALOGE("[%s] bt_socket_connect_rfcomm() failed.", __FUNCTION__);
            return -1;
        }

        if( bt_socket_disconnect_rfcomm(gSocketFd) != BT_ERROR_NONE )
        {
            ALOGE("[%s] bt_socket_disconnect_rfcomm() failed.", __FUNCTION__);
            return -1;
        }
    }
    else
    {
        ALOGE("[%s] Bond is not created.", __FUNCTION__);
        return -1;
    }


	return 0;
}

int rkf_finalize_bluetooth(void) {
	bt_deinitialize();
	return 0;
}

int mindstorm_motor1(int type, int power) {
	char data[15] = { 0x0c, 0x00, (char) 0x80, 0x04, 0x02, 0x32, 0x07, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00,};

	data[4] = (char) type;	
	data[5] = (char) power;
	
	rkf_send_data(data, 14);
}

int mindstorm_beep(void) {
	char data[] = {0x06, 0x00, 0x80, 0x03, 0x0B, 0x02, 0xF4, 0x01};
	rkf_send_data(data, 8);
}

int mindstorm_color(int color) {
	char data[7] = { 0x05, 0x00, 0x80, 0x05, 0x02, 0x0D, 0x80 };
	char type;

	switch(color){
		case 0:// All color
			type = 0x0D;
			break;
		case 1:// Red
			type = 0x0E;
			break;
		case 2:// Green
			type = 0x0F;
			break;
		case 3:// Blue
			type = 0x10;
			break;
		case 4:// Light off
			type = 0x11;
			break;
	}

	data[5] = type;
	
	rkf_send_data(data, 7);
}

int mindstorm_sonar_set() {
	char data[7] = { 0x05, 0x00, 0x00, 0x05, 0x03, 0x0b, 0x00 };
	
	rkf_send_data(data, 7);
}

int mindstorm_sonar_read() {
	char data1[10] = { 0x08, 0x00, 0x00, 0x0F, 0x03/*port*/, 0x03, 0x00, 0x02, 0x41, 0x02 };
	rkf_send_data(data1, 10);

	char data2[9] = { 0x07, 0x00, 0x00, 0x0F, 0x03/*port*/, 0x02, 0x01, 0x02, 0x42};
	rkf_send_data(data2, 9);
			
	char data3[5] = { 0x03, 0x00, 0x00, 0x0E, 0x03/*port*/};
	rkf_send_data(data3, 5);
}

static DBusHandlerResult dbus_filter (DBusConnection *connection, DBusMessage *message, void *user_data) {

	DBusError error = {0};
	dbus_error_init(&error);

	if (dbus_message_is_signal(message,"User.Mindstorm.API","Motor")) {
		int motor_type = 0;
		int motor_power = 0;
		ALOGD("Message motor received\n");
			
		dbus_message_get_args(message, &error,
			DBUS_TYPE_INT32, &motor_type,
			DBUS_TYPE_INT32, &motor_power,
			DBUS_TYPE_INVALID);	

		mindstorm_motor1(motor_type, motor_power);
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	if (dbus_message_is_signal(message,"User.Mindstorm.API","Beep")) {
		ALOGD("Message beep received\n");
			
		dbus_message_get_args(message, &error,
			DBUS_TYPE_INVALID);	

		mindstorm_beep();
		return DBUS_HANDLER_RESULT_HANDLED;
	}


	if (dbus_message_is_signal(message,"User.Mindstorm.API","Color")) {
		int color;	
		ALOGD("Message color received\n");
			
		dbus_message_get_args(message, &error,
			DBUS_TYPE_INT32, &color,
			DBUS_TYPE_INVALID);	

		mindstorm_color(color);
		return DBUS_HANDLER_RESULT_HANDLED;
	}


	if (dbus_message_is_signal(message,"User.Mindstorm.API","SonarSet")) {
		ALOGD("Message setting sonar received\n");
			
		dbus_message_get_args(message, &error,
			DBUS_TYPE_INVALID);	

		mindstorm_sonar_set();
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	if (dbus_message_is_signal(message,"User.Mindstorm.API","SonarRead")) {
		ALOGD("Message read sonar received\n");
			
		dbus_message_get_args(message, &error,
			DBUS_TYPE_INVALID);	

		mindstorm_sonar_read();
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	ALOGD("DBUS_HANDLER_RESULT_NOT_YET_HANDLED\n");
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


int dbus_listen_connection(void) {

	DBusConnection *connection;
	DBusError error;

	dbus_error_init(&error);

	connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);

	if (dbus_error_is_set(&error)) {
		ALOGD("Error connecting to the daemon bus: %s",error.message);
		dbus_error_free(&error);
		return -1;
	}

	dbus_bus_add_match (connection, "path='/User/Mindstorm/API',type='signal',interface='User.Mindstorm.API'",NULL);
	dbus_connection_add_filter (connection, dbus_filter, gMainLoop, NULL);

	/* dbus-glib call */
	dbus_connection_setup_with_g_main(connection,NULL);

	return 0;
}

int rkf_send_data(const char *data, int length) {
	int ret = bt_socket_send_data(gSocketFd, data, length);

	if(ret != BT_ERROR_NONE) {
		ALOGD("RemoteKeyFW: unknown error(%d) is occured in rkf_serror_end_data()", ret);
		return -1;
	} else {
		ALOGD("Send Success!");
		return 0;
	}
}

void rkf_main_loop(void) {
	g_main_loop_run(gMainLoop);
}

int gReceiveCount = 0;

// bt_socket_data_received_cb
void rkf_received_data_cb(bt_socket_received_data_s *data, void *user_data) {
	static char buffer[1024];
	char menu_string[]="menu";
	char home_string[]="home";
	char back_string[]="back";

	strncpy(buffer, data->data, 1024);
	buffer[data->data_size] = '\0';
	ALOGD("RemoteKeyFW: received a data!(%d) %s", ++gReceiveCount, buffer);

	// ACTION!
	if(strncmp(buffer, menu_string, strlen(home_string)) == 0) {
		system("/bin/echo 1 > /sys/bus/platform/devices/homekey/coordinates");
	} else if(strncmp(buffer, home_string, strlen(home_string)) == 0) {
		system("/bin/echo 11 > /sys/bus/platform/devices/homekey/coordinates");
	} else if(strncmp(buffer, back_string, strlen(home_string)) == 0) {
		system("/bin/echo 111 > /sys/bus/platform/devices/homekey/coordinates");
	}

	// Sending ack is optional
	//char ack_string[]="ack rkf ";
	//ack_string[strlen(ack_string)-1] = '0' + (gReceiveCount % 10);
	//rkf_send_data(ack_string, strlen(ack_string)+1);
}

// bt_socket_connection_state_changed_cb
void rkf_socket_connection_state_changed_cb(int result, bt_socket_connection_state_e connection_state_event, bt_socket_connection_s *connection, void *user_data) {
	if(result == BT_ERROR_NONE) {
		ALOGD("RemoteKeyFW: connection state changed (BT_ERROR_NONE)");
	} else {
		ALOGD("RemoteKeyFW: connection state changed (not BT_ERROR_NONE)");
	}

	if(connection_state_event == BT_SOCKET_CONNECTED) {
		ALOGD("RemoteKeyFW: connected");
//		if(connection != NULL) {
//			ALOGD("RemoteKeyFW: connected (%d,%s)", connection->local_role, connection->remote_address);
//		}
	} else if(connection_state_event == BT_SOCKET_DISCONNECTED) {
		ALOGD("RemoteKeyFW: disconnected");
//		if(connection != NULL) {
//			ALOGD("RemoteKeyFW: disconnected (%d,%s)", connection->local_role, connection->remote_address);
//		}
		g_main_loop_quit(gMainLoop);
	}
}

void rkf_state_changed_cb(int result, bt_adapter_state_e adapter_state, void *user_data) {
	if(adapter_state == BT_ADAPTER_ENABLED) {
		if(result == BT_ERROR_NONE) {
			ALOGD("RemoteKeyFW: bluetooth was enabled successfully.");
			gBtState = BT_ADAPTER_ENABLED;
		} else {
			ALOGD("RemoteKeyFW: failed to enable BT.: %x", result);
			gBtState = BT_ADAPTER_DISABLED;
		}
	}
	if(gMainLoop) {
		ALOGD("It will terminate gMainLoop.", result);
		g_main_loop_quit(gMainLoop);
	}
}

gboolean timeout_func_cb(gpointer data)
{
	ALOGE("timeout_func_cb");
	if(gMainLoop)
	{
		g_main_loop_quit((GMainLoop*)data);
	}
	return FALSE;
}

int main(int argc, char *argv[])
{
	int error, ret = 0;
	const char default_device_name[] = "Tizen-RK";
	const char *device_name = NULL;
	gMainLoop = g_main_loop_new(NULL, FALSE);
	// Listen connection (dbus)
	error = dbus_listen_connection();

	ALOGD("Sever started\n");
	// Initialize bluetooth
	error = rkf_initialize_bluetooth(device_name);
	if(error != 0) {
		ret = -2;
		goto error_end_without_socket;
	}
	ALOGD("succeed in rkf_initialize_bluetooth()\n");

error_end_without_socket:
	rkf_finalize_bluetooth();
	return ret;
}

//! End of a file
