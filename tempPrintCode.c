

#define SERIAL_NUM_LEN 12
#define SERVER_IP_STR_LEN 30 // string 길이이므로 \0 처리 길이까지 포함 시킴.
#define FOTA_IP_STR_LEN 16

typedef struct {
	uint8 serialNum[SERIAL_NUM_LEN + 1];
	uint8 meterType;
	char serviceCode[5]; // using only LG platfom. others all '0'
	char serverIp[SERVER_IP_STR_LEN];
	uint16 serverPort;
    uint8 serverFormatSel;
	char fotaIp[FOTA_IP_STR_LEN]; // using for FOTA. It used when not use LG platform.
	uint16 fotaPort; // using for FOTA. It used when not use LG platform.
	uint8 fotaInterval; // using for FOTA. It used when not use LG platform.
	uint8 imei[8];
	uint8 imsi[8];
	BOOL isModemInit;
	uint8 dataSkipMode;
	uint8 sleepMode;
	uint8 riCtrlMode; // 0 : no ctrl, 1 : ctrl
	uint8 riCtrlChgCount;
	uint8 riCtrlValue;
	uint8 isShortInterval;
	uint8 intervalBaseTime;
	uint8 meterInterval;
	uint8 reportInterval;
	uint8 reportRange;
	uint8 reportMin;
	uint8 reportSec;
	uint16 resetCount;
	uint8 debugPrint;
	uint8 termModel;
	uint8 bslModel;
	uint8 reserved;
	uint8 meterProtoVer;
	uint8 checksum;
} Config_t;