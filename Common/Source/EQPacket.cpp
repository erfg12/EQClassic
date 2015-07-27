// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

/************ EQPacket functions ************/
#include <iostream>
#ifndef WIN32
	#include <netinet/in.h>
#else
	#include <windows.h>
	#include <winsock.h>
#endif

#include "EQPacket.h"
#include "packet_dump.h"
#include "packet_dump_file.h"

bool bDumpPacket2 = false;
using namespace std;

namespace EQC
{
	namespace Common
	{
		namespace Network
		{
			// Constructor
			// Ensures that all fields are clear when created
			EQPacket::EQPacket()
			{
				// Clear Fields
				Clear();
			           
			}

			// Destructor
			// deletes pExtra
			EQPacket::~EQPacket()
			{
				if (pExtra)
				{
					safe_delete(pExtra);//delete pExtra;
				}
			}

			// CRC Table generation code
			int32 EQPacket::RoL(int32 in, int32 bits) 
			{
			  int32 temp, out;

			  temp = in;
			  temp >>= (32 - bits);
			  out = in;
			  out <<= bits;
			  out |= temp;

			  return out;
			}
			
			int32 EQPacket::CRCLookup(uchar idx) 
			{

				if (idx == 0)
				{
					return 0x00000000;
				}
			    
				if (idx == 1)
				{
					return 0x77073096;
				}
			    
				if (idx == 2)
				{
					return RoL(CRCLookup(1), 1);
				}
			     
				if (idx == 4)
				{
					return 0x076DC419;
				}
			    
				for (uchar b=7; b>0; b--)
				{
					uchar bv = 1 << b;
			    
					if (!(idx ^ bv)) 
					{
						/* bit is only one set */
						return ( RoL(CRCLookup (4), b - 2) );
					}

					if (idx&bv) 
					{
						/* bit is set */
						return( CRCLookup(bv) ^ CRCLookup(idx&(bv - 1)) );
					}
				}

				//Failure
				return false;
			}
			int32 EQPacket::GenerateCRC(int32 b, int32 bufsize, uchar *buf) 
			{
			  int32 CRC = (b ^ 0xFFFFFFFF), bufremain = bufsize;
			  uchar  *bufptr = buf;

			  while (bufremain--) 
			  {
				  CRC = CRCLookup((uchar)(*(bufptr++)^ (CRC&0xFF))) ^ (CRC >> 8);
			  }
			  
			  return (htonl (CRC ^ 0xFFFFFFFF));
			}
			/**************************************************************************************/

			//PACKET FUNCTIONS!
			void EQPacket::DecodePacket(int16 length, uchar *pPacket)
			{
				// Local variables
				int16 *intptr   = (int16*) pPacket;
				int16  size     = 0;
			    
				// Start Processing

				if (length < 10) // Agz: Adding checks for illegal packets, so we dont read beyond the buffer
				{                //      Minimum normal packet length is 10
					cerr << "EQPacket.cpp: Illegal packet length" << endl;
					return;      // TODO: Do we want to check crc checksum on the incoming packets?
				}           

				HDR = *((EQPACKET_HDR_INFO*)intptr++);
				size+=2;

			/*	cout << "HDR.a7_SEQEnd   = " << (int) HDR.a7_SEQEnd << endl;
				cout << "HDR.a6_Closing  = " << (int) HDR.a6_Closing << endl;
				cout << "HDR.a5_SEQStart = " << (int) HDR.a5_SEQStart << endl;
				cout << "HDR.a4_ASQ      = " << (int) HDR.a4_ASQ << endl;
				cout << "HDR.a3_Fragment = " << (int) HDR.a3_Fragment << endl;
				cout << "HDR.a2_Closing  = " << (int) HDR.a2_Closing << endl;
				cout << "HDR.a1_ARQ      = " << (int) HDR.a1_ARQ << endl;
				cout << "HDR.a0_Unknown  = " << (int) HDR.a0_Unknown << endl;
				cout << "HDR.b7_Unknown  = " << (int) HDR.b7_Unknown << endl;
				cout << "HDR.b6_Unknown  = " << (int) HDR.b6_Unknown << endl;
				cout << "HDR.b5_Unknown  = " << (int) HDR.b5_Unknown << endl;
				cout << "HDR.b4_Unknown  = " << (int) HDR.b4_Unknown << endl;
				cout << "HDR.b3_Unknown  = " << (int) HDR.b3_Unknown << endl;
				cout << "HDR.b2_ARSP     = " << (int) HDR.b2_ARSP << endl;
				cout << "HDR.b1_Unknown  = " << (int) HDR.b1_Unknown << endl;
				cout << "HDR.b0_SpecARQ  = " << (int) HDR.b0_SpecARQ << endl;
			*/

				dwSEQ = ntohs(*intptr++);
				size+=2;
			    

				/************ CHECK ACK FIELDS ************/
				//Common ACK Response
				if(HDR.b2_ARSP)
				{
					dwARSP = ntohs(*intptr++);
					size+=2;
				}
				// Agz: Dont know what this HDR.b4_Unknown data is, gets sent when there is packetloss
				bool bDumpPacket = false;
				if (HDR.b4_Unknown)
				{
					// cout << "DEBUG: HDR.b4_Unknown" << endl;
					// DumpPacket(pPacket, length);
					size++; // One unknown byte
					intptr = (int16*)&pPacket[size]; // Stepping intptr half a step...
					bDumpPacket = true;
				}
				/*
				  See Agz's comment above about HDR.b4.
				  Same story with b5, b6, b7, but they're 2, 4 and 8 bytes respectively.
				  -Quagmire
				*/
				if (HDR.b5_Unknown)
				{
					// cout << "DEBUG: HDR.b5_Unknown" << endl;
					bDumpPacket = true;
					size += 2; // 2 unknown bytes
					intptr++;
				}
				if (HDR.b6_Unknown)
				{
					// cout << "DEBUG: HDR.b6_Unknown" << endl;
					bDumpPacket = true;
					size += 4; // 4 unknown bytes
					intptr += 2;
				}
				if (HDR.b7_Unknown)
				{
					//cout << "DEBUG: HDR.b7_Unknown" << endl;
					bDumpPacket = true;
					size += 8; // 8 unknown bytes
					intptr += 4;
				}
			/*
				if (bDumpPacket) 
				{
					bDumpPacket2 = true;
					cout << "Unknown Headers! b4:";
				if (HDR.b4_Unknown)
				{
					cout << "Y";
				}
				else
				{
					cout << " ";
				}

				cout << "|b5:";
				
				if (HDR.b5_Unknown)
				{
					cout << "Y";
				}
				else
				{
					cout << " ";
				}

				cout << "|b6:";

				if (HDR.b6_Unknown)
				{
					cout << "Y";
				}
				else
				{
					cout << " ";
				}

				cout << "|b7:";
				
				if (HDR.b7_Unknown)
				{
					cout << "Y";
				}
				else
				{
					cout << " ";
				}

				cout << endl;

				DumpPacket(pPacket, length);
			}

			//if (bDumpPacket) {
			FilePrintLine("rawpacket.txt", true);
			FilePrintLine("rawpacket.txt", false, "HDR.a7_SEQEnd   = %i", (int) HDR.a7_SEQEnd);
			FilePrintLine("rawpacket.txt", false, "HDR.a6_Closing  = %i", (int) HDR.a6_Closing);
			FilePrintLine("rawpacket.txt", false, "HDR.a5_SEQStart = %i", (int) HDR.a5_SEQStart);
			FilePrintLine("rawpacket.txt", false, "HDR.a4_ASQ      = %i", (int) HDR.a4_ASQ);
			FilePrintLine("rawpacket.txt", false, "HDR.a3_Fragment = %i", (int) HDR.a3_Fragment);
			FilePrintLine("rawpacket.txt", false, "HDR.a2_Closing  = %i", (int) HDR.a2_Closing);
			FilePrintLine("rawpacket.txt", false, "HDR.a1_ARQ      = %i", (int) HDR.a1_ARQ);
			FilePrintLine("rawpacket.txt", false, "HDR.a0_Unknown  = %i", (int) HDR.a0_Unknown);
			FilePrintLine("rawpacket.txt", false, "HDR.b7_Unknown  = %i", (int) HDR.b7_Unknown);
			FilePrintLine("rawpacket.txt", false, "HDR.b6_Unknown  = %i", (int) HDR.b6_Unknown);
			FilePrintLine("rawpacket.txt", false, "HDR.b5_Unknown  = %i", (int) HDR.b5_Unknown);
			FilePrintLine("rawpacket.txt", false, "HDR.b4_Unknown  = %i", (int) HDR.b4_Unknown);
			FilePrintLine("rawpacket.txt", false, "HDR.b3_Unknown  = %i", (int) HDR.b3_Unknown);
			FilePrintLine("rawpacket.txt", false, "HDR.b2_ARSP     = %i", (int) HDR.b2_ARSP);
			FilePrintLine("rawpacket.txt", false, "HDR.b1_Unknown  = %i", (int) HDR.b1_Unknown);
			FilePrintLine("rawpacket.txt", false, "HDR.b0_SpecARQ  = %i", (int) HDR.b0_SpecARQ);
			FileDumpPacketHex("rawpacket.txt", pPacket, length);
			}*/

			    
				//Common  ACK Request
				if(HDR.a1_ARQ)
				{
					dwARQ = ntohs(*intptr++);
					size+=2;
				}
				/************ END CHECK ACK FIELDS ************/



				/************ CHECK FRAGMENTS ************/
				if(HDR.a3_Fragment)
				{ 
					if (length < 16) // Agz: Adding checks for illegal packets, so we dont read beyond the buffer
					{
						cerr << "Illegal packet length" << endl;
						return;
					}
					size += 6;
					pPacket += size;

					//Extract frag info.
					fraginfo.dwSeq    = ntohs(*intptr++);
					fraginfo.dwCurr   = ntohs(*intptr++);
					fraginfo.dwTotal  = ntohs(*intptr++);
				}
				/************ END CHECK FRAGMENTS ************/
			    


				/************ CHECK ACK SEQUENCE ************/
				if(HDR.a4_ASQ && HDR.a1_ARQ)
				{
					dbASQ_high = ((char*)intptr)[0];
					dbASQ_low  = ((char*)intptr)[1];
					intptr++;
					size+=2;
				}
				else
				{
					if(HDR.a4_ASQ)
					{
						dbASQ_high = ((char*)intptr)[0]; intptr = (int16*)&pPacket[size+1]; // Agz: This better?
						// dbASQ_high = ((char*)intptr++)[0];
						size+=1;
					}
				}
				/************ END CHECK ACK SEQUENCE ************/
			    
				/************ GET OPCODE/EXTRACT DATA ************/
				
				if(length-size > 4 && !(HDR.a2_Closing && HDR.a6_Closing)) // Agz: Another bug fix. I got a arq&arsp packet with no data from the client.
				{
					/************ Extract applayer ************/
					if(!fraginfo.dwCurr) //OPCODE only included in first fragment!
					{
						dwOpCode = ntohs(*intptr++);
						size += 2;
					}

					dwExtraSize = length - size - 4;
					if (length < size + 4)
					{
						cerr << "ERROR: Packet we couldnt handle, ExtraSize < 0" << endl;
						DumpPacket(pPacket, length);
						dwExtraSize = 0;    
					}
					if (dwExtraSize > 0)    
					{
						pExtra      = new uchar[dwExtraSize];
						memcpy((void*)pExtra, (void*) intptr, dwExtraSize);
					}
				}
				else
				{   /************ PURE ACK ************/
					dwOpCode    = 0;

					pExtra      = 0;
					dwExtraSize = 0;
				}
				/************ END GET OPCODE/EXTRA DATA ************/


			/************ END PROCESSING ************/
			}
			/**************************************************************************************/
			/************ Create the packet ************/
			uchar* EQPacket::ReturnPacket(int16 *dwLength)
			{
				*dwLength = 0;
				/************ ALLOCATE MEMORY ************/
				int32 length = MAX_HEADER_SIZE + dwExtraSize + 4;
				uchar *pPacket = new uchar[length];
				int16 *temp    = (int16*)pPacket;
			    

				/************ SET INFO BYTES ************/
				temp[0] = *((int16*)&HDR);
				temp[1] = ntohs(dwSEQ);

				temp      += 2;
				*dwLength += 4;
				/************ END SET INFO ************/

				/************ PUT ACK FIELDS ************/
				if(HDR.b2_ARSP)
				{
					temp[0] = ntohs(dwARSP);
					temp++;
					*dwLength+=2;
				}
			    
				if(HDR.a1_ARQ)
				{
					temp[0] = ntohs(dwARQ);
					temp++;
					*dwLength+=2;
				}
				/************ END PUT ACK FIELDS ************/


				/************ GET FRAGMENT INFORMATION ************/
				if(HDR.a3_Fragment)
				{
					temp[0] = ntohs(fraginfo.dwSeq);
					temp[1] = ntohs(fraginfo.dwCurr);
					temp[2] = ntohs(fraginfo.dwTotal);

					*dwLength   += 6;
					temp        += 3;
				}
				/************ END FRAGMENT INFO ************/

				/************ PUT ACKSEQ FIELD ************/
				if(HDR.a4_ASQ && HDR.a1_ARQ)
				{
					((char*)temp)[0] = dbASQ_high;
					((char*)temp)[1] = dbASQ_low;

					*dwLength   += 2;
					temp++;
				}
				else
				{
					if(HDR.a4_ASQ)
					{
						((char*)temp++)[0] = dbASQ_high;
						(*dwLength)++;
					}
				}
				/************ END PUT ACKSEQ FIELD ************/

				/************ CHECK FOR PURE ACK == NO OPCODE ************/
				if(dwOpCode)
				{
					temp[0] = ntohs(dwOpCode);
					temp++;

					*dwLength+=2;
				}
				if(pExtra)
				{
					memcpy((void*) temp, (void*)pExtra, dwExtraSize);
					*dwLength += dwExtraSize;
				}
				/************ END CHECK FOR PURE ACK == NO OPCODE ************/

			    
				/************ CALCULATE CHECKSUM ************/
			    
				uchar* temp2 = ((uchar*)pPacket)+*dwLength;
				((int32*)temp2)[0] = GenerateCRC(0, *dwLength, (uchar*)pPacket);
				*dwLength+=4;

				/************ END CALCULATE CHECKSUM ************/

				return(pPacket);
			}
		}
	}
}