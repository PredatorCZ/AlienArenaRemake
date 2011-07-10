#include  <ctype.h>
#include  <string.h>
#include  <iostream>
#include  <fstream>
#include  <time.h>
#include  <Windows.h>

#include "accountserv.h"

using namespace std;

extern SYSTEMTIME st;

char RandomChar()
{   
	return (char)((rand() % 78) + 30);
}

void ObtainVStringForPlayer(char name[32])
{
	ifstream playerProfileFile;
	char szPath[256];
	char szTmp[256];
	int i;

	//look for existing account
	sprintf(szPath, "playerprofiles/%s", name);

	//open file
	playerProfileFile.open(szPath);

	if(!playerProfileFile)
	{
		//create randomstring
		for(i = 0; i < 32; i++)
		{
			vString[i] = RandomChar();
		}

		vString[31] = 0;
	}
	else
	{
		playerProfileFile.getline(szTmp, 256);
		playerProfileFile.getline(vString, 32);
		playerProfileFile.close();
	}

}

bool ValidatePlayer(char name[32], char password[256], char pVString[32])
{
	ifstream playerProfileFile;
	ofstream newPlayerProfileFile;
	char szPath[256];
	char svPass[256];
	char svTime[32];

	sprintf(szPath, "playerprofiles/%s", name);
	
	//open file
	playerProfileFile.open(szPath);

	//if no file, create one and return true
	if(!playerProfileFile)
	{
		printf("creating new profile for %s\n", name);

		GetSystemTime(&st);
		sprintf(svTime, "%i-%i-%i-%i", st.wYear, st.wMonth, st.wDay, st.wHour);
		newPlayerProfileFile.open(szPath);
		newPlayerProfileFile << password <<endl;
		newPlayerProfileFile << pVString <<endl;
		newPlayerProfileFile << svTime <<endl;
		newPlayerProfileFile.close();

		return true;
	}
	else
	{
		printf("reading profile for %s\n", name);

		playerProfileFile.getline(svPass, 256);
		playerProfileFile.close();

		if(!_stricmp(svPass, password))
		{
			//matched!
			return true;
		}
		else
		{
			printf("invalid password for %s!", name);
			return false;
		}
	}	

	return false;
}

void DumpValidPlayersToFile(void)
{
	ofstream	currPlayers;
	player_t	*player = &players;

	currPlayers.open("validated");
	while (player->next)
	{
		player = player->next;
		currPlayers << player->name<<endl;		
	}
	currPlayers.close();
}
