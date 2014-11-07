﻿// This code is part of DNSPing(Windows)
// DNSPing, Ping with DNS requesting.
// Copyright (C) 2014 Chengr28
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include "DNSPing.h"

std::string TestDomain, TargetString, TargetDomainString;
std::wstring wTargetString, OutputFileName;
long double TotalTime = 0, MaxTime = 0, MinTime = 0;
size_t SendNum = DEFAULT_SEND_TIMES, RealSendNum = 0, RecvNum = 0, TransmissionInterval = 0, BufferSize = PACKET_MAXSIZE, RawDataLen = 0, EDNS0PayloadSize = 0;
sockaddr_storage SockAddr = {0};
uint16_t Protocol = 0, ServiceName = 0;
std::shared_ptr<char> RawData;
int SocketTimeout = DEFAULT_TIME_OUT, IP_HopLimits = 0;
auto RawSocket = false, IPv4_DF = false, EDNS0 = false, DNSSEC = false, Validate = true;
dns_hdr HeaderParameter = {0};
dns_qry QueryParameter = {0};
dns_edns0_label EDNS0Parameter = {0};
FILE *OutputFile = nullptr;

//Main function of program
int _tmain(int argc, _TCHAR* argv[])
{
//Handle the system signal.
	if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE) == false)
	{
		wprintf(L"\nSet console ctrl handler error, error code is %lu.\n", GetLastError());
		return EXIT_FAILURE;
	}

//Winsock initialization
	WSAData WSAInitialization = {0};
	if (WSAStartup(MAKEWORD(2, 2), &WSAInitialization) != 0 || LOBYTE(WSAInitialization.wVersion) != 2 || HIBYTE(WSAInitialization.wVersion) != 2)
	{
		wprintf(L"\nWinsock initialization error, error code is %d.\n", WSAGetLastError());

		WSACleanup();
		return EXIT_FAILURE;
	}

//Main
	if (argc > 2)
	{
		std::wstring Parameter;
		SSIZE_T Result = 0;

	//Read parameter
		auto ReverseLookup = false;
		for (size_t Index = 1U;Index < (size_t)argc;Index++)
		{
			Parameter = argv[Index];
			Result = 0;

		//Description(Usage)
			if (Parameter.find(L"?") != std::string::npos || Parameter == (L"-H") || Parameter == (L"-h"))
			{
				PrintDescription();
			}
		//Pings the specified host until stopped. To see statistics and continue type Control-Break. To stop type Control-C.
			else if (Parameter == (L"-t"))
			{
				SendNum = 0;
			}
		//Resolve addresses to host names.
			else if (Parameter == (L"-a"))
			{
				ReverseLookup = true;
			}
		//Set number of echo requests to send.
			else if (Parameter == (L"-n"))
			{
				if (Index + 1U < (size_t)argc)
				{
					Index++;
					Parameter = argv[Index];

					Result = wcstol(Parameter.c_str(), nullptr, NULL);
					if (Result > 0 && Result <= U16_MAXNUM)
					{
						SendNum = Result;
					}
					else {
						wprintf(L"\nParameter[-n Count] error.\n");

						WSACleanup();
						return EXIT_FAILURE;
					}
				}
				else {
					wprintf(L"\nNot enough parameters error.\n");

					WSACleanup();
					return EXIT_FAILURE;
				}
			}
		//Set the "Don't Fragment" flag in outgoing packets.
			else if (Parameter == (L"-f"))
			{
				IPv4_DF = true;
			}
		//Specifie a Time To Live for outgoing packets.
			else if (Parameter == (L"-i"))
			{
				if (Index + 1U < (size_t)argc)
				{
					Index++;
					Parameter = argv[Index];

					Result = wcstol(Parameter.c_str(), nullptr, NULL);
					if (Result > 0 && Result <= U8_MAXNUM)
					{
						IP_HopLimits = (int)Result;
					}
					else {
						wprintf(L"\nParameter[-i HopLimit/TTL] error.\n");

						WSACleanup();
						return EXIT_FAILURE;
					}
				}
				else {
					wprintf(L"\nNot enough parameters error.\n");

					WSACleanup();
					return EXIT_FAILURE;
				}
			}
		//Set a long wait periods (in milliseconds) for a response.
			else if (Parameter == (L"-w"))
			{
				if (Index + 1U < (size_t)argc)
				{
					Index++;
					Parameter = argv[Index];

					Result = wcstol(Parameter.c_str(), nullptr, NULL);
					if (Result >= TIME_OUT_MIN && Result < U16_MAXNUM)
					{
					//Minimum supported system of Windows Version Helpers is Windows Vista.
					#ifdef _WIN64
						if (IsWindows8OrGreater())
					#else
						if (IsLowerThanWin8())
					#endif
							SocketTimeout = (int)Result;
						else
							SocketTimeout = (int)(Result - 500);
					}
					else {
						wprintf(L"\nParameter[-w Timeout] error.\n");

						WSACleanup();
						return EXIT_FAILURE;
					}
				}
				else {
					wprintf(L"\nNot enough parameters error.\n");

					WSACleanup();
					return EXIT_FAILURE;
				}
			}
		//Specifie DNS header ID.
			else if (Parameter == (L"-ID") || Parameter == (L"-id"))
			{
				if (Index + 1U < (size_t)argc)
				{
					Index++;
					Parameter = argv[Index];

					Result = wcstol(Parameter.c_str(), nullptr, NULL);
					if (Result > 0 && Result <= U16_MAXNUM)
					{
						HeaderParameter.ID = htons((uint16_t)Result);
					}
					else {
						wprintf(L"\nParameter[-id DNS_ID] error.\n");

						WSACleanup();
						return EXIT_FAILURE;
					}
				}
				else {
					wprintf(L"\nNot enough parameters error.\n");

					WSACleanup();
					return EXIT_FAILURE;
				}
			}
		//Set DNS header flag: QR
			else if (Parameter == (L"-QR") || Parameter == (L"-qr"))
			{
				HeaderParameter.FlagsBits.QR = ~HeaderParameter.FlagsBits.QR;
			}
		//Specifie DNS header OPCode.
			else if (Parameter == (L"-OPCode") || Parameter == (L"-opcode"))
			{
				if (Index + 1U < (size_t)argc)
				{
					Index++;
					Parameter = argv[Index];

					Result = wcstol(Parameter.c_str(), nullptr, NULL);
					if (Result > 0 && Result <= U4_MAXNUM)
					{
					#if __BYTE_ORDER == __LITTLE_ENDIAN
						uint16_t TempFlags = (uint16_t)Result;
						TempFlags = htons(TempFlags << 11U);
						HeaderParameter.Flags = HeaderParameter.Flags | TempFlags;
					#else //Big-Endian
						uint8_t TempFlags = (uint8_t)Result;
						TempFlags = TempFlags & 15;//0x00001111
						HeaderParameter.FlagsBits.OPCode = TempFlags;
					#endif
					}
					else {
						wprintf(L"\nParameter[-opcode OPCode] error.\n");

						WSACleanup();
						return EXIT_FAILURE;
					}
				}
				else {
					wprintf(L"\nNot enough parameters error.\n");

					WSACleanup();
					return EXIT_FAILURE;
				}
			}
		//Set DNS header flag: AA
			else if (Parameter == (L"-AA") || Parameter == (L"-aa"))
			{
				HeaderParameter.FlagsBits.AA = ~HeaderParameter.FlagsBits.AA;
			}
		//Set DNS header flag: TC
			else if (Parameter == (L"-TC") || Parameter == (L"-tc"))
			{
				HeaderParameter.FlagsBits.TC = ~HeaderParameter.FlagsBits.TC;
			}
		//Set DNS header flag: RD
			else if (Parameter == (L"-RD") || Parameter == (L"-rd"))
			{
				HeaderParameter.FlagsBits.RD = ~HeaderParameter.FlagsBits.RD;
			}
		//Set DNS header flag: RA
			else if (Parameter == (L"-RA") || Parameter == (L"-ra"))
			{
				HeaderParameter.FlagsBits.RA = ~HeaderParameter.FlagsBits.RA;
			}
		//Set DNS header flag: AD
			else if (Parameter == (L"-AD") || Parameter == (L"-ad"))
			{
				HeaderParameter.FlagsBits.AD = ~HeaderParameter.FlagsBits.AD;
			}
		//Set DNS header flag: CD
			else if (Parameter == (L"-CD") || Parameter == (L"-cd"))
			{
				HeaderParameter.FlagsBits.CD = ~HeaderParameter.FlagsBits.CD;
			}
		//Specifie DNS header RCode.
			else if (Parameter == (L"-RCode") || Parameter == (L"-rcode"))
			{
				if (Index + 1U < (size_t)argc)
				{
					Index++;
					Parameter = argv[Index];

					Result = wcstol(Parameter.c_str(), nullptr, NULL);
					if (Result > 0 && Result <= U4_MAXNUM)
					{
					#if __BYTE_ORDER == __LITTLE_ENDIAN
						uint16_t TempFlags = (uint16_t)Result;
						TempFlags = htons(TempFlags);
						HeaderParameter.Flags = HeaderParameter.Flags | TempFlags;
					#else //Big-Endian
						uint8_t TempFlags = (uint8_t)Result;
						TempFlags = TempFlags & 15; //0x00001111
						HeaderParameter.FlagsBits.RCode = TempFlags;
					#endif
					}
					else {
						wprintf(L"\nParameter[-rcode RCode] error.\n");

						WSACleanup();
						return EXIT_FAILURE;
					}
				}
				else {
					wprintf(L"\nNot enough parameters error.\n");

					WSACleanup();
					return EXIT_FAILURE;
				}
			}
		//Specifie DNS header question count.
			else if (Parameter == (L"-QN") || Parameter == (L"-qn"))
			{
				if (Index + 1U < (size_t)argc)
				{
					Index++;
					Parameter = argv[Index];

					Result = wcstol(Parameter.c_str(), nullptr, NULL);
					if (Result > 0 && Result <= U16_MAXNUM)
					{
						HeaderParameter.Questions = htons((uint16_t)Result);
					}
					else {
						wprintf(L"\nParameter[-qn Count] error.\n");

						WSACleanup();
						return EXIT_FAILURE;
					}
				}
				else {
					wprintf(L"\nNot enough parameters error.\n");

					WSACleanup();
					return EXIT_FAILURE;
				}
			}
		//Specifie DNS header Answer count.
			else if (Parameter == (L"-ANN") || Parameter == (L"-ann"))
			{
				if (Index + 1U < (size_t)argc)
				{
					Index++;
					Parameter = argv[Index];

					Result = wcstol(Parameter.c_str(), nullptr, NULL);
					if (Result > 0 && Result <= U16_MAXNUM)
					{
						HeaderParameter.Answer = htons((uint16_t)Result);
					}
					else {
						wprintf(L"\nParameter[-ann Count] error.\n");

						WSACleanup();
						return EXIT_FAILURE;
					}
				}
				else {
					wprintf(L"\nNot enough parameters error.\n");

					WSACleanup();
					return EXIT_FAILURE;
				}
			}
		//Specifie DNS header Authority count.
			else if (Parameter == (L"-AUN") || Parameter == (L"-aun"))
			{
				if (Index + 1U < (size_t)argc)
				{
					Index++;
					Parameter = argv[Index];

					Result = wcstol(Parameter.c_str(), nullptr, NULL);
					if (Result > 0 && Result <= U16_MAXNUM)
					{
						HeaderParameter.Authority = htons((uint16_t)Result);
					}
					else {
						wprintf(L"\nParameter[-aun Count] error.\n");

						WSACleanup();
						return EXIT_FAILURE;
					}
				}
				else {
					wprintf(L"\nNot enough parameters error.\n");

					WSACleanup();
					return EXIT_FAILURE;
				}
			}
		//Specifie DNS header Additional count.
			else if (Parameter == (L"-ADN") || Parameter == (L"-adn"))
			{
				if (Index + 1U < (size_t)argc)
				{
					Index++;
					Parameter = argv[Index];

					Result = wcstol(Parameter.c_str(), nullptr, NULL);
					if (Result > 0 && Result <= U16_MAXNUM)
					{
						HeaderParameter.Additional = htons((uint16_t)Result);
					}
					else {
						wprintf(L"\nParameter[-adn Count] error.\n");

						WSACleanup();
						return EXIT_FAILURE;
					}
				}
				else {
					wprintf(L"\nNot enough parameters error.\n");

					WSACleanup();
					return EXIT_FAILURE;
				}
			}
		//Specifie transmission interval time(in milliseconds).
			else if (Parameter == (L"-Ti") || Parameter == (L"-ti"))
			{
				if (Index + 1U < (size_t)argc)
				{
					Index++;
					Parameter = argv[Index];

					Result = wcstol(Parameter.c_str(), nullptr, NULL);
					if (Result >= 0)
					{
						TransmissionInterval = Result;
					}
					else {
						wprintf(L"\nParameter[-ti IntervalTime] error.\n");

						WSACleanup();
						return EXIT_FAILURE;
					}
				}
				else {
					wprintf(L"\nNot enough parameters error.\n");

					WSACleanup();
					return EXIT_FAILURE;
				}
			}
		//Send with EDNS0 Label.
			else if (Parameter == (L"-EDNS0") || Parameter == (L"-edns0"))
			{
				EDNS0 = true;
			}
		//Specifie EDNS0 Label UDP Payload length.
			else if (Parameter == (L"-Payload") || Parameter == (L"-payload"))
			{
				if (Index + 1U < (size_t)argc)
				{
					Index++;
					Parameter = argv[Index];

					Result = wcstol(Parameter.c_str(), nullptr, NULL);
					if (Result > OLD_DNS_MAXSIZE && Result <= U16_MAXNUM)
					{
						EDNS0PayloadSize = Result;
					}
					else {
						wprintf(L"\nParameter[-payload Length] error.\n");

						WSACleanup();
						return EXIT_FAILURE;
					}
				}
				else {
					wprintf(L"\nNot enough parameters error.\n");

					WSACleanup();
					return EXIT_FAILURE;
				}

				EDNS0 = true;
			}
		//Send with DNSSEC requesting.
			else if (Parameter == (L"-DNSSEC") || Parameter == (L"-dnssec"))
			{
				EDNS0 = true;
				DNSSEC = true;
			}
		//Specifie Query Type.
			else if (Parameter == (L"-QT") || Parameter == (L"-qt"))
			{
				if (Index + 1U < (size_t)argc)
				{
					Index++;
					Parameter = argv[Index];

				//Type name
					if (Parameter == (L"A") || Parameter == (L"a"))
					{
						QueryParameter.Type = htons(DNS_A_RECORDS);
					}
					else if (Parameter == (L"NS") || Parameter == (L"ns"))
					{
						QueryParameter.Type = htons(DNS_NS_RECORDS);
					}
					else if (Parameter == (L"CNAME") || Parameter == (L"cname"))
					{
						QueryParameter.Type = htons(DNS_CNAME_RECORDS);
					}
					else if (Parameter == (L"SOA") || Parameter == (L"soa"))
					{
						QueryParameter.Type = htons(DNS_SOA_RECORDS);
					}
					else if (Parameter == (L"PTR") || Parameter == (L"ptr"))
					{
						QueryParameter.Type = htons(DNS_PTR_RECORDS);
					}
					else if (Parameter == (L"MX") || Parameter == (L"mx"))
					{
						QueryParameter.Type = htons(DNS_MX_RECORDS);
					}
					else if (Parameter == (L"TXT") || Parameter == (L"txt"))
					{
						QueryParameter.Type = htons(DNS_TXT_RECORDS);
					}
					else if (Parameter == (L"RP") || Parameter == (L"rp"))
					{
						QueryParameter.Type = htons(DNS_RP_RECORDS);
					}
					else if (Parameter == (L"SIG") || Parameter == (L"sig"))
					{
						QueryParameter.Type = htons(DNS_SIG_RECORDS);
					}
					else if (Parameter == (L"KEY") || Parameter == (L"key"))
					{
						QueryParameter.Type = htons(DNS_KEY_RECORDS);
					}
					else if (Parameter == (L"AAAA") || Parameter == (L"aaaa"))
					{
						QueryParameter.Type = htons(DNS_AAAA_RECORDS);
					}
					else if (Parameter == (L"LOC") || Parameter == (L"loc"))
					{
						QueryParameter.Type = htons(DNS_LOC_RECORDS);
					}
					else if (Parameter == (L"SRV") || Parameter == (L"srv"))
					{
						QueryParameter.Type = htons(DNS_SRV_RECORDS);
					}
					else if (Parameter == (L"NAPTR") || Parameter == (L"naptr"))
					{
						QueryParameter.Type = htons(DNS_NAPTR_RECORDS);
					}
					else if (Parameter == (L"KX") || Parameter == (L"kx"))
					{
						QueryParameter.Type = htons(DNS_KX_RECORDS);
					}
					else if (Parameter == (L"CERT") || Parameter == (L"cert"))
					{
						QueryParameter.Type = htons(DNS_CERT_RECORDS);
					}
					else if (Parameter == (L"DNAME") || Parameter == (L"dname"))
					{
						QueryParameter.Type = htons(DNS_DNAME_RECORDS);
					}
					else if (Parameter == (L"EDNS0") || Parameter == (L"edns0"))
					{
						QueryParameter.Type = htons(DNS_EDNS0_RECORDS);
					}
					else if (Parameter == (L"APL") || Parameter == (L"apl"))
					{
						QueryParameter.Type = htons(DNS_APL_RECORDS);
					}
					else if (Parameter == (L"DS") || Parameter == (L"ds"))
					{
						QueryParameter.Type = htons(DNS_DS_RECORDS);
					}
					else if (Parameter == (L"SSHFP") || Parameter == (L"sshfp"))
					{
						QueryParameter.Type = htons(DNS_SSHFP_RECORDS);
					}
					else if (Parameter == (L"IPSECKEY") || Parameter == (L"ipseckey"))
					{
						QueryParameter.Type = htons(DNS_IPSECKEY_RECORDS);
					}
					else if (Parameter == (L"RRSIG") || Parameter == (L"rrsig"))
					{
						QueryParameter.Type = htons(DNS_RRSIG_RECORDS);
					}
					else if (Parameter == (L"NSEC") || Parameter == (L"nsec"))
					{
						QueryParameter.Type = htons(DNS_NSEC_RECORDS);
					}
					else if (Parameter == (L"DNSKEY") || Parameter == (L"dnskey"))
					{
						QueryParameter.Type = htons(DNS_DNSKEY_RECORDS);
					}
					else if (Parameter == (L"DHCID") || Parameter == (L"dhcid"))
					{
						QueryParameter.Type = htons(DNS_DHCID_RECORDS);
					}
					else if (Parameter == (L"NSEC3") || Parameter == (L"nsec3"))
					{
						QueryParameter.Type = htons(DNS_NSEC3_RECORDS);
					}
					else if (Parameter == (L"NSEC3PARAM") || Parameter == (L"nsec3param"))
					{
						QueryParameter.Type = htons(DNS_NSEC3PARAM_RECORDS);
					}
					else if (Parameter == (L"HIP") || Parameter == (L"hip"))
					{
						QueryParameter.Type = htons(DNS_HIP_RECORDS);
					}
					else if (Parameter == (L"SPF") || Parameter == (L"spf"))
					{
						QueryParameter.Type = htons(DNS_SPF_RECORDS);
					}
					else if (Parameter == (L"TKEY") || Parameter == (L"tkey"))
					{
						QueryParameter.Type = htons(DNS_TKEY_RECORDS);
					}
					else if (Parameter == (L"TSIG") || Parameter == (L"tsig"))
					{
						QueryParameter.Type = htons(DNS_TSIG_RECORDS);
					}
					else if (Parameter == (L"IXFR") || Parameter == (L"ixfr"))
					{
						QueryParameter.Type = htons(DNS_IXFR_RECORDS);
					}
					else if (Parameter == (L"AXFR") || Parameter == (L"axfr"))
					{
						QueryParameter.Type = htons(DNS_AXFR_RECORDS);
					}
					else if (Parameter == (L"ANY") || Parameter == (L"any"))
					{
						QueryParameter.Type = htons(DNS_ANY_RECORDS);
					}
					else if (Parameter == (L"TA") || Parameter == (L"ta"))
					{
						QueryParameter.Type = htons(DNS_TA_RECORDS);
					}
					else if (Parameter == (L"DLV") || Parameter == (L"dlv"))
					{
						QueryParameter.Type = htons(DNS_DLV_RECORDS);
					}
				//Type number
					else {
						Result = wcstol(Parameter.c_str(), nullptr, NULL);
						if (Result > 0 && Result <= U16_MAXNUM)
						{
							QueryParameter.Type = htons((uint16_t)Result);
						}
						else {
							wprintf(L"\nParameter[-qt Type] error.\n");

							WSACleanup();
							return EXIT_FAILURE;
						}
					}
				}
				else {
					wprintf(L"\nNot enough parameters error.\n");

					WSACleanup();
					return EXIT_FAILURE;
				}
			}
		//Specifie Query Classes.
			else if (Parameter == (L"-QC") || Parameter == (L"-qc"))
			{
				if (Index + 1U < (size_t)argc)
				{
					Index++;
					Parameter = argv[Index];

				//Classes name
					if (Parameter == (L"INTERNET") || Parameter == (L"internet") || Parameter == (L"IN") || Parameter == (L"in"))
					{
						QueryParameter.Classes = htons(DNS_CLASS_IN);
					}
					else if (Parameter == (L"CSNET") || Parameter == (L"csnet"))
					{
						QueryParameter.Classes = htons(DNS_CLASS_CSNET);
					}
					else if (Parameter == (L"CHAOS") || Parameter == (L"chaos"))
					{
						QueryParameter.Classes = htons(DNS_CLASS_CHAOS);
					}
					else if (Parameter == (L"HESIOD") || Parameter == (L"hesiod"))
					{
						QueryParameter.Classes = htons(DNS_CLASS_HESIOD);
					}
					else if (Parameter == (L"NONE") || Parameter == (L"none"))
					{
						QueryParameter.Classes = htons(DNS_CLASS_NONE);
					}
					else if (Parameter == (L"ALL") || Parameter == (L"ANY") || Parameter == (L"all") || Parameter == (L"any"))
					{
						QueryParameter.Classes = htons(DNS_CLASS_ANY);
					}
				//Classes number
					else {
						Result = wcstol(Parameter.c_str(), nullptr, NULL);
						if (Result > 0 && Result <= U16_MAXNUM)
						{
							QueryParameter.Classes = htons((uint16_t)Result);
						}
						else {
							wprintf(L"\nParameter[-qc Classes] error.\n");

							WSACleanup();
							return EXIT_FAILURE;
						}
					}
				}
				else {
					wprintf(L"\nNot enough parameters error.\n");

					WSACleanup();
					return EXIT_FAILURE;
				}
			}
		//Specifie requesting server name or port.
			else if (Parameter == (L"-p"))
			{
				if (Index + 1U < (size_t)argc)
				{
					Index++;
					Parameter = argv[Index];

				//Server name
					if (Parameter == (L"TCPMUX") || Parameter == (L"tcpmux"))
					{
						ServiceName = htons(IPPORT_TCPMUX);
					}
					else if (Parameter == (L"ECHO") || Parameter == (L"echo"))
					{
						ServiceName = htons(IPPORT_ECHO);
					}
					else if (Parameter == (L"DISCARD") || Parameter == (L"discard"))
					{
						ServiceName = htons(IPPORT_DISCARD);
					}
					else if (Parameter == (L"SYSTAT") || Parameter == (L"systat"))
					{
						ServiceName = htons(IPPORT_SYSTAT);
					}
					else if (Parameter == (L"DAYTIME") || Parameter == (L"daytime"))
					{
						ServiceName = htons(IPPORT_DAYTIME);
					}
					else if (Parameter == (L"NETSTAT") || Parameter == (L"netstat"))
					{
						ServiceName = htons(IPPORT_NETSTAT);
					}
					else if (Parameter == (L"QOTD") || Parameter == (L"qotd"))
					{
						ServiceName = htons(IPPORT_QOTD);
					}
					else if (Parameter == (L"MSP") || Parameter == (L"msp"))
					{
						ServiceName = htons(IPPORT_MSP);
					}
					else if (Parameter == (L"CHARGEN") || Parameter == (L"chargen"))
					{
						ServiceName = htons(IPPORT_CHARGEN);
					}
					else if (Parameter == (L"FTPDATA") || Parameter == (L"ftpdata"))
					{
						ServiceName = htons(IPPORT_FTP_DATA);
					}
					else if (Parameter == (L"FTP") || Parameter == (L"ftp"))
					{
						ServiceName = htons(IPPORT_FTP);
					}
					else if (Parameter == (L"SSH") || Parameter == (L"ssh"))
					{
						ServiceName = htons(IPPORT_SSH);
					}
					else if (Parameter == (L"TELNET") || Parameter == (L"telnet"))
					{
						ServiceName = htons(IPPORT_TELNET);
					}
					else if (Parameter == (L"SMTP") || Parameter == (L"smtp"))
					{
						ServiceName = htons(IPPORT_SMTP);
					}
					else if (Parameter == (L"TIME") || Parameter == (L"time"))
					{
						ServiceName = htons(IPPORT_TIMESERVER);
					}
					else if (Parameter == (L"RAP") || Parameter == (L"rap"))
					{
						ServiceName = htons(IPPORT_RAP);
					}
					else if (Parameter == (L"RLP") || Parameter == (L"rlp"))
					{
						ServiceName = htons(IPPORT_RLP);
					}
					else if (Parameter == (L"NAME") || Parameter == (L"name"))
					{
						ServiceName = htons(IPPORT_NAMESERVER);
					}
					else if (Parameter == (L"WHOIS") || Parameter == (L"whois"))
					{
						ServiceName = htons(IPPORT_WHOIS);
					}
					else if (Parameter == (L"TACACS") || Parameter == (L"tacacs"))
					{
						ServiceName = htons(IPPORT_TACACS);
					}
					else if (Parameter == (L"DNS") || Parameter == (L"dns"))
					{
						ServiceName = htons(DNS_PORT);
					}
					else if (Parameter == (L"XNSAUTH") || Parameter == (L"xnsauth"))
					{
						ServiceName = htons(IPPORT_XNSAUTH);
					}
					else if (Parameter == (L"MTP") || Parameter == (L"mtp"))
					{
						ServiceName = htons(IPPORT_MTP);
					}
					else if (Parameter == (L"BOOTPS") || Parameter == (L"bootps"))
					{
						ServiceName = htons(IPPORT_BOOTPS);
					}
					else if (Parameter == (L"BOOTPC") || Parameter == (L"bootpc"))
					{
						ServiceName = htons(IPPORT_BOOTPC);
					}
					else if (Parameter == (L"TFTP") || Parameter == (L"tftp"))
					{
						ServiceName = htons(IPPORT_TFTP);
					}
					else if (Parameter == (L"RJE") || Parameter == (L"rje"))
					{
						ServiceName = htons(IPPORT_RJE);
					}
					else if (Parameter == (L"FINGER") || Parameter == (L"finger"))
					{
						ServiceName = htons(IPPORT_FINGER);
					}
					else if (Parameter == (L"HTTP") || Parameter == (L"http"))
					{
						ServiceName = htons(IPPORT_HTTP);
					}
					else if (Parameter == (L"HTTPBACKUP") || Parameter == (L"httpbackup"))
					{
						ServiceName = htons(IPPORT_HTTPBACKUP);
					}
					else if (Parameter == (L"TTYLINK") || Parameter == (L"ttylink"))
					{
						ServiceName = htons(IPPORT_TTYLINK);
					}
					else if (Parameter == (L"SUPDUP") || Parameter == (L"supdup"))
					{
						ServiceName = htons(IPPORT_SUPDUP);
					}
					else if (Parameter == (L"POP3") || Parameter == (L"pop3"))
					{
						ServiceName = htons(IPPORT_POP3);
					}
					else if (Parameter == (L"SUNRPC") || Parameter == (L"sunrpc"))
					{
						ServiceName = htons(IPPORT_SUNRPC);
					}
					else if (Parameter == (L"SQL") || Parameter == (L"sql"))
					{
						ServiceName = htons(IPPORT_SQL);
					}
					else if (Parameter == (L"NTP") || Parameter == (L"ntp"))
					{
						ServiceName = htons(IPPORT_NTP);
					}
					else if (Parameter == (L"EPMAP") || Parameter == (L"epmap"))
					{
						ServiceName = htons(IPPORT_EPMAP);
					}
					else if (Parameter == (L"NETBIOSNS") || Parameter == (L"netbiosns"))
					{
						ServiceName = htons(IPPORT_NETBIOS_NS);
					}
					else if (Parameter == (L"NETBIOSDGM") || Parameter == (L"netbiosdgm"))
					{
						ServiceName = htons(IPPORT_NETBIOS_DGM);
					}
					else if (Parameter == (L"NETBIOSSSN") || Parameter == (L"netbiosssn"))
					{
						ServiceName = htons(IPPORT_NETBIOS_SSN);
					}
					else if (Parameter == (L"IMAP") || Parameter == (L"imap"))
					{
						ServiceName = htons(IPPORT_IMAP);
					}
					else if (Parameter == (L"BFTP") || Parameter == (L"bftp"))
					{
						ServiceName = htons(IPPORT_BFTP);
					}
					else if (Parameter == (L"SGMP") || Parameter == (L"sgmp"))
					{
						ServiceName = htons(IPPORT_SGMP);
					}
					else if (Parameter == (L"SQLSRV") || Parameter == (L"sqlsrv"))
					{
						ServiceName = htons(IPPORT_SQLSRV);
					}
					else if (Parameter == (L"DMSP") || Parameter == (L"dmsp"))
					{
						ServiceName = htons(IPPORT_DMSP);
					}
					else if (Parameter == (L"SNMP") || Parameter == (L"snmp"))
					{
						ServiceName = htons(IPPORT_SNMP);
					}
					else if (Parameter == (L"SNMPTRAP") || Parameter == (L"snmptrap"))
					{
						ServiceName = htons(IPPORT_SNMP_TRAP);
					}
					else if (Parameter == (L"ATRTMP") || Parameter == (L"atrtmp"))
					{
						ServiceName = htons(IPPORT_ATRTMP);
					}
					else if (Parameter == (L"ATHBP") || Parameter == (L"athbp"))
					{
						ServiceName = htons(IPPORT_ATHBP);
					}
					else if (Parameter == (L"QMTP") || Parameter == (L"qmtp"))
					{
						ServiceName = htons(IPPORT_QMTP);
					}
					else if (Parameter == (L"IPX") || Parameter == (L"ipx"))
					{
						ServiceName = htons(IPPORT_IPX);
					}
					else if (Parameter == (L"IMAP3") || Parameter == (L"imap3"))
					{
						ServiceName = htons(IPPORT_IMAP3);
					}
					else if (Parameter == (L"BGMP") || Parameter == (L"bgmp"))
					{
						ServiceName = htons(IPPORT_BGMP);
					}
					else if (Parameter == (L"TSP") || Parameter == (L"tsp"))
					{
						ServiceName = htons(IPPORT_TSP);
					}
					else if (Parameter == (L"IMMP") || Parameter == (L"immp"))
					{
						ServiceName = htons(IPPORT_IMMP);
					}
					else if (Parameter == (L"ODMR") || Parameter == (L"odmr"))
					{
						ServiceName = htons(IPPORT_ODMR);
					}
					else if (Parameter == (L"RPC2PORTMAP") || Parameter == (L"rpc2portmap"))
					{
						ServiceName = htons(IPPORT_RPC2PORTMAP);
					}
					else if (Parameter == (L"CLEARCASE") || Parameter == (L"clearcase"))
					{
						ServiceName = htons(IPPORT_CLEARCASE);
					}
					else if (Parameter == (L"HPALARMMGR") || Parameter == (L"hpalarmmgr"))
					{
						ServiceName = htons(IPPORT_HPALARMMGR);
					}
					else if (Parameter == (L"ARNS") || Parameter == (L"arns"))
					{
						ServiceName = htons(IPPORT_ARNS);
					}
					else if (Parameter == (L"AURP") || Parameter == (L"aurp"))
					{
						ServiceName = htons(IPPORT_AURP);
					}
					else if (Parameter == (L"LDAP") || Parameter == (L"ldap"))
					{
						ServiceName = htons(IPPORT_LDAP);
					}
					else if (Parameter == (L"UPS") || Parameter == (L"ups"))
					{
						ServiceName = htons(IPPORT_UPS);
					}
					else if (Parameter == (L"SLP") || Parameter == (L"slp"))
					{
						ServiceName = htons(IPPORT_SLP);
					}
					else if (Parameter == (L"HTTPS") || Parameter == (L"https"))
					{
						ServiceName = htons(IPPORT_HTTPS);
					}
					else if (Parameter == (L"SNPP") || Parameter == (L"snpp"))
					{
						ServiceName = htons(IPPORT_SNPP);
					}
					else if (Parameter == (L"MICROSOFTDS") || Parameter == (L"microsoftds"))
					{
						ServiceName = htons(IPPORT_MICROSOFT_DS);
					}
					else if (Parameter == (L"KPASSWD") || Parameter == (L"kpasswd"))
					{
						ServiceName = htons(IPPORT_KPASSWD);
					}
					else if (Parameter == (L"TCPNETHASPSRV") || Parameter == (L"tcpnethaspsrv"))
					{
						ServiceName = htons(IPPORT_TCPNETHASPSRV);
					}
					else if (Parameter == (L"RETROSPECT") || Parameter == (L"retrospect"))
					{
						ServiceName = htons(IPPORT_RETROSPECT);
					}
					else if (Parameter == (L"ISAKMP") || Parameter == (L"isakmp"))
					{
						ServiceName = htons(IPPORT_ISAKMP);
					}
					else if (Parameter == (L"BIFFUDP") || Parameter == (L"biffudp"))
					{
						ServiceName = htons(IPPORT_BIFFUDP);
					}
					else if (Parameter == (L"WHOSERVER") || Parameter == (L"whoserver"))
					{
						ServiceName = htons(IPPORT_WHOSERVER);
					}
					else if (Parameter == (L"SYSLOG") || Parameter == (L"syslog"))
					{
						ServiceName = htons(IPPORT_SYSLOG);
					}
					else if (Parameter == (L"ROUTERSERVER") || Parameter == (L"routerserver"))
					{
						ServiceName = htons(IPPORT_ROUTESERVER);
					}
					else if (Parameter == (L"NCP") || Parameter == (L"ncp"))
					{
						ServiceName = htons(IPPORT_NCP);
					}
					else if (Parameter == (L"COURIER") || Parameter == (L"courier"))
					{
						ServiceName = htons(IPPORT_COURIER);
					}
					else if (Parameter == (L"COMMERCE") || Parameter == (L"commerce"))
					{
						ServiceName = htons(IPPORT_COMMERCE);
					}
					else if (Parameter == (L"RTSP") || Parameter == (L"rtsp"))
					{
						ServiceName = htons(IPPORT_RTSP);
					}
					else if (Parameter == (L"NNTP") || Parameter == (L"nntp"))
					{
						ServiceName = htons(IPPORT_NNTP);
					}
					else if (Parameter == (L"HTTPRPCEPMAP") || Parameter == (L"httprpcepmap"))
					{
						ServiceName = htons(IPPORT_HTTPRPCEPMAP);
					}
					else if (Parameter == (L"IPP") || Parameter == (L"ipp"))
					{
						ServiceName = htons(IPPORT_IPP);
					}
					else if (Parameter == (L"LDAPS") || Parameter == (L"ldaps"))
					{
						ServiceName = htons(IPPORT_LDAPS);
					}
					else if (Parameter == (L"MSDP") || Parameter == (L"msdp"))
					{
						ServiceName = htons(IPPORT_MSDP);
					}
					else if (Parameter == (L"AODV") || Parameter == (L"aodv"))
					{
						ServiceName = htons(IPPORT_AODV);
					}
					else if (Parameter == (L"FTPSDATA") || Parameter == (L"ftpsdata"))
					{
						ServiceName = htons(IPPORT_FTPSDATA);
					}
					else if (Parameter == (L"FTPS") || Parameter == (L"ftps"))
					{
						ServiceName = htons(IPPORT_FTPS);
					}
					else if (Parameter == (L"NAS") || Parameter == (L"nas"))
					{
						ServiceName = htons(IPPORT_NAS);
					}
					else {
						if (Parameter == (L"TELNETS") || Parameter == (L"telnets"))
							ServiceName = htons(IPPORT_TELNETS);

					//Number port
						Result = wcstol(Parameter.c_str(), nullptr, NULL);
						if (Result > 0 && Result <= U16_MAXNUM)
						{
							ServiceName = htons((uint16_t)Result);
						}
						else {
							wprintf(L"\nParameter[-p ServiceName/Protocol] error.\n");

							WSACleanup();
							return EXIT_FAILURE;
						}
					}
				}
				else {
					wprintf(L"\nNot enough parameters error.\n");

					WSACleanup();
					return EXIT_FAILURE;
				}
			}
		//Specifie Raw data to send.
			else if (Parameter == (L"-RAWDATA") || Parameter == (L"-rawdata"))
			{
				if (Index + 1U < (size_t)argc)
				{
					Index++;

				//Initialization
					std::shared_ptr<char> RawDataStringPTR(new char[lstrlenW(argv[Index]) + 1U]());
					WideCharToMultiByte(CP_ACP, NULL, Parameter.c_str(), (int)Parameter.length(), RawDataStringPTR.get(), lstrlenW(argv[Index]) + 1U, NULL, NULL);
					std::string RawDataString = RawDataStringPTR.get();
					RawDataStringPTR.reset();
					if (RawDataString.length() < PACKET_MINSIZE && RawDataString.length() > PACKET_MAXSIZE)
					{
						wprintf(L"\nParameter[-rawdata RAW_Data] error.\n");

						WSACleanup();
						return EXIT_FAILURE;
					}
					std::shared_ptr<char> TempRawData(new char[PACKET_MAXSIZE]());
					RawData.swap(TempRawData);
					TempRawData.reset();
					std::shared_ptr<char> Temp(new char[5U]());
					Temp.get()[0] = 48; //"0"
					Temp.get()[1U] = 120; //"x"

				//Read raw data.
					for (size_t InnerIndex = 0; InnerIndex < RawDataString.length(); InnerIndex++)
					{
						Temp.get()[2U] = RawDataString[InnerIndex];
						InnerIndex++;
						Temp.get()[3U] = RawDataString[InnerIndex];
						Result = strtol(Temp.get(), nullptr, NULL);
						if (Result > 0 && Result <= U8_MAXNUM)
						{
							RawData.get()[RawDataLen] = (char)Result;
							RawDataLen++;
						}
						else {
							wprintf(L"\nParameter[-rawdata RAW_Data] error.\n");

							WSACleanup();
							return EXIT_FAILURE;
						}
					}
				}
				else {
					wprintf(L"\nNot enough parameters error.\n");

					WSACleanup();
					return EXIT_FAILURE;
				}
			}
		//Send RAW data with Raw Socket.
			else if (Parameter == (L"-RAW") || Parameter == (L"-raw"))
			{
				if (Index + 1U < (size_t)argc)
				{
					Index++;
					Parameter = argv[Index];

				//Protocol name
					RawSocket = true;
					if (Parameter == (L"UDP") || Parameter == (L"udp"))
					{
						RawSocket = false;
						continue;
					}
					else if (Parameter == (L"HOPOPTS") || Parameter == (L"hopopts"))
					{
						ServiceName = IPPROTO_HOPOPTS;
					}
					else if (Parameter == (L"ICMP") || Parameter == (L"icmp"))
					{
						ServiceName = IPPROTO_ICMP;
					}
					else if (Parameter == (L"IGMP") || Parameter == (L"igmp"))
					{
						ServiceName = IPPROTO_IGMP;
					}
					else if (Parameter == (L"GGP") || Parameter == (L"ggp"))
					{
						ServiceName = IPPROTO_GGP;
					}
					else if (Parameter == (L"IPV4") || Parameter == (L"ipv4"))
					{
						ServiceName = IPPROTO_IPV4;
					}
					else if (Parameter == (L"ST") || Parameter == (L"st"))
					{
						ServiceName = IPPROTO_ST;
					}
					else if (Parameter == (L"TCP") || Parameter == (L"tcp"))
					{
						ServiceName = IPPROTO_TCP;
					}
					else if (Parameter == (L"CBT") || Parameter == (L"cbt"))
					{
						ServiceName = IPPROTO_CBT;
					}
					else if (Parameter == (L"EGP") || Parameter == (L"egp"))
					{
						ServiceName = IPPROTO_EGP;
					}
					else if (Parameter == (L"IGP") || Parameter == (L"igp"))
					{
						ServiceName = IPPROTO_IGP;
					}
					else if (Parameter == (L"PUP") || Parameter == (L"pup"))
					{
						ServiceName = IPPROTO_PUP;
					}
					else if (Parameter == (L"IDP") || Parameter == (L"idp"))
					{
						ServiceName = IPPROTO_IDP;
					}
					else if (Parameter == (L"IPV6") || Parameter == (L"ipv6"))
					{
						ServiceName = IPPROTO_IPV6;
					}
					else if (Parameter == (L"ROUTING") || Parameter == (L"routing"))
					{
						ServiceName = IPPROTO_ROUTING;
					}
					else if (Parameter == (L"ESP") || Parameter == (L"esp"))
					{
						ServiceName = IPPROTO_ESP;
					}
					else if (Parameter == (L"FRAGMENT") || Parameter == (L"fragment"))
					{
						ServiceName = IPPROTO_FRAGMENT;
					}
					else if (Parameter == (L"AH") || Parameter == (L"ah"))
					{
						ServiceName = IPPROTO_AH;
					}
					else if (Parameter == (L"ICMPV6") || Parameter == (L"icmpv6"))
					{
						ServiceName = IPPROTO_ICMPV6;
					}
					else if (Parameter == (L"NONE") || Parameter == (L"none"))
					{
						ServiceName = IPPROTO_NONE;
					}
					else if (Parameter == (L"DSTOPTS") || Parameter == (L"dstopts"))
					{
						ServiceName = IPPROTO_DSTOPTS;
					}
					else if (Parameter == (L"ND") || Parameter == (L"nd"))
					{
						ServiceName = IPPROTO_ND;
					}
					else if (Parameter == (L"ICLFXBM") || Parameter == (L"iclfxbm"))
					{
						ServiceName = IPPROTO_ICLFXBM;
					}
					else if (Parameter == (L"PIM") || Parameter == (L"pim"))
					{
						ServiceName = IPPROTO_PIM;
					}
					else if (Parameter == (L"PGM") || Parameter == (L"pgm"))
					{
						ServiceName = IPPROTO_PGM;
					}
					else if (Parameter == (L"L2TP") || Parameter == (L"l2tp"))
					{
						ServiceName = IPPROTO_L2TP;
					}
					else if (Parameter == (L"SCTP") || Parameter == (L"sctp"))
					{
						ServiceName = IPPROTO_SCTP;
					}
					else if (Parameter == (L"RAW") || Parameter == (L"raw"))
					{
						ServiceName = IPPROTO_RAW;
					}
				//Protocol number
					else {
						Result = wcstol(Parameter.c_str(), nullptr, NULL);
						if (Result > 0 && Result <= U4_MAXNUM)
						{
							ServiceName = (uint8_t)Result;
						}
						else {
							wprintf(L"\nParameter[-raw ServiceName] error.\n");

							WSACleanup();
							return EXIT_FAILURE;
						}
					}
				}
				else {
					wprintf(L"\nNot enough parameters error.\n");

					WSACleanup();
					return EXIT_FAILURE;
				}
			}
		//Specifie buffer size.
			else if (Parameter == (L"-Buf") || Parameter == (L"-buf"))
			{
				if (Index + 1U < (size_t)argc)
				{
					Index++;
					Parameter = argv[Index];

					Result = wcstol(Parameter.c_str(), nullptr, NULL);
					if (Result >= OLD_DNS_MAXSIZE && Result <= LARGE_PACKET_MAXSIZE)
					{
						BufferSize = Result;
					}
					else {
						wprintf(L"\nParameter[-buf Size] error.\n");

						WSACleanup();
						return EXIT_FAILURE;
					}
				}
				else {
					wprintf(L"\nNot enough parameters error.\n");

					WSACleanup();
					return EXIT_FAILURE;
				}
			}
		//Disable packets validated.
			else if (Parameter == (L"-DV") || Parameter == (L"-dv"))
			{
				Validate = false;
			}
		//Output result to file.
			else if (Parameter == (L"-OF") || Parameter == (L"-of"))
			{
				if (Index + 1U < (size_t)argc)
				{
					Index++;
					Parameter = argv[Index];

					if (Parameter.length() <= MAX_PATH)
					{
						OutputFileName = Parameter;
					}
					else {
						wprintf(L"\nParameter[-of FileName] error.\n");

						WSACleanup();
						return EXIT_FAILURE;
					}
				}
				else {
					wprintf(L"\nNot enough parameters error.\n");

					WSACleanup();
					return EXIT_FAILURE;
				}
			}
		//Using IPv6.
			else if (Parameter == (L"-6"))
			{
				Protocol = AF_INET6;
			}
		//Using IPv4.
			else if (Parameter == (L"-4"))
			{
				Protocol = AF_INET;
			}
		//Specifie Query Domain.
			else if (!RawData && TestDomain.empty() && Index == (size_t)(argc - 2) && Parameter.length() > 2U)
			{
				std::shared_ptr<char> TestDomainPTR(new char[Parameter.length() + 1U]());
				WideCharToMultiByte(CP_ACP, NULL, Parameter.c_str(), (int)Parameter.length(), TestDomainPTR.get(), (int)(Parameter.length() + 1U), NULL, NULL);
				TestDomain = TestDomainPTR.get();
			}
		//Specifie target.
			else if (Index == (size_t)(argc - 1) && Parameter.length() > 2U)
			{
			//Initialization
				std::shared_ptr<char> ParameterPTR(new char[Parameter.length() + 1U]());
				WideCharToMultiByte(CP_ACP, NULL, Parameter.c_str(), (int)Parameter.length(), ParameterPTR.get(), (int)(Parameter.length() + 1U), NULL, NULL);
				std::string ParameterString = ParameterPTR.get();
				ParameterPTR.reset();

			//IPv6 address
				if (ParameterString.find(58) != std::string::npos)
				{
					if (Protocol == AF_INET)
					{
						wprintf(L"\nTarget protocol error.\n");

						WSACleanup();
						return EXIT_FAILURE;
					}

					Protocol = AF_INET6;
					SockAddr.ss_family = AF_INET6;
					if (AddressStringToBinary((PSTR)ParameterString.c_str(), &((PSOCKADDR_IN6)&SockAddr)->sin6_addr, AF_INET6, Result) == EXIT_FAILURE)
					{
						wprintf(L"\nTarget format error, error code is %d.\n", (int)Result);

						WSACleanup();
						return EXIT_FAILURE;
					}

					TargetString.append("[");
					TargetString.append(ParameterString);
					TargetString.append("]");
				}
				else {
					for (auto StringIter = ParameterString.begin(); StringIter != ParameterString.end(); StringIter++)
					{
					//Domain
						if (*StringIter < 46 || *StringIter == 47 || *StringIter > 57)
						{
							ADDRINFOA AddrInfoHints = { 0 }, *AddrInfo = nullptr;
						//Try with IPv6.
							if (Protocol == 0)
								Protocol = AF_INET6;
							AddrInfoHints.ai_family = Protocol;
							SockAddr.ss_family = Protocol;

						//Get address.
							if (getaddrinfo(ParameterString.c_str(), NULL, &AddrInfoHints, &AddrInfo) != 0)
							{
							//Retry with IPv4.
								Protocol = AF_INET;
								AddrInfoHints.ai_family = Protocol;
								SockAddr.ss_family = Protocol;

								if (getaddrinfo(ParameterString.c_str(), NULL, &AddrInfoHints, &AddrInfo) != 0)
								{
									wprintf(L"\nResolve domain name error, error code is %d.\n", WSAGetLastError());

									WSACleanup();
									return EXIT_FAILURE;
								}
							}

						//Get address form PTR.
							if (AddrInfo != nullptr)
							{
								for (auto PTR = AddrInfo; PTR != nullptr; PTR = PTR->ai_next)
								{
								//IPv6
									if (PTR->ai_family == AF_INET6 && SockAddr.ss_family == AF_INET6 &&
										!IN6_IS_ADDR_LINKLOCAL((in6_addr *)(PTR->ai_addr)) &&
										!(((PSOCKADDR_IN6)(PTR->ai_addr))->sin6_scope_id == 0)) //Get port from first(Main) IPv6 device
									{
										((PSOCKADDR_IN6)&SockAddr)->sin6_addr = ((PSOCKADDR_IN6)(PTR->ai_addr))->sin6_addr;

									//Get string of address.
										TargetDomainString = ParameterString;
										std::shared_ptr<char> Buffer(new char[ADDR_STRING_MAXSIZE]());

									//Minimum supported system of inet_ntop() and inet_pton() is Windows Vista. [Roy Tam]
									#ifdef _WIN64
										inet_ntop(AF_INET6, &((PSOCKADDR_IN6)&SockAddr)->sin6_addr, Buffer.get(), ADDR_STRING_MAXSIZE);
									#else //x86
										DWORD BufferLength = ADDR_STRING_MAXSIZE;
										WSAAddressToStringA((LPSOCKADDR)&SockAddr, sizeof(sockaddr_in6), NULL, Buffer.get(), &BufferLength);
									#endif

										TargetString.append("[");
										TargetString.append(Buffer.get());
										TargetString.append("]");
										break;
									}
								//IPv4
									else if (PTR->ai_family == AF_INET && SockAddr.ss_family == AF_INET &&
										((PSOCKADDR_IN)(PTR->ai_addr))->sin_addr.S_un.S_addr != INADDR_LOOPBACK &&
										((PSOCKADDR_IN)(PTR->ai_addr))->sin_addr.S_un.S_addr != INADDR_BROADCAST)
									{
										((PSOCKADDR_IN)&SockAddr)->sin_addr = ((PSOCKADDR_IN)(PTR->ai_addr))->sin_addr;

									//Get string of address.
										TargetDomainString = ParameterString;
										std::shared_ptr<char> Buffer(new char[ADDR_STRING_MAXSIZE]());

									//Minimum supported system of inet_ntop() and inet_pton() is Windows Vista. [Roy Tam]
									#ifdef _WIN64
										inet_ntop(AF_INET, &((PSOCKADDR_IN)&SockAddr)->sin_addr, Buffer.get(), ADDR_STRING_MAXSIZE);
									#else //x86
										DWORD BufferLength = ADDR_STRING_MAXSIZE;
										WSAAddressToStringA((LPSOCKADDR)&SockAddr, sizeof(sockaddr_in), NULL, Buffer.get(), &BufferLength);
									#endif

										TargetString = Buffer.get();
										break;
									}
								}

								freeaddrinfo(AddrInfo);
							}

							break;
						}

					//IPv4
						if (StringIter == ParameterString.end() - 1U)
						{
							if (Protocol == AF_INET6)
							{
								wprintf(L"\nTarget protocol error.\n");

								WSACleanup();
								return EXIT_FAILURE;
							}

							Protocol = AF_INET;
							SockAddr.ss_family = AF_INET;
							if (AddressStringToBinary((PSTR)ParameterString.c_str(), &((PSOCKADDR_IN)&SockAddr)->sin_addr, AF_INET, Result) == EXIT_FAILURE)
							{
								wprintf(L"\nTarget format error, error code is %d.\n", (int)Result);

								WSACleanup();
								return EXIT_FAILURE;
							}

							TargetString = ParameterString;
						}
					}
				}
			}
		}

	//Check parameter reading.
		if (SockAddr.ss_family == AF_INET6) //IPv6
		{
			if (CheckEmptyBuffer(&((PSOCKADDR_IN6)&SockAddr)->sin6_addr, sizeof(in6_addr)))
			{
				wprintf(L"\nTarget is empty.\n");

				WSACleanup();
				return EXIT_FAILURE;
			}
			else {
			//Mark port.
				if (ServiceName == 0)
				{
					ServiceName = htons(DNS_PORT);
					((PSOCKADDR_IN6)&SockAddr)->sin6_port = htons(DNS_PORT);
				}
				else {
					((PSOCKADDR_IN6)&SockAddr)->sin6_port = ServiceName;
				}
			}
		}
		else { //IPv4
			if (((PSOCKADDR_IN)&SockAddr)->sin_addr.S_un.S_addr == 0)
			{
				wprintf(L"\nTarget is empty.\n");

				WSACleanup();
				return EXIT_FAILURE;
			}
			else {
			//Mark port.
				if (ServiceName == 0)
				{
					ServiceName = htons(DNS_PORT);
					((PSOCKADDR_IN)&SockAddr)->sin_port = htons(DNS_PORT);
				}
				else {
					((PSOCKADDR_IN)&SockAddr)->sin_port = ServiceName;
				}
			}
		}

	//Check parameter.
	//Minimum supported system of Windows Version Helpers is Windows Vista.
	#ifdef _WIN64
		if (SocketTimeout == DEFAULT_TIME_OUT && !IsWindows8OrGreater())
	#else
		if (SocketTimeout == DEFAULT_TIME_OUT && IsLowerThanWin8())
	#endif
			SocketTimeout = DEFAULT_TIME_OUT - 500;
		MinTime = SocketTimeout;

	//Convert Multi byte(s) to wide char(s).
		std::wstring wTestDomain, wTargetDomainString;
		std::shared_ptr<wchar_t> wTargetStringPTR(new wchar_t[LARGE_PACKET_MAXSIZE]());
		if (TargetString.length() > LARGE_PACKET_MAXSIZE || TargetDomainString.length() > LARGE_PACKET_MAXSIZE || TestDomain.length() > LARGE_PACKET_MAXSIZE)
		{
			wprintf(L"\nTest Domain or Target is/are too long.\n");
			return EXIT_FAILURE;
		}
		MultiByteToWideChar(CP_ACP, NULL, TargetString.c_str(), MBSTOWCS_NULLTERMINATE, wTargetStringPTR.get(), (int)TargetString.length());
		wTargetString = wTargetStringPTR.get();
		memset(wTargetStringPTR.get(), 0, sizeof(wchar_t) * LARGE_PACKET_MAXSIZE);
		MultiByteToWideChar(CP_ACP, NULL, TestDomain.c_str(), MBSTOWCS_NULLTERMINATE, wTargetStringPTR.get(), (int)TestDomain.length());
		wTestDomain = wTargetStringPTR.get();
		if (!TargetDomainString.empty())
		{
			memset(wTargetStringPTR.get(), 0, sizeof(wchar_t) * LARGE_PACKET_MAXSIZE);
			MultiByteToWideChar(CP_ACP, NULL, TargetDomainString.c_str(), MBSTOWCS_NULLTERMINATE, wTargetStringPTR.get(), (int)TargetDomainString.length());
			wTargetDomainString = wTargetStringPTR.get();
		}
		wTargetStringPTR.reset();

	//Check DNS header.
		if (HeaderParameter.Flags == 0)
			HeaderParameter.Flags = htons(DNS_STANDARD);
		if (HeaderParameter.Questions == 0)
			HeaderParameter.Questions = htons(0x0001);

	//Check DNS query.
		if (QueryParameter.Classes == 0)
			QueryParameter.Classes = htons(DNS_CLASS_IN);
		if (QueryParameter.Type == 0)
		{
			if (SockAddr.ss_family == AF_INET6) //IPv6
				QueryParameter.Type = htons(DNS_AAAA_RECORDS);
			else //IPv4
				QueryParameter.Type = htons(DNS_A_RECORDS);
		}

	//Check EDNS0 Label.
		if (DNSSEC)
			EDNS0 = true;
		if (EDNS0)
		{
			HeaderParameter.Additional = htons(0x0001);
			EDNS0Parameter.Type = htons(DNS_EDNS0_RECORDS);
			if (EDNS0PayloadSize == 0)
				EDNS0Parameter.UDPPayloadSize = htons(EDNS0_MINSIZE);
			else 
				EDNS0Parameter.UDPPayloadSize = htons((uint16_t)EDNS0PayloadSize);
			if (DNSSEC)
			{
				HeaderParameter.FlagsBits.AD = ~HeaderParameter.FlagsBits.AD; //Local DNSSEC Server validate
				HeaderParameter.FlagsBits.CD = ~HeaderParameter.FlagsBits.CD; //Client validate
				EDNS0Parameter.Z_Bits.DO = ~EDNS0Parameter.Z_Bits.DO; //Accepts DNSSEC security RRs
			}
		}

	//Output result to file.
		if (!OutputFileName.empty())
		{
			Result = _wfopen_s(&OutputFile, OutputFileName.c_str(), L"w,ccs=UTF-8");
			if (OutputFile == nullptr)
			{
				wprintf(L"Create output result file %ls error, error code is %d.\n", OutputFileName.c_str(), (int)Result);

				WSACleanup();
				return EXIT_SUCCESS;
			}
			else {
				fwprintf_s(OutputFile, L"\n");
			}
		}

	//Print to screen before sending.
		wprintf(L"\n");
		if (ReverseLookup)
		{
			if (wTargetDomainString.empty())
			{
				std::shared_ptr<char> FQDN(new char[NI_MAXHOST]());
				if (getnameinfo((PSOCKADDR)&SockAddr, sizeof(sockaddr_in), FQDN.get(), NI_MAXHOST, NULL, NULL, NI_NUMERICSERV) != 0)
				{
					wprintf(L"\nResolve addresses to host names error, error code is %d.\n", WSAGetLastError());
					wprintf(L"DNSPing %ls:%u with %ls:\n", wTargetString.c_str(), ntohs(ServiceName), wTestDomain.c_str());

				//Output to file.
					if (OutputFile != nullptr)
						fwprintf_s(OutputFile, L"DNSPing %ls:%u with %ls:\n", wTargetString.c_str(), ntohs(ServiceName), wTestDomain.c_str());
				}
				else {
					if (TargetString == FQDN.get())
					{
						wprintf(L"DNSPing %ls:%u with %ls:\n", wTargetString.c_str(), ntohs(ServiceName), wTestDomain.c_str());

					//Output to file.
						if (OutputFile != nullptr)
							fwprintf_s(OutputFile, L"DNSPing %ls:%u with %ls:\n", wTargetString.c_str(), ntohs(ServiceName), wTestDomain.c_str());
					}
					else {
						std::shared_ptr<wchar_t> wFQDN(new wchar_t[strlen(FQDN.get())]());
						MultiByteToWideChar(CP_ACP, NULL, FQDN.get(), MBSTOWCS_NULLTERMINATE, wFQDN.get(), (int)strlen(FQDN.get()));
						wprintf(L"DNSPing %ls:%u [%ls] with %ls:\n", wFQDN.get(), ntohs(ServiceName), wTargetString.c_str(), wTestDomain.c_str());

					//Output to file.
						if (OutputFile != nullptr)
							fwprintf_s(OutputFile, L"DNSPing %ls:%u [%ls] with %ls:\n", wFQDN.get(), ntohs(ServiceName), wTargetString.c_str(), wTestDomain.c_str());
					}
				}
			}
			else {
				wprintf(L"DNSPing %ls:%u [%ls] with %ls:\n", wTargetDomainString.c_str(), ntohs(ServiceName), wTargetString.c_str(), wTestDomain.c_str());

			//Output to file.
				if (OutputFile != nullptr)
					fwprintf_s(OutputFile, L"DNSPing %ls:%u [%ls] with %ls:\n", wTargetDomainString.c_str(), ntohs(ServiceName), wTargetString.c_str(), wTestDomain.c_str());
			}
		}
		else {
			if (!TargetDomainString.empty())
			{
				wprintf(L"DNSPing %ls:%u [%ls] with %ls:\n", wTargetDomainString.c_str(), ntohs(ServiceName), wTargetString.c_str(), wTestDomain.c_str());

			//Output to file.
				if (OutputFile != nullptr)
					fwprintf_s(OutputFile, L"DNSPing %ls:%u [%ls] with %ls:\n", wTargetDomainString.c_str(), ntohs(ServiceName), wTargetString.c_str(), wTestDomain.c_str());
			}
			else {
				wprintf(L"DNSPing %ls:%u with %ls:\n", wTargetString.c_str(), ntohs(ServiceName), wTestDomain.c_str());

			//Output to file.
				if (OutputFile != nullptr)
					fwprintf_s(OutputFile, L"DNSPing %ls:%u with %ls:\n", wTargetString.c_str(), ntohs(ServiceName), wTestDomain.c_str());
			}
		}

	//Send.
		if (SendNum == 0)
		{
			while (true)
			{
				if (RealSendNum <= U16_MAXNUM)
				{
					RealSendNum++;
					if (SendProcess(SockAddr) == EXIT_FAILURE)
					{
						WSACleanup();
						return EXIT_FAILURE;
					}
				}
				else {
					wprintf(L"\nStatistics is full.\n");
				//Output to file.
					if (OutputFile != nullptr)
						fwprintf_s(OutputFile, L"\nStatistics is full.\n");

					PrintProcess(true, true);
				//Close file handle.
					if (OutputFile != nullptr)
						fclose(OutputFile);

					WSACleanup();
					return EXIT_SUCCESS;
				}
			}
		}
		else {
			for (size_t Index = 0;Index < SendNum;Index++)
			{
				RealSendNum++;
				if (SendProcess(SockAddr) == EXIT_FAILURE)
				{
				//Close file handle.
					if (OutputFile != nullptr)
						fclose(OutputFile);

					WSACleanup();
					return EXIT_FAILURE;
				}
			}
		}

	//Print to screen before finished.
		PrintProcess(true, true);

	//Close file handle.
		if (OutputFile != nullptr)
			fclose(OutputFile);
	}
	else {
		PrintDescription();
	}

	WSACleanup();
	return EXIT_SUCCESS;
}
