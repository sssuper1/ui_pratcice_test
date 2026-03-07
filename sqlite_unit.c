#include "sqlite3.h"
#include "sqlite_unit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Lock.h"
#include "mgmt_types.h"
#include <sys/stat.h>
#include <fcntl.h>
#include "wg_config.h"
#include "mgmt_transmit.h"
#include "socketUDP.h"
#include <unistd.h>
#include "ui_get.h"

sqlite3 *g_psqlitedb = NULL;
uint8_t  rate_auto=0;   //0:fix  1:auto
pthread_mutex_t sqlite3_mutex1;
bcMeshInfo meshinfo;
struct sockaddr_in S_GROUND_BCAST;
#define BCAST_SEND_PORT  7901   //组播端口
int SOCKET_BCAST_SEND; 


#define TEST_DB_SRC "/www/cgi-bin/test.db"
#define TEST_DB_DST "/www/cgi/test.db"
#define TEST_DB_TMP "/www/cgi/test.db.tmp"
#define TEST_DB_BAK "/www/cgi/test.db.bak"


// 通过 read/write + fsync 拷贝文件，避免产生部分写入的副本
static int copy_file_contents(const char *src, const char *dst)
{
	int in_fd = open(src, O_RDONLY);
	if (in_fd < 0)
		return -1;

	int out_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (out_fd < 0) {
		close(in_fd);
		return -1;
	}

	char buffer[4096];
	ssize_t rd;
	while ((rd = read(in_fd, buffer, sizeof(buffer))) > 0) {
		char *ptr = buffer;
		ssize_t remaining = rd;
		while (remaining > 0) {
			ssize_t wr = write(out_fd, ptr, remaining);
			if (wr <= 0) {
				close(in_fd);
				close(out_fd);
				return -1;
			}
			ptr += wr;
			remaining -= wr;
		}
	}

	if (rd < 0) {
		close(in_fd);
		close(out_fd);
		return -1;
	}

	if (fsync(out_fd) != 0) {
		close(in_fd);
		close(out_fd);
		return -1;
	}

	close(in_fd);
	close(out_fd);

	return 0;
}
// SQLite quick_check 回调：当返回值为 "ok" 时将标志置为 true
static int sqlite_quick_check_cb(void *data, int argc, char **argv, char **azColName)
{
	bool *ok = (bool *)data;
	if (argc > 0 && argv[0] && strcmp(argv[0], "ok") == 0) {
		*ok = true;
	}
	return 0;
}

// 在当前打开的 DB 句柄上执行 PRAGMA quick_check
static bool sqlite_db_is_valid(void)
{
	bool ok = false;
	char *errmsg = NULL;

	if (!g_psqlitedb)
		return false;

	if (SQLITE_OK != sqlite3_exec(g_psqlitedb, "PRAGMA quick_check;", sqlite_quick_check_cb, &ok, &errmsg)) {
		if (errmsg) {
			sqlite3_free(errmsg);
		}
		return false;
	}

	return ok;
}


bool persist_test_db(void)
{
	struct stat st;
    //检查文件是否存在且非空
	if (stat(TEST_DB_SRC, &st) != 0 || st.st_size <= 0) {
		printf("[sqlite_unit] skip persist test.db: empty or missing source\n");
		return false;
	}
    //检查当前 test.db是否损坏
	if (!sqlite_db_is_valid()) {
		printf("[sqlite_unit] skip persist test.db: integrity check failed\n");
		return false;
	}
    //执行文件拷贝
	if (copy_file_contents(TEST_DB_SRC, TEST_DB_TMP) != 0) {
		printf("[sqlite_unit] failed to create temporary test.db copy: %s\n", strerror(errno));
		unlink(TEST_DB_TMP);
		return false;
	}
    //同步
	sync();
    //备份旧文件，替换新文件
	if (rename(TEST_DB_DST, TEST_DB_BAK) != 0 && errno != ENOENT) {
		printf("[sqlite_unit] warning: could not rotate previous test.db: %s\n", strerror(errno));
	}

	if (rename(TEST_DB_TMP, TEST_DB_DST) != 0) {
		printf("[sqlite_unit] failed to promote new test.db: %s\n", strerror(errno));
		unlink(TEST_DB_TMP);
		if (rename(TEST_DB_BAK, TEST_DB_DST) != 0 && errno != ENOENT) {
			printf("[sqlite_unit] warning: could not restore previous test.db from backup\n");
		}
		return false;
	}
    //再次同步，确保新文件完全写入磁盘
	sync();
	return true;
}


static int sqlite_set_userinfo_callback(void *NotUsed, int argc, char **argv, char **azColName) {
	char *zErrMsg = 0;
	char updateSql[100];
	int rc;
	int counttmp = 0;
	uint8_t cmd[200];
	bool isset = FALSE;
	if(0 == strcmp(argv[2],"1"))
	{
//		printf("%s = %s  %s %s\n", azColName[2], argv[2],azColName[0],argv[0]);
		if(0 == strcmp(argv[0],"m_ip"))
		{
			sscanf(argv[1], "%d.%d.%d.%d", &SELFIP_s[0],&SELFIP_s[1],&SELFIP_s[2],&SELFIP_s[3]);
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,
					"ifconfig eth0 %d.%d.%d.%d",
					SELFIP_s[0],SELFIP_s[1],SELFIP_s[2],SELFIP_s[3]);
			system(cmd);
				
			printf("set ------- eth0 ip address = %d.%d.%d.%d\n", SELFIP_s[0],SELFIP_s[1],SELFIP_s[2],SELFIP_s[3]);
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,
					"sed -i \"s/ip_addr .*/ip_addr %s/g\" /mnt/node_xwg",
					argv[1]);
			system(cmd);
			system("sync");
			isset = TRUE;	
		}
		if(0 == strcmp(argv[0],"m_dhcpStart"))
		{
		}
		if(0 == strcmp(argv[0],"m_dhcpGateway"))
		{
		}
		if(0 == strcmp(argv[0],"m_dhcpDns"))
		{
		}

		snprintf(updateSql, sizeof(updateSql), "UPDATE userInfo SET state = '0' WHERE name = '%s';" \
					,argv[0]);
		
		sqlite3_busy_handler(g_psqlitedb,busyHandle,NULL);
		Lock(&sqlite3_mutex1,0);
	    while (SQLITE_OK != sqlite3_exec(g_psqlitedb, updateSql, NULL, 0, &zErrMsg)) {
	        //fprintf(stderr, "callback error1: %s\n", zErrMsg);
	        counttmp ++;
	        if(counttmp > 10)
	        	break;
	        sqlite3_busy_handler(g_psqlitedb,busyHandle,NULL);
	    }
	    Unlock(&sqlite3_mutex1);
		
		if(isset)
		{
			isset=FALSE;
			sleep(1);
			if (!persist_test_db()) {
				printf("[sqlite_unit] persist test.db failed after userinfo change\n");
			}
		}
	}
    return 0;
}

static int sqlite_set_meshinfo_callback(void *NotUsed, int argc, char **argv, char **azColName) {
    
    char *zErrMsg = 0;
	bool isset = FALSE;
	char updateSql[100];
	int countTmp = 0;
	int m_chanbw;
	int m_rate;
	int m_freq;
	int m_power;
	uint8_t bcastmode;
	int workmode;//0:开启 1：关闭
	int rc;
	static int SOCKET_BCAST; 
	//INT8 bcast_buf[BUFLEN];
	static struct sockaddr_in S_GROUND_BCAST;
    int temp;

    INT8 buffer[sizeof(Smgmt_header) + sizeof(Smgmt_set_param)];
	INT32 buflen = sizeof(Smgmt_header) + sizeof(Smgmt_set_param);
	Smgmt_header* mhead = (Smgmt_header*)buffer;
	Smgmt_set_param* mparam = (Smgmt_set_param*)mhead->mgmt_data;
	uint8_t cmd[200]; 
	bzero(buffer, buflen);
	memset(cmd,0,sizeof(cmd));
	mhead->mgmt_head = htons(HEAD);
	mhead->mgmt_len = sizeof(Smgmt_set_param);
	mhead->mgmt_type = 0;

    stInData stsysteminfodata;// 用来暂存从数据库读出的数据，准备下发到内核
	memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));

    if(0 == strcmp(argv[4],"1"))
    {
        if(0 == strcmp(argv[0],"m_txpower"))
        {
            sscanf(argv[1],"%d",&m_power);
            mparam->mgmt_mac_txpower = htons((uint16_t)m_power);
            //网管上下发的功率，到每个通道时需要查表才能精确的补偿到每个通道都是下发的功率
            meshinfo.m_txpower = mparam->mgmt_mac_txpower;
            uint16_t txpower_channels[POWER_CHANNEL_NUM];
            txpower_lookup_channels(m_power, txpower_channels);
            for (int i = 0; i < POWER_CHANNEL_NUM; ++i) {
				meshinfo.m_txpower_ch[i] = htons(txpower_channels[i]);
			}
			//isset = TRUE;					
			meshinfo.txpower_isset=1;
/*  更新宽带参数 */
			meshinfo.sys_power=m_power;
//			printf("get txpower:%d  \r\n",m_power-39);
			 memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
			 sprintf(stsysteminfodata.name,"%s","m_txpower");
			 sprintf(stsysteminfodata.value,"%d",39-m_power);
			 stsysteminfodata.state[0] = '1';
			 updateData_systeminfo(stsysteminfodata);
        }
        else if(0 == strcmp(argv[0],"rf_freq"))
		{
            sscanf(argv[1], "%d", &(mparam->mgmt_mac_freq));	
			 //g_radio_param.g_rf_freq=mparam->mgmt_mac_freq;
            mparam->mgmt_mac_freq = htonl(mparam->mgmt_mac_freq);		
			meshinfo.rf_freq=mparam->mgmt_mac_freq;		
			//isset = TRUE;
			meshinfo.freq_isset=1;

			sscanf(argv[1], "%d", &(m_freq));
			meshinfo.sys_freq=m_freq;
//			printf("get freq:%d  \r\n",m_freq);
			memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
			sprintf(stsysteminfodata.name,"%s","rf_freq");
			sprintf(stsysteminfodata.value,"%d",m_freq);
			stsysteminfodata.state[0] = '1';
			updateData_systeminfo(stsysteminfodata);
        }
        else if(0 == strcmp(argv[0],"m_chanbw"))
		{
			//mhead->mgmt_type |= MGMT_SET_BANDWIDTH;
			sscanf(argv[1], "%d", &temp);
            mparam->mgmt_mac_bw = (uint8_t)temp;
             //g_radio_param.g_chanbw=mparam->mgmt_mac_bw;
			//g_radio_param.g_chanbw=mparam->mgmt_mac_bw;
			meshinfo.m_chanbw=mparam->mgmt_mac_bw;
			//isset = TRUE;
			meshinfo.chanbw_isset=1;
#if 0
			if(mparam->mgmt_mac_bw==0)
			{
			m_chanbw=20;
			}
			else if(mparam->mgmt_mac_bw==1)
			{
			m_chanbw=10;
			}
			else if(mparam->mgmt_mac_bw==2)
			{
			m_chanbw=5;
			}
			else if(mparam->mgmt_mac_bw==3)
            {
            m_chanbw=;  
            }
#endif // ssq


			meshinfo.sys_bw=mparam->mgmt_mac_bw;
//			printf("get bw=%d  \r\n",m_chanbw);
			memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
			sprintf(stsysteminfodata.name,"%s","m_chanbw");
			sprintf(stsysteminfodata.value,"%d",mparam->mgmt_mac_bw);
			stsysteminfodata.state[0] = '1';
			updateData_systeminfo(stsysteminfodata);
		}
        else if(0 == strcmp(argv[0],"m_rate"))
		{
			//mhead->mgmt_type |= MGMT_SET_UNICAST_MCS;
			sscanf(argv[1], "%d", &temp);
            mparam->mgmt_virt_unicast_mcs = (uint8_t)temp;
			//g_radio_param.g_rate=mparam->mgmt_virt_unicast_mcs;
			meshinfo.m_rate=mparam->mgmt_virt_unicast_mcs;
			//isset = TRUE;
			
			meshinfo.rate_isset=1;
			
			sscanf(argv[1], "%d", &(m_rate));
			meshinfo.sys_rate=m_rate;
			if(m_rate<0)
			{
				rate_auto=1;// 开启自动速率标志
				meshinfo.rate_isset=0;
			}
			
//			printf("get rate=%d  \r\n",mparam->mgmt_virt_unicast_mcs);
			memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
			sprintf(stsysteminfodata.name,"%s","m_rate");
			sprintf(stsysteminfodata.value,"%d",mparam->mgmt_virt_unicast_mcs);
			stsysteminfodata.state[0] = '1';
			updateData_systeminfo(stsysteminfodata);
		}
        else if(0 == strcmp(argv[0],"m_distance"))
		{
		}
		else if(0 == strcmp(argv[0],"m_ssid"))
		{
		}
		else if(0 == strcmp(argv[0],"m_bcastmode"))
		{
			sscanf(argv[1], "%d", &temp);
            meshinfo.m_bcastmode = (uint8_t)temp;
			printf("bcast mode=%d\r\n",meshinfo.m_bcastmode);		
		}
        else if(0 == strcmp(argv[0],"workmode"))//
        {
            sscanf(argv[1], "%d", &temp);
            mparam->mgmt_net_work_mode.NET_work_mode = (uint8_t)temp;
            meshinfo.workmode = mparam->mgmt_net_work_mode.NET_work_mode;
            meshinfo.workmode_isset=1;
        }
        else if(0 == strcmp(argv[0],"m_route"))
        {
            sscanf(argv[1],"%d",&temp);
            meshinfo.m_route=(uint8_t)temp;
            meshinfo.route_isset=1;
        }
        else if(0 == strcmp(argv[0],"m_slot_len"))
        {
            sscanf(argv[1],"%d",&temp);
            meshinfo.m_slot_len=(uint8_t)temp;
            meshinfo.slot_isset=1;
        }
        else if(0 == strcmp(argv[0],"m_trans_mode"))
        {
            sscanf(argv[1],"%d",&(meshinfo.m_trans_mode));
            meshinfo.m_trans_mode=htons(meshinfo.m_trans_mode);
            meshinfo.trans_mode_isset=1;
        }
        else if(0 == strcmp(argv[0],"m_select_freq1"))
		{
			//mhead->mgmt_type |= MGMT_SET_UNICAST_MCS;
			sscanf(argv[1], "%d", &temp);
            meshinfo.m_select_freq_1 = temp;
			//g_radio_param.g_select_freq_1=meshinfo.m_select_freq_1;
			meshinfo.select_freq_isset=1;
		}
        else if(0 == strcmp(argv[0],"m_select_freq2"))
		{
			sscanf(argv[1], "%d", &temp);
            meshinfo.m_select_freq_2 = temp;
			meshinfo.select_freq_isset=1;
		}
        else if(0 == strcmp(argv[0],"m_select_freq3"))
		{
			sscanf(argv[1], "%d", &temp);
            meshinfo.m_select_freq_3 = temp;
			meshinfo.select_freq_isset=1;
		}
        else if(0 == strcmp(argv[0],"m_select_freq4"))
		{
			sscanf(argv[1], "%d", &temp);
            meshinfo.m_select_freq_4 = temp;
			meshinfo.select_freq_isset=1;
		}
        else if(0 == strcmp(argv[0],"power_level"))
		{
			int level = 0;
			sscanf(argv[1], "%d", &level);
			meshinfo.power_level = (uint8_t)level;
			meshinfo.sys_power_level = level;
			meshinfo.power_level_isset = 1;
		}
		else if(0 == strcmp(argv[0],"power_attenuation"))
		{
			int attenuation = 0;
			sscanf(argv[1], "%d", &attenuation);
			meshinfo.power_attenuation = (uint8_t)attenuation;
			meshinfo.sys_power_attenuation = attenuation;
			meshinfo.power_attenuation_isset = 1;
		}
		else if(0 == strcmp(argv[0],"rx_channel_mode"))
		{
			int rx_mode = 0;
			sscanf(argv[1], "%d", &rx_mode);
			if (rx_mode < 0) {
				rx_mode = 0;
			} else if (rx_mode > 4) {
				rx_mode = 4;
			}
			meshinfo.rx_channel_mode = (uint8_t)rx_mode;
			meshinfo.sys_rx_channel_mode = rx_mode;
			meshinfo.rx_channel_mode_isset = 1;
		}
        /*  meshInfo表中各参数的state值置0  */
		snprintf(updateSql, sizeof(updateSql), "UPDATE meshInfo SET state = '0' WHERE name = '%s' AND state = '0';" \
					,argv[0]);
        sqlite3_busy_handler(g_psqlitedb,busyHandle,NULL); // 设置忙处理函数，遇忙等待
        Lock(&sqlite3_mutex1,0);

        while (SQLITE_OK != sqlite3_exec(g_psqlitedb, updateSql, NULL, 0, &zErrMsg)) {
			fprintf(stderr, "SQL error1: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
			countTmp ++;
			if(countTmp > 5)
				break;
			sqlite3_busy_handler(g_psqlitedb,busyHandle,NULL);
		}
		Unlock(&sqlite3_mutex1);
    }
    return 0;

}


int sqlite_set_param(void){
    char *zErrMsg = 0;
    int rc;
	int opt=1;
    bool isset=FALSE;
	int ret;
	static uint32_t s_fix_freq=0;   //   定频中心频率
	s_fix_freq=FREQ_INIT;
    double slot_info[]={0.5,1,1.25,2};     //时隙长度

    char bcast_buf[1000];
	uint8_t cmd[200];
	INT8 buffer[sizeof(Smgmt_header) + sizeof(Smgmt_set_param)];
	INT32 buflen = sizeof(Smgmt_header) + sizeof(Smgmt_set_param);
	Smgmt_header* mhead = (Smgmt_header*)buffer;
	Smgmt_set_param* mparam = (Smgmt_set_param*)mhead->mgmt_data;

    bzero(buffer, buflen);
	memset(cmd,0,sizeof(cmd));
	mhead->mgmt_head = htons(HEAD);
	mhead->mgmt_len = sizeof(Smgmt_set_param);
	mhead->mgmt_type = 0;


    stInData stsysteminfodata;

    memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
	memset((char*)&meshinfo,0,sizeof(meshinfo));

    meshinfo.m_bcastmode=1;//初始化为非组播模式

    S_GROUND_BCAST.sin_family=AF_INET;
	S_GROUND_BCAST.sin_addr.s_addr = inet_addr("192.168.2.255"); //设置成广播
	S_GROUND_BCAST.sin_port = htons(BCAST_SEND_PORT);
	SOCKET_BCAST_SEND=socket (AF_INET,SOCK_DGRAM, 0 );

    if(SOCKET_BCAST_SEND <= 0)
	{
		printf("create bcast socket failed\r\n");
		//return -1;
	}

    setsockopt(SOCKET_BCAST_SEND,SOL_SOCKET,SO_BROADCAST,&opt,sizeof(opt));

    while(1){

        sqlite3_busy_handler(g_psqlitedb,busyHandle,NULL);

    	sqlite3_exec(g_psqlitedb,"SELECT * FROM userInfo;",sqlite_set_userinfo_callback, 0, &zErrMsg);
    	sqlite3_exec(g_psqlitedb,"SELECT * FROM meshInfo;",sqlite_set_meshinfo_callback, 0, &zErrMsg);

        if(meshinfo.m_bcastmode==1){ 
        
            bzero(buffer, buflen);
            mhead->mgmt_head = htons(HEAD);
            mhead->mgmt_len = sizeof(Smgmt_set_param);
            mhead->mgmt_type = 0;

            /* 非组播模式*/
            if(meshinfo.txpower_isset)
            {
                meshinfo.txpower_isset=0;
                isset=TRUE;
                mhead->mgmt_type |= MGMT_SET_POWER;
                mparam->mgmt_mac_txpower=meshinfo.m_txpower;
				memcpy(mparam->mgmt_mac_txpower_ch, meshinfo.m_txpower_ch, sizeof(meshinfo.m_txpower_ch));
                
            }
            if(meshinfo.power_level_isset)
            {
                meshinfo.power_level_isset=0;
                isset=TRUE;
                mhead->mgmt_keep |= MGMT_SET_POWER_LEVEL;
                mparam->mgmt_mac_power_level=meshinfo.power_level;
            }
            if(meshinfo.power_attenuation_isset)
			{
				meshinfo.power_attenuation_isset=0;
				isset=TRUE;
				mhead->mgmt_keep |= MGMT_SET_POWER_ATTENUATION;
				mparam->mgmt_mac_power_attenuation=meshinfo.power_attenuation;
			}
            if(meshinfo.rx_channel_mode_isset)
			{
				meshinfo.rx_channel_mode_isset=0;
				isset=TRUE;
				mhead->mgmt_keep |= MGMT_SET_RX_CHANNEL_MODE;
				mparam->mgmt_rx_channel_mode = meshinfo.rx_channel_mode;
			}
            if(meshinfo.freq_isset)
            {
                meshinfo.freq_isset=0;
                if(meshinfo.workmode !=4)
                {
                    isset=TRUE;
                    mhead->mgmt_type |=MGMT_SET_FREQUENCY;
                    mparam->mgmt_mac_freq=meshinfo.rf_freq;
                    s_fix_freq=mparam->mgmt_mac_freq;  
                }
            }
            if(meshinfo.chanbw_isset)
            {
                meshinfo.chanbw_isset=0;
                isset=TRUE;
                mhead->mgmt_type |= MGMT_SET_BANDWIDTH;
                mparam->mgmt_mac_bw=meshinfo.m_chanbw;
            }
            if(meshinfo.rate_isset)
            {
                meshinfo.rate_isset=0;
                isset=TRUE;
                mhead->mgmt_type |= MGMT_SET_UNICAST_MCS;
                mparam->mgmt_virt_unicast_mcs=meshinfo.m_rate;
            }
            if(meshinfo.workmode_isset)
            {
                meshinfo.workmode_isset=0;
                isset=TRUE;
                mhead->mgmt_type |= MGMT_SET_WORKMODE;
                mparam->mgmt_net_work_mode.NET_work_mode=meshinfo.workmode;
                printf("set workmode=%d \r\n",mparam->mgmt_net_work_mode.NET_work_mode);
                if(meshinfo.workmode==1)
                {
                    /* 定频 */
                    mhead->mgmt_type |= MGMT_SET_FREQUENCY;
                    mparam->mgmt_mac_freq=s_fix_freq;

                    sprintf(cmd,
                            "sed -i \"s/workmode .*/workmode %d/g\" /etc/node_xwg",1);
                    system(cmd);
                }
                if(meshinfo.workmode==4)
                {
                    /* 自适应选频 */
                    if(meshinfo.select_freq_isset ==1)
                    {
                        meshinfo.select_freq_isset=0;
                        printf("set select freq,freq1:%d,freq2:%d,freq3:%d,freq4:%d \r\n",
							meshinfo.m_select_freq_1,meshinfo.m_select_freq_2,meshinfo.m_select_freq_3,meshinfo.m_select_freq_4);
						mparam->mgmt_net_work_mode.fh_len=4;
						mparam->mgmt_net_work_mode.hop_freq_tb[0]=meshinfo.m_select_freq_1;
						mparam->mgmt_net_work_mode.hop_freq_tb[1]=meshinfo.m_select_freq_2;
						mparam->mgmt_net_work_mode.hop_freq_tb[2]=meshinfo.m_select_freq_3;
						mparam->mgmt_net_work_mode.hop_freq_tb[3]=meshinfo.m_select_freq_4;
                    }
                    else
                    {
                        mparam->mgmt_net_work_mode.fh_len=4;
						mparam->mgmt_net_work_mode.hop_freq_tb[0]=s_fix_freq;
						mparam->mgmt_net_work_mode.hop_freq_tb[1]=s_fix_freq;
						mparam->mgmt_net_work_mode.hop_freq_tb[2]=s_fix_freq;
						mparam->mgmt_net_work_mode.hop_freq_tb[3]=s_fix_freq;
                    }
                    sprintf(cmd, "sed -i \"s/workmode .*/workmode %d/; \
						s/select_freq1 .*/select_freq1 %d/; \
						s/select_freq2 .*/select_freq2 %d/; \
						s/select_freq3 .*/select_freq3 %d/; \
						s/select_freq4 .*/select_freq4 %d/\" /etc/node_xwg", 
					4, mparam->mgmt_net_work_mode.hop_freq_tb[0], mparam->mgmt_net_work_mode.hop_freq_tb[1], 
					mparam->mgmt_net_work_mode.hop_freq_tb[2], mparam->mgmt_net_work_mode.hop_freq_tb[3]);
			        system(cmd);
                }

            }
            if(meshinfo.route_isset)
            {
                meshinfo.route_isset=0;
                switch (meshinfo.m_route)
                {
                case KD_ROUTING_OLSR:
                    printf("set route olsr \r\n");
					ret = system("/home/root/cs_olsr.sh");
					if(ret == -1) printf("change olsr failed\r\n");
                    break;
                case KD_ROUTING_AODV:
						// aodv
					printf("set route aodv \r\n");
					ret = system("/home/root/cs_aodv.sh");
					if(ret == -1) printf("change aodv failed\r\n");
						
					break;
						
				case KD_ROUTING_CROSS_LAYER:  //batman
					printf("set route batman \r\n");
					ret = system("/home/root/cs_batman.sh");
					if(ret == -1) printf("change batman failed\r\n");

					break;

                default:
                    break;
                }
                sprintf(cmd,
					"sed -i \"s/router .*/router %d/g\" /etc/node_xwg",
				meshinfo.m_route);		
				system(cmd);
            }
            if(meshinfo.slot_isset)
			{	
				meshinfo.slot_isset=0;
				isset=TRUE;
				mhead->mgmt_keep |= MGMT_SET_SLOTLEN;
				mparam->u8Slotlen=meshinfo.m_slot_len;
				printf("set slot len %lf \r\n",slot_info[mparam->u8Slotlen]);

			}
            if(meshinfo.trans_mode_isset)
			{
				isset=TRUE;
				meshinfo.trans_mode_isset=0;
				printf("set trans_mode %#x \r\n",ntohs(meshinfo.m_trans_mode));

				mhead->mgmt_type |= MGMT_SET_TEST_MODE;
				mparam->mgmt_mac_work_mode=meshinfo.m_trans_mode;
			}

        
        if(isset)
			{
				isset=FALSE;
				mhead->mgmt_type = htons(mhead->mgmt_type);
				mhead->mgmt_keep = htons(mhead->mgmt_keep);
				mgmt_netlink_set_param(buffer, buflen,NULL);	
				sleep(1);
				if (!persist_test_db()) {
					printf("[sqlite_unit] persist test.db failed after meshinfo change\n");
				}
		    }	
        }
        else{
            /* 组播模式*/
            bzero(buffer, buflen);
		    mhead->mgmt_head = htons(HEAD);
			mhead->mgmt_len = sizeof(Smgmt_set_param);
			mhead->mgmt_type = 0;
			memcpy(bcast_buf,&meshinfo,sizeof(meshinfo));
			if(SendUDPClient(SOCKET_BCAST_SEND,bcast_buf,sizeof(meshinfo),&S_GROUND_BCAST)<0)
			{
				printf("send broadcast packet fail\r\n");
			}
			sleep(4);//延时4秒后修改自身参数
            if(meshinfo.txpower_isset)
			{
				meshinfo.txpower_isset=0;
				isset=TRUE;
				mhead->mgmt_type |= MGMT_SET_POWER;
				//mhead->mgmt_type = htons(mhead->mgmt_type);
				mparam->mgmt_mac_txpower=meshinfo.m_txpower;
				memcpy(mparam->mgmt_mac_txpower_ch, meshinfo.m_txpower_ch, sizeof(meshinfo.m_txpower_ch));			
			}
			if(meshinfo.power_level_isset)
			{
				meshinfo.power_level_isset=0;
				isset=TRUE;
				mhead->mgmt_keep |= MGMT_SET_POWER_LEVEL;
				mparam->mgmt_mac_power_level=meshinfo.power_level;
			}
			if(meshinfo.power_attenuation_isset)
			{
				meshinfo.power_attenuation_isset=0;
				isset=TRUE;
				mhead->mgmt_keep |= MGMT_SET_POWER_ATTENUATION;
				mparam->mgmt_mac_power_attenuation=meshinfo.power_attenuation;
			}
			if(meshinfo.rx_channel_mode_isset)
			{
				meshinfo.rx_channel_mode_isset=0;
				isset=TRUE;
				mhead->mgmt_keep |= MGMT_SET_RX_CHANNEL_MODE;
				mparam->mgmt_rx_channel_mode = meshinfo.rx_channel_mode;
			}
			if(meshinfo.freq_isset)
			{
				meshinfo.freq_isset=0;
				isset=TRUE;
				mhead->mgmt_type |= MGMT_SET_FREQUENCY;
				//mhead->mgmt_type = htons(mhead->mgmt_type);
				mparam->mgmt_mac_freq=meshinfo.rf_freq;
			}
			if(meshinfo.chanbw_isset)
			{
				meshinfo.chanbw_isset=0;
				isset=TRUE;
				mhead->mgmt_type |= MGMT_SET_BANDWIDTH;
				//mhead->mgmt_type = htons(mhead->mgmt_type);
				mparam->mgmt_mac_bw=meshinfo.m_chanbw;
			}
			if(meshinfo.rate_isset)
			{
				meshinfo.rate_isset=0;
				mhead->mgmt_type |= MGMT_SET_UNICAST_MCS;
				isset=TRUE;
				//mhead->mgmt_type = htons(mhead->mgmt_type);
				mparam->mgmt_virt_unicast_mcs=meshinfo.m_rate;
			}
			if(meshinfo.workmode_isset)
			{
				meshinfo.workmode_isset=0;
				isset=TRUE;
				mhead->mgmt_type |= MGMT_SET_WORKMODE;
				//mhead->mgmt_type = htons(mhead->mgmt_type);
				mparam->mgmt_net_work_mode.NET_work_mode=meshinfo.workmode;
			}
			if(isset)
			{
				isset=FALSE;
				mhead->mgmt_type = htons(mhead->mgmt_type);
				mhead->mgmt_keep = htons(mhead->mgmt_keep);
				mgmt_netlink_set_param(buffer, buflen,NULL);
				if (!persist_test_db()) {
					printf("[sqlite_unit] persist test.db failed during meshinfo multicast sync\n");
				}
			}

        }

        sleep(5);
    }

    sqlite3_close(g_psqlitedb);
    return 0;
}



int busyHandle(void* ptr, int retry_times)
{
    printf("[SQLite] 数据库正忙，正在重试第 %d 次...\n", retry_times);
    sqlite3_sleep(10); // 睡 10 毫秒再试
    return 1;          // 返回非 0，告诉 sqlite 继续重试，别直接报错退出
}


int sqliteinit(void)
{
    sqlite3_mutex1 = CreateLock();
    int rc = sqlite3_open(TEST_DB_SRC, &g_psqlitedb);
    if(rc){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(g_psqlitedb));
        sqlite3_close(g_psqlitedb);
        return -1;
    }
	// 1. 开启 WAL 模式（解决并发，抗崩溃）
	//sqlite3_exec(g_psqlitedb, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);

	// 2. 开启同步写入模式（数据直达物理 Flash，不怕断电）
	//sqlite3_exec(g_psqlitedb, "PRAGMA synchronous=FULL;", NULL, NULL, NULL);
    updateData_init();
    return 0;
}


void updateData_systeminfo(stInData data)
{
    char updateSql[SQLDATALEN];
	//sqlite3 *g_psqlitedb;
    int rc ;

    snprintf(updateSql, sizeof(updateSql), "UPDATE systemInfo SET value = '%s', state = '%s', lib = '%s' WHERE name = '%s';" \
    		, data.value, data.state,data.lib,data.name);

    char* errMsg;
    sqlite3_busy_handler(g_psqlitedb,busyHandle,NULL);
    Lock(&sqlite3_mutex1,0);
   
	rc = sqlite3_exec(g_psqlitedb, updateSql, NULL, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        printf(stderr, "无法更新数据: %s\n", errMsg);
    }

    Unlock(&sqlite3_mutex1);
}
// 传入参数：参数名 (如 "rf_freq")，参数值 (如 1430)
void updateData_systeminfo_qk(const char* name,const int value)
{
    if (!g_psqlitedb) return;

    char value_str[32];
    char updatesql[1024];
    int rc;
    char* errMsg;
    // 1. 将数字转成字符串
    snprintf(value_str, sizeof(value_str), "%d", value);
    // 2. 构造 SQL 语句
    snprintf(updatesql, sizeof(updatesql),
            "UPDATE systemInfo SET value = '%s', state = '1', lib = '0' WHERE name = '%s';"\
            ,value_str, name);
    // 3. 执行 SQL 语句
    sqlite3_busy_handler(g_psqlitedb, busyHandle, NULL);
    Lock(&sqlite3_mutex1,0);

    rc = sqlite3_exec(g_psqlitedb,updatesql,NULL,0,&errMsg);
    if(rc != SQLITE_OK){
        printf(stderr,"无法更新数据：%s\r\n",errMsg);
    }

    Unlock(&sqlite3_mutex1);
}



void updateData_meshinfo_qk(const char* name,const int value)
{


	char value_str[32];
    snprintf(value_str, sizeof(value_str), "%d", value);

    char updateSql[SQLDATALEN];
	//sqlite3 *g_psqlitedb;
    int rc ;

    snprintf(updateSql, sizeof(updateSql), "UPDATE meshInfo SET value = '%s', state = '1', lib = '0' WHERE name = '%s' AND state = '0';" \
    		, value_str,name);

    char* errMsg;
    sqlite3_busy_handler(g_psqlitedb,busyHandle,NULL);
    Lock(&sqlite3_mutex1,0);
   
	rc = sqlite3_exec(g_psqlitedb, updateSql, NULL, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        printf(stderr, "无法更新数据: %s\n", errMsg);
    }

    Unlock(&sqlite3_mutex1);
}

void updateData_meshinfo(stInData data)
{
    char updateSql[SQLDATALEN];
	//sqlite3 *g_psqlitedb;
    int rc ;

    snprintf(updateSql, sizeof(updateSql), "UPDATE meshInfo SET value = '%s', state = '%s', lib = '%s' WHERE name = '%s' AND state = '0';" \
    		, data.value, data.state,data.lib,data.name);

    char* errMsg;
    sqlite3_busy_handler(g_psqlitedb,busyHandle,NULL);
    Lock(&sqlite3_mutex1,0);
   
	rc = sqlite3_exec(g_psqlitedb, updateSql, NULL, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        printf(stderr, "无法更新数据: %s\n", errMsg);
    }

    Unlock(&sqlite3_mutex1);
}

void updateData_linkinfo(stLink *data,int cnt,int selfid)
{
    char updateSql[SQLDATALEN];
	char snr[20];
	char getlv[20];
	char flowrate[20];
	
	int rc;
	
	snprintf(snr,sizeof(snr),"snr%d",data->m_stNbInfo[cnt].nbid1);
	snprintf(getlv,sizeof(getlv),"getlv%d",data->m_stNbInfo[cnt].nbid1);
	snprintf(flowrate,sizeof(flowrate),"flowrate%d",data->m_stNbInfo[cnt].nbid1);

    //printf("link neigh_%d: snr:%d getlv:%d  flowrate:%d  \r\n",data->m_stNbInfo[cnt].nbid1,data->m_stNbInfo[cnt].snr1,);
    snprintf(updateSql, sizeof(updateSql), "UPDATE link SET  %s = %d, %s = %d,%s = %d WHERE id = %d;",\
    		snr,data->m_stNbInfo[cnt].snr1,getlv,data->m_stNbInfo[cnt].getlv1,flowrate,data->m_stNbInfo[cnt].flowrate1,selfid);
	
    char* errMsg;

	sqlite3_busy_handler(g_psqlitedb,busyHandle,NULL);
    Lock(&sqlite3_mutex1,0);

    rc = sqlite3_exec(g_psqlitedb, updateSql, NULL, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        printf(stderr, "无法更新数据: %s\n", errMsg);
    }

	Unlock(&sqlite3_mutex1);

}

void updateData_timeslotinfo(unsigned char data, int selfid)
{
    char updateSql[SQLDATALEN]; 
	int rc;
    //printf("link neigh_%d: snr:%d getlv:%d  flowrate:%d  \r\n",data->m_stNbInfo[cnt].nbid1,data->m_stNbInfo[cnt].snr1,);
    snprintf(updateSql, sizeof(updateSql), "UPDATE timeslot SET color = %d WHERE id = %d;",data,selfid);
	
    char* errMsg;

	sqlite3_busy_handler(g_psqlitedb,busyHandle,NULL);
    Lock(&sqlite3_mutex1,0);

    rc = sqlite3_exec(g_psqlitedb, updateSql, NULL, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        printf(stderr, "无法更新数据: %s\n", errMsg);
    }

	Unlock(&sqlite3_mutex1);

}

int systemexit(void)
{
    if(g_psqlitedb)
    {
        sqlite3_close(g_psqlitedb);
        g_psqlitedb = NULL;
    }
    return 0;
}



