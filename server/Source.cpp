#include <signal.h>
#include <iostream>
#include <cstring>
#include<string>
#include<thread>
#include<math.h>
# include "open62541.h"
#include"Fwlib32.h"
using namespace std;
int Alm = 0;
#pragma region UADefine
UA_ServerConfig *config = UA_ServerConfig_new_default();
UA_Server *server = UA_Server_new(config);
UA_Boolean running = true;
UA_Logger logger = UA_Log_Stdout;
static void stopHandler(int sign) {
	UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "received ctrl-c");
	running = false;
}
#pragma endregion

#pragma region CNC_Function

unsigned short CNC_handle;
string alarm2Info[] =
{
	"None",
	"P/S 100 ALARM", "P/S 000 ALARM",
	"P/S 101 ALARM", "P/S ALARM (1-255)",
	"OT ALARM", "OH ALARM",
	"SERVO ALARM", "SYSTEM ALARM",
	"APC ALARM", "SPINDLE ALARM",
	"P/S ALARM (5000-)"
};


struct CNC_POSITION {
	char name;
	float data;
};
CNC_POSITION CNC_position[MAX_AXIS];
bool gethandle(void) {
	// handle
	short ret;
	ret = cnc_allclibhndl3("192.168.1.201", 8193, 10, &CNC_handle);

	if (!ret)
	{

		//success
		return true;
	}
	else
	{
		return false;
	}
}
long read_rotationalSpeed() {
	ODBACT buf;
	long rotateSpeed;
	cnc_acts(CNC_handle, &buf);
	rotateSpeed = buf.data;
	return rotateSpeed;
}
string read_alarm()
{
	long alm;
	cnc_alarm2(CNC_handle, &alm);
	if (alm == 0)
	{
		Alm = 0;
		return alarm2Info[0];
	}
	else
	{
		Alm = 1;
		unsigned short idx;
		for (idx = 0; idx < 11; idx++) {
			if (alm & 0x0001)
				return alarm2Info[idx];
			alm >>= 1;
		}
	}
}
string read_ProgramName()
{
	ODBEXEPRG buf;

	long rotateSpeed;
	cnc_exeprgname(CNC_handle, &buf);
	string str(buf.name);
	return str;
}
long read_feedrate() {
	ODBACT buf;
	long feedrate;
	cnc_actf(CNC_handle, &buf);
	feedrate = buf.data;
	return feedrate;
}
void read_position(CNC_POSITION cnc_position[MAX_AXIS]) {
	ODBPOS pos[MAX_AXIS];
	short num = MAX_AXIS;

	int ret_position = cnc_rdposition(CNC_handle, 0, &num, pos);		// 0 : absolute position, 1 : machine position, 2 : relative position, 3 : distance to go, -1 : all type 

	if (!ret_position) {
		int i;
		for (i = 0; i < num; i++) {
			cnc_position[i].name = pos[i].abs.name;
			cnc_position[i].data = pos[i].abs.data;
			cnc_position[i].data /= 10000;
		}
	}
}


int readmacro(short number) {
	ODBM macro;
	char strbuf[11];
	//			short ret;
	int macro_value;

	short ret_macro = cnc_rdmacro(CNC_handle, number, 10, &macro);
	if (!ret_macro)
	{
		sprintf(&strbuf[1], "%09ld", macro.mcr_val);
		if (strbuf[1] == '0') strbuf[1] = ' ';
		strncpy(&strbuf[0], &strbuf[1], 9 - macro.dec_val);
		strbuf[9 - macro.dec_val] = '.';
		//				printf( "%s\n", strbuf ) ;
		//				String^ str = gcnew String(strbuf);
		macro_value = atoi(strbuf);
		return macro_value;
	}
	else
	{
		return 3333;	//-----
	}
	//			return ret;
}
#pragma endregion


#pragma region UpDateValue
void upDateValue2OPC(string name, int value)
{
	UA_Int32 Integer = value;
	UA_NodeId NodeId = UA_NODEID_STRING_ALLOC(1, name.c_str());
	UA_Variant myVar;
	UA_Variant_init(&myVar);
	UA_Variant_setScalar(&myVar, &Integer, &UA_TYPES[UA_TYPES_INT32]);
	UA_Server_writeValue(server, NodeId, myVar);
}
void upDateValue2OPC(string name, double value)
{
	UA_Double Integer = value;
	UA_NodeId NodeId = UA_NODEID_STRING_ALLOC(1, name.c_str());
	UA_Variant myVar;
	UA_Variant_init(&myVar);
	UA_Variant_setScalar(&myVar, &Integer, &UA_TYPES[UA_TYPES_DOUBLE]);
	UA_Server_writeValue(server, NodeId, myVar);
}
void upDateValue2OPC(string name, string str)
{
	UA_String uastr = UA_String_fromChars(str.c_str());

	UA_NodeId NodeId = UA_NODEID_STRING_ALLOC(1, name.c_str());
	UA_Variant myVar;
	UA_Variant_init(&myVar);
	UA_Variant_setScalar(&myVar, &uastr, &UA_TYPES[UA_TYPES_STRING]);
	UA_Server_writeValue(server, NodeId, myVar);
}
#pragma endregion


#pragma region AddValue
void addValue2OPC(string name, double value)
{
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	UA_Double Integer = value;
	UA_Variant_setScalarCopy(&attr.value, &Integer, &UA_TYPES[UA_TYPES_DOUBLE]);
	attr.description = UA_LOCALIZEDTEXT_ALLOC("en-US", name.c_str());
	attr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", name.c_str());
	UA_NodeId NodeId = UA_NODEID_STRING_ALLOC(1, name.c_str());
	UA_QualifiedName Name = UA_QUALIFIEDNAME_ALLOC(1, name.c_str());
	UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
	UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
	UA_Server_addVariableNode(server, NodeId, parentNodeId,
		parentReferenceNodeId, Name,
		UA_NODEID_NULL, attr, NULL, NULL);
	UA_VariableAttributes_deleteMembers(&attr);
	UA_NodeId_deleteMembers(&NodeId);
	UA_QualifiedName_deleteMembers(&Name);
}
void addValue2OPC(string name, int value)
{
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	UA_Int32 Integer = value;
	UA_Variant_setScalarCopy(&attr.value, &Integer, &UA_TYPES[UA_TYPES_INT32]);
	attr.description = UA_LOCALIZEDTEXT_ALLOC("en-US", name.c_str());
	attr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", name.c_str());
	UA_NodeId NodeId = UA_NODEID_STRING_ALLOC(1, name.c_str());
	UA_QualifiedName Name = UA_QUALIFIEDNAME_ALLOC(1, name.c_str());
	UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
	UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
	UA_Server_addVariableNode(server, NodeId, parentNodeId,
		parentReferenceNodeId, Name,
		UA_NODEID_NULL, attr, NULL, NULL);
	UA_VariableAttributes_deleteMembers(&attr);
	UA_NodeId_deleteMembers(&NodeId);
	UA_QualifiedName_deleteMembers(&Name);
}
void addValue2OPC(string name, string str)
{
	UA_VariableAttributes attr = UA_VariableAttributes_default;

	UA_String uastr = UA_String_fromChars(str.c_str());
	UA_Variant_setScalarCopy(&attr.value, &uastr, &UA_TYPES[UA_TYPES_STRING]);
	attr.description = UA_LOCALIZEDTEXT_ALLOC("en-US", name.c_str());
	attr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", name.c_str());
	UA_NodeId NodeId = UA_NODEID_STRING_ALLOC(1, name.c_str());
	UA_QualifiedName Name = UA_QUALIFIEDNAME_ALLOC(1, name.c_str());
	UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
	UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
	UA_Server_addVariableNode(server, NodeId, parentNodeId,
		parentReferenceNodeId, Name,
		UA_NODEID_NULL, attr, NULL, NULL);
	UA_VariableAttributes_deleteMembers(&attr);
	UA_NodeId_deleteMembers(&NodeId);
	UA_QualifiedName_deleteMembers(&Name);

}

#pragma endregion
int LaserTransfer2Watte(int n)
{
	return 5.911*pow(10, -6)*pow(n, 2) + 0.2251*n + -0.4611;
}

void ReadDataFromCNC()
{
	assert(gethandle());

	while (true)
	{
		//upDateValue2OPC(server, "testValue", testUpdate);
		for (uint16_t i = 0; i < MAX_AXIS; i++)
		{
			CNC_position[i].name = 0;
			CNC_position[i].data = 0;
		}

		read_position(CNC_position);

		upDateValue2OPC("AMH_PosX", (double)CNC_position[0].data);
		upDateValue2OPC("AMH_PosY", (double)CNC_position[1].data);
		upDateValue2OPC("AMH_PosZ", (double)CNC_position[2].data);
		upDateValue2OPC("AMH_PosA", (double)CNC_position[3].data);
		upDateValue2OPC("AMH_PosC", (double)CNC_position[4].data);

		upDateValue2OPC("AMH_FeedRate", read_feedrate());

		upDateValue2OPC("AMH_RotateSpeed", read_rotationalSpeed());
		upDateValue2OPC("AMH_ProgramName", read_ProgramName());
		upDateValue2OPC("AMH_Alarm", read_alarm());
		upDateValue2OPC("AMH_AlarmValue", Alm);

		upDateValue2OPC("AMH_LaserPower", LaserTransfer2Watte( readmacro(1133)));
		upDateValue2OPC("AMH_PowderA", (int)(readmacro(1134)/26.0f));
		upDateValue2OPC("AMH_PowderB", (int)(readmacro(1135)/26.0f));
		if(!Alm)
			upDateValue2OPC("AMH_Switch", readmacro(555));
		else
			upDateValue2OPC("AMH_Switch",0);
	}
}

int main() {


	signal(SIGINT, stopHandler);
	signal(SIGTERM, stopHandler);

	addValue2OPC("AMH_PosX", 0.f);
	addValue2OPC("AMH_PosY", 0.f);
	addValue2OPC("AMH_PosZ", 0.f);
	addValue2OPC("AMH_PosA", 0.f);
	addValue2OPC("AMH_PosC", 0.f);
	addValue2OPC("AMH_FeedRate", 0.f);
	addValue2OPC("AMH_RotateSpeed", 0.f);
	addValue2OPC("AMH_ProgramName", "Null");
	addValue2OPC("AMH_Alarm", "Null");
	addValue2OPC("AMH_LaserPower",0);
	addValue2OPC("AMH_PowderA", 0);
	addValue2OPC("AMH_PowderB", 0);
	addValue2OPC("AMH_Switch", 0);
	addValue2OPC("AMH_AlarmValue", 0);
	thread t(ReadDataFromCNC);


	///* allocations on the heap need to be freed */


	UA_StatusCode retval = UA_Server_run(server, &running);
	UA_Server_delete(server);
	UA_ServerConfig_delete(config);
	return retval;

}