/*  EQEMu:  Everquest Server Emulator
    Copyright (C) 2001-2002  EQEMu Development Team (http://eqemu.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "../common/debug.h"
#include <iomanip>
#include <string.h>
#include <zlib.h>
#include <assert.h>
#include <iostream>

using namespace std;

#include "EQCheckTable.h"
#include "EQNetwork.h"
#include "packet_functions.h"

#ifndef WIN32
#include <netinet/in.h>
#endif

void SetEQChecksum(uchar* in_data, int32 in_length)
{
	unsigned long data;
    unsigned long check = 0xffffffff;

assert(in_length >= 4 && in_data);

    for(int32 i=4; i<in_length; i++)
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

void EncryptProfilePacket(APPLAYER* app) {
	EncryptProfilePacket(app->pBuffer, app->size);
}


void EncryptProfilePacket(uchar* pBuffer, int32 size) {
	int64* data=(int64*)pBuffer;
	int64 crypt = 0x659365E7;
	int64 next_crypt;
	int32 len = size >> 3;

	int64 swap = data[0];
	data[0] = data[len/2];
	data[len/2] = swap;

    for(int32 i=0; i<len;i++) {
		next_crypt = crypt+data[i]-0x422437A9;
		data[i] = ((data[i]>>0x19)|(data[i]<<0x27))+0x422437A9;
		data[i] =  (data[i]<<0x07)|(data[i]>>0x39);
		data[i] = data[i] - crypt;
		crypt = next_crypt;
    }
}

/*void EncryptSpawnPacket(APPLAYER* app) {
	EncryptSpawnPacket(app->pBuffer, app->size);
}

void EncryptSpawnPacket(uchar* pBuffer, int32 size) {
	EncryptZoneSpawnPacket(pBuffer, size);
	int64* data = (int64*) pBuffer;
	int32 len = size >> 3;		// number of int64's
	int64 crypt = 0;
	int64 next_crypt;
	for (int i=0; i<len; i++) {
		next_crypt = crypt + data[i] - 0x65e7;
		data[i] = ((data[i] << 0x09) | (data[i] >> 0x37)) + 0x65E7;
		data[i] = ((data[i] << 0x0d) | (data[i] >> 0x33)) - crypt;
		crypt = next_crypt;
	}
}*/

void EncryptZoneSpawnPacket(APPLAYER* app) {
	EncryptZoneSpawnPacket(app->pBuffer, app->size);
}

void EncryptZoneSpawnPacket(uchar* pBuffer, int32 size) {
	int64* data=(int64*)pBuffer;
	int64 crypt = 0x0000;
	int64 next_crypt;
	int32 len = size >> 3;

	int64 swap = data[0];
	data[0] = data[len/2];
	data[len/2] = swap;

    for(int32 i=0; i<len;i++) {
		next_crypt = crypt+data[i]-0x659365E7;
		data[i] = ((data[i]<<0x1d)|(data[i]>>0x23))+0x659365E7;
		data[i] =  (data[i]<<0x0e)|(data[i]>>0x32);
		data[i] = data[i] - crypt;
		crypt = next_crypt;
    }
}

int DeflatePacket(unsigned char* in_data, int in_length, unsigned char* out_data, int max_out_length) {
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

uint32 InflatePacket(uchar* indata, uint32 indatalen, uchar* outdata, uint32 outdatalen, bool iQuiet) {
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
		if (!iQuiet) {
			cout << "Error: InflatePacket: inflate() returned " << zerror << " '";
			if (zstream.msg)
				cout << zstream.msg;

			cout << "'" << endl;
#ifdef DEBUG
			cout<< "Packet contents:" << indata << endl;
#endif
		}
		if (zerror == -4 && zstream.msg == 0)
			return 0;
		zerror = inflateEnd( &zstream );
		return 0;
	}
}

int32 roll(int32 in, int8 bits) {
	return ((in << bits) | (in >> (32-bits)));
}

int64 roll(int64 in, int8 bits) {
	return ((in << bits) | (in >> (64-bits)));
}

int32 rorl(int32 in, int8 bits) {
	return ((in >> bits) | (in << (32-bits)));
}



int64 rorl(int64 in, int8 bits) {
	return ((in >> bits) | (in << (64-bits)));
}

int32 CRCLookup(uchar idx) {
	if (idx == 0)
		return 0x00000000;

	if (idx == 1)
		return 0x77073096;

	if (idx == 2)
		return roll(CRCLookup(1), 1);

	if (idx == 4)
		return 0x076DC419;
    
	for (uchar b=7; b>0; b--) {
		uchar bv = 1 << b;

		if (!(idx ^ bv)) {
			/* bit is only one set */
			return ( roll(CRCLookup (4), b - 2) );
		}

		if (idx&bv) {
			/* bit is set */
			return( CRCLookup(bv) ^ CRCLookup(idx&(bv - 1)) );
		}
	}
	//Failure
	return false;
}

uint32 GenerateCRC(int32 b, int32 bufsize, uchar *buf) {
	int32 CRC = (b ^ 0xFFFFFFFF);
	int32 bufremain = bufsize;
	uchar* bufptr = buf;

	while (bufremain--) {
		CRC = CRCLookup((uchar)(*(bufptr++)^ (CRC&0xFF))) ^ (CRC >> 8);
	}

	return (htonl (CRC ^ 0xFFFFFFFF));
}
