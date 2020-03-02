#ifndef _THREATS_H__
#define _THREATS_H__
#include <string.h>
namespace ZyThreats
{
	typedef struct _tag_virusdesc
	{
		const char *VirusNamePrefix;
		const char *VirusName;
		const char *VirusDetails;
	}VIRUS_DESC, *PVIRUS_DESC;

	VIRUS_DESC VirusDescTable[] = {
		{
			"Virus",
			"virus_VIRUS",
			"virus_VIRUS_DESC",
		},
		{
			"Dropper",
			"virus_DROPPER",
			"virus_DROPPER_DESC",
		},
		{
			"Trojan-DL",
			"virus_TROJAN-DL",
			"virus_TROJAN-DL_DESC",
		},
		{
			"Downloader",
			"virus_TROJAN-DL",
			"virus_TROJAN-DL_DESC",
		},
		{
			"Harm",
			"virus_HARM",
			"virus_HARM_DESC",
		},
		{
			"PSW",
			"virus_PSW",
			"virus_PSW_DESC",
		},
		{
			"Exploit",
			"virus_EXPLOIT",
			"virus_EXPLOIT_DESC",
		},
		{
			"Risk",
			"virus_RISK",
			"virus_RISK_DESC",
		},
		{
			"HackTool",
			"virus_HACKTOOL",
			"virus_HACKTOOL_DESC",
		},
		{
			"Rootkit",
			"virus_ROOTKIT",
			"virus_ROOTKIT_DESC",
		},
		{
			"Clicker",
			"virus_CLICKER",
			"virus_CLICKER_DESC",
		},
		{
			"Adware",
			"virus_ADWARE",
			"virus_ADWARE_DESC",
		},
		{
			"Packer",
			"virus_PACKER",
			"virus_PACKER_DESC",
		},
		{
			"Joke",
			"virus_JOKE",
			"virus_JOKE_DESC",
		},
		{
			"Backdoor",
			"virus_BACKDOOR",
			"virus_BACKDOOR_DESC",
		},
		{
			"Worm",
			"virus_WORM",
			"virus_WORM_DESC",
		},
		{
			"EICAR",
			"virus_EICAR",
			"virus_EICAR_DESC",
		},
		{
			"Trojan",
			"virus_TROJAN",
			"virus_TROJAN_DESC",
		}
	};



	enum{ MAX_VIRUS_DESC_NUM = sizeof(VirusDescTable)/sizeof(VirusDescTable[0]) };

	static PVIRUS_DESC GetDescTableByVirusName(std::string virusName)
	{
		for (int i = 0; i < MAX_VIRUS_DESC_NUM; i++)
		{
			if (virusName.find(VirusDescTable[i].VirusNamePrefix) != std::string::npos)
			{
				return &VirusDescTable[i];
			}
		}
		return &VirusDescTable[MAX_VIRUS_DESC_NUM-1]; // 返回Trojan，请确保Trojan放在最后
	}
}


#endif//_THREATS_H__
