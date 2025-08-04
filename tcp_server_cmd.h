#ifndef __TCP_SERVER_CMD_H__
#define __TCP_SERVER_CMD_H__

//#include <stdio.h>

typedef unsigned char uint8;
//#define uint8  unsigned char

typedef struct {
	uint8 STX;
	uint8 Len;
	uint8 Type;
} Meter_proto_header;

typedef struct {
	Meter_proto_header Header;
	#if 0
	uint8 STX;
	uint8 Len;
	uint8 Type;
	uint8 Ver;
	#endif
	uint8 Ver;
	uint8 Calib;
	uint8 Serial[4];
	uint8 Q3[2];
	uint8 Qt[2];
	uint8 Qs[2];
	uint8 Q2[2];
	uint8 Q1[2];
	uint8 SetTemp[2];
	uint8 Maker;
	uint8 Chk;
} Meter_Conf_Set;


typedef struct {
	Meter_proto_header Header;
	uint8 Ver;
	uint8 Qn;
	uint8 Flow[2];
	uint8 Value[2];
	uint8 Chk;
}Meter_Flow_Set;

typedef struct {
	Meter_proto_header Header;
	#if 0
	uint8 STX;
	uint8 Len;
	uint8 Type;
	uint8 Ver;
	#endif
	uint8 Ver;
	uint8 Qn;
	uint8 FlowRate[4];
	uint8 Chk;
} Meter_Flow_Rate_Set;

typedef struct {
	Meter_proto_header Header;
	uint8 Ver;
	uint8 Line;
	uint8 S_N[5];
	uint8 IMEI[8];
	uint8 Chk;
} CONNECT_INFO;

typedef struct {
	Meter_proto_header Header;
	uint8 Cliber;
	uint8 Serial[4];
	uint8 Q3[2];
	uint8 Qt[2];
	uint8 Qs[2];
	uint8 Q2[2];
	uint8 Q1[2];
	uint8 SetTemp[2];
	uint8 CurTemp[2];
	uint8 Maker;
	uint8 FW;
	uint8 Result;
	uint8 Chk;
	uint8 Stop;
}METER_CONF_RES;

typedef struct {
	Meter_proto_header Header;
	uint8 Status;
	uint8 Chk;
	uint8 Stop;
}METER_CALIB_S_RES;

typedef struct {
	Meter_proto_header Header;
	uint8 Qn;
	uint8 ServerFlowRate[4];
	uint8 MeterFlowRate[4];
	uint8 Result;
	uint8 Chk;
	uint8 Stop;
}METER_CALIB_R_RES;

typedef struct {
	Meter_proto_header Header;
	uint8 Ver;
	uint8 CL;
	uint8 MDH;
	uint8 Serial[4];
	uint8 Status;
	uint8 DIF;
	uint8 VIF;
	uint8 Value[4];
	uint8 Chk;
	uint8 Stop;
}METER_VALUE_RES;

#endif
