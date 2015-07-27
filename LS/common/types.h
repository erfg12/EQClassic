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
#ifndef TYPES_H
#define TYPES_H

// TODO: If we require signed or unsigned we should the s and u types..

typedef unsigned char		int8;
typedef unsigned short		int16;
typedef unsigned int		int32;

typedef unsigned char		uint8;
typedef  signed  char		sint8;
typedef unsigned short		uint16;
typedef  signed  short		sint16;
typedef unsigned int		uint32;
typedef  signed  int		sint32;

#ifdef WIN32
	#if defined(_INTEGRAL_MAX_BITS) && _INTEGRAL_MAX_BITS >= 64
		typedef unsigned __int64	int64;
		typedef unsigned __int64	uint64;
		typedef signed __int64		sint64;
	#else
		#error __int64 not supported
	#endif
#else
typedef unsigned long long	int64;
typedef unsigned long long	uint64;
typedef signed long long	sint64;
//typedef __u64				int64;
//typedef __u64				uint64;
//typedef __s64				sint64;
#endif

typedef unsigned long		ulong;
typedef unsigned short		ushort;
typedef unsigned char		uchar;

#ifdef WIN32
	#define snprintf	_snprintf
	//#define vsnprintf	_vsnprintf
	#define strncasecmp	_strnicmp
	#define strcasecmp  _stricmp
	typedef void ThreadReturnType;
#else
	typedef void* ThreadReturnType;
	typedef int SOCKET;
#endif

#define safe_delete(d) if(d) { delete d; d=0; }
#define L32(i)	((int32) i)
#define H32(i)	((int32) (i >> 32))
#define L16(i)	((int16) i)

#pragma pack(1)
struct uint16_breakdown {
	union {
		uint16 all;
		struct {
			uint8 b1;
			uint8 b2;
		} bytes;
	};
	inline uint16&	operator=(const uint16& val) { return (all=val); }
	inline uint16*	operator&() { return &all; }
	inline operator	uint16&() { return all; }
	inline uint8&	b1()	{ return bytes.b1; }
	inline uint8&	b2()	{ return bytes.b2; }
};

struct uint32_breakdown {
	union {
		uint32 all;
		struct {
			uint16 w1;
			uint16 w2;
		} words;
		struct {
			uint8 b1;
			union {
				struct {
					uint8 b2;
					uint8 b3;
				} middle;
				uint16 w2_3; // word bytes 2 to 3
			};
			uint8 b4;
		} bytes;
	};
	inline uint32&	operator=(const uint32& val) { return (all=val); }
	inline uint32*	operator&() { return &all; }
	inline operator	uint32&() { return all; }

	inline uint16&	w1()	{ return words.w1; }
	inline uint16&	w2()	{ return words.w2; }
	inline uint16&	w2_3()	{ return bytes.w2_3; }
	inline uint8&	b1()	{ return bytes.b1; }
	inline uint8&	b2()	{ return bytes.middle.b2; }
	inline uint8&	b3()	{ return bytes.middle.b3; }
	inline uint8&	b4()	{ return bytes.b4; }
};
/*
struct uint64_breakdown {
	union {
		uint64	all;
		struct {
			uint16	w1;	// 1 2
			uint16	w2;	// 3 4
			uint16	w3; // 5 6
			uint16	w4; // 7 8
		};
		struct {
			uint32	dw1; // 1 4
			uint32	dw2; // 5 6
		};
		struct {
			uint8	b1;
			union {
				struct {
					uint16	w2_3;
					uint16	w4_5;
					uint16	w6_7;
				};
				uint32	dw2_5;
				struct {
					uint8	b2;
					union {
						uint32	dw3_6;
						struct {
							uint8	b3;
							union {
								uint32	dw4_7;
								struct {
									uint8	b4;
									uint8	b5;
									uint8	b6;
									uint8	b7;
								};
							};
						};
					};
				};
			};
		};
	};
	inline uint64* operator&() { return &all; }
	inline operator uint64&() { return all; }
};
*/
#pragma pack()


#endif
