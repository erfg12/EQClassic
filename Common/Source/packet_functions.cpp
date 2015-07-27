// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#include <iostream>
#include <iomanip>
#include <cstring>
#include <zlib.h>
#include <assert.h>

#include "EQCheckTable.h"
#include "EQPacketManager.h"
#include "packet_functions.h"
using namespace std;


namespace EQC
{
	namespace Common
	{
		namespace Network
		{
			void SetEQChecksum(uchar* in_data, int32 in_length)
			{
				unsigned long data;
				unsigned long check = 0xffffffff;

				assert(in_length >= 4 && in_data);

				for(int i=4; i<in_length; i++)
				{
					data = in_data[i];
					data = data ^ (check);
					data = data & 0x000000ff;
					check = check >> 8;
					data = check_table[data];
					check = check ^ data;
				}

				memcpy(in_data, (char*)&check, 4);
			}

			/*
			This is current release as of 23/okt
			*/
			void EncryptProfilePacket(APPLAYER* app)
			{
				int32* data=(int32*)app->pBuffer;
				int32 crypt = 0x65e7;
				int32 next_crypt;

				int32 swap = data[0];
				data[0] = data[app->size/8];
				data[app->size/8] = swap;

				for(int i=0; i<app->size/4;i++)
				{
					next_crypt = crypt+data[i]-0x37a9;
					data[i] = ((data[i]<<0x07)|(data[i]>>0x19))+0x37a9;
					data[i] =  (data[i]<<0x0f)|(data[i]>>0x11);
					data[i] = data[i] - crypt;
					crypt = next_crypt;
				}
			}

			void EncryptSpawnPacket(APPLAYER* app) {
				EncryptSpawnPacket(app->pBuffer, app->size);
			}

			void EncryptSpawnPacket(uchar* pBuffer, int16 size)
			{
				int32* data=(int32*)pBuffer;
				int32 crypt = 0x0000;
				int32 next_crypt;

				for(int i=0; i<size/4;i++)
				{
					next_crypt = crypt+data[i]-0x65e7;
					data[i] = ((data[i]<<0x09)|(data[i]>>0x17))+0x65e7;
					data[i] =  (data[i]<<0x0d)|(data[i]>>0x13);
					data[i] = data[i] - crypt;
					crypt = next_crypt;
				}
			}

			void EncryptZoneSpawnPacket(APPLAYER* app) {
				EncryptZoneSpawnPacket(app->pBuffer, app->size);
			}

			void EncryptZoneSpawnPacket(uchar* pBuffer, int16 size) {
				int32* data=(int32*)pBuffer;
				int32 crypt = 0x0000;
				int32 next_crypt;
				int32 swap = data[0];
				data[0] = data[size/8];
				data[size/8] = swap;
				for(int i=0; i<size/4;i++) {
					next_crypt = crypt+data[i]-0x65e7;
					data[i] = ((data[i]<<0x09)|(data[i]>>0x17))+0x65e7;
					data[i] =  (data[i]<<0x0d)|(data[i]>>0x13);
					data[i] = data[i] - crypt;
					crypt = next_crypt;
				}
			}




			int DeflatePacket(unsigned char* in_data, int in_length, unsigned char* out_data, int max_out_length)
			{
				z_stream zstream;
				int zerror;

				zstream.next_in   = in_data;
				zstream.avail_in  = in_length;
				zstream.zalloc    = Z_NULL;
				zstream.zfree     = Z_NULL;
				zstream.opaque    = Z_NULL;
				deflateInit(&zstream, Z_FINISH);
				zstream.next_out  = out_data;
				zstream.avail_out = max_out_length;
				zerror = deflate(&zstream, Z_FINISH);

				if (zerror == Z_STREAM_END)
				{
					deflateEnd(&zstream);
					//		cout << "Unpacked size:" << in_length << " Packed size:" << zstream.total_out << endl;
					return zstream.total_out;
				}
				else
				{
					zerror = deflateEnd(&zstream);
					//		cout << "Unpack error:" << zerror << endl;
					return 0;
				}
			}

			uint32 InflatePacket(uchar* indata, uint32 indatalen, uchar* outdata, uint32 outdatalen) {
				z_stream zstream;
				int zerror = 0;
				int i;

				zstream.next_in		= indata;
				zstream.avail_in	= indatalen;
				zstream.next_out	= outdata;
				zstream.avail_out	= outdatalen;
				zstream.zalloc		= Z_NULL;
				zstream.zfree		= Z_NULL;
				zstream.opaque		= Z_NULL;

				i = inflateInit2( &zstream, 15 ); 
				if (i != Z_OK) { 
					return 0;
				}

				zerror = inflate( &zstream, Z_FINISH );

				if(zerror == Z_STREAM_END) {
					inflateEnd( &zstream );
					return zstream.total_out;
				}
				else {
					cout << "Error: InflatePacket: inflate() returned " << zerror << " '";
					if (zstream.msg)
						cout << zstream.msg;
					cout << "'" << endl;
					if (zerror == -4 && zstream.msg == 0)
						return 0;
					zerror = inflateEnd( &zstream );
					return 0;
				}	
			}
		}
	}
}