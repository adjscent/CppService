#include "EnumProcess.h"


EnumProcess::EnumProcess()
{
}


EnumProcess::~EnumProcess()
{
}


#define MAX_NAME 256

BOOL GetLogonFromToken(HANDLE hToken, std::string& strUser, std::string& strdomain)
{
	DWORD dwSize = MAX_NAME;
	BOOL bSuccess = FALSE;
	DWORD dwLength = 0;
	PTOKEN_USER ptu = nullptr;
	//Verify the parameter passed in is not NULL.
	if (nullptr == hToken)
		goto Cleanup;

	if (!GetTokenInformation(
		hToken, // handle to the access token
		TokenUser, // get information about the token's groups 
		(LPVOID)ptu, // pointer to PTOKEN_USER buffer
		0, // size of buffer
		&dwLength // receives required buffer size
	))
	{
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			goto Cleanup;

		ptu = (PTOKEN_USER)HeapAlloc(GetProcessHeap(),
		                             HEAP_ZERO_MEMORY, dwLength);

		if (ptu == nullptr)
			goto Cleanup;
	}

	if (!GetTokenInformation(
		hToken, // handle to the access token
		TokenUser, // get information about the token's groups 
		(LPVOID)ptu, // pointer to PTOKEN_USER buffer
		dwLength, // size of buffer
		&dwLength // receives required buffer size
	))
	{
		goto Cleanup;
	}
	SID_NAME_USE SidType;
	char lpName[MAX_NAME];
	char lpDomain[MAX_NAME];

	if (!LookupAccountSidA(nullptr, ptu->User.Sid, lpName, &dwSize, lpDomain, &dwSize, &SidType))
	{
		DWORD dwResult = GetLastError();
		if (dwResult == ERROR_NONE_MAPPED)
			strcpy_s(lpName, "NONE_MAPPED");
		else
		{
			printf("LookupAccountSid Error %u\n", GetLastError());
		}
	}
	else
	{
		printf("Current user is  %s\\%s\n",
		       lpDomain, lpName);
		strUser = lpName;
		strdomain = lpDomain;
		bSuccess = TRUE;
	}

Cleanup:

	if (ptu != nullptr)
		HeapFree(GetProcessHeap(), 0, (LPVOID)ptu);
	return bSuccess;
}


HRESULT GetUserFromProcess(const DWORD procId, std::string& strUser, std::string& strdomain)
{
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, PROCESS_VM_READ, procId);
	if (hProcess == nullptr)
		return E_FAIL;
	HANDLE hToken = nullptr;

	if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken))
	{
		CloseHandle(hProcess);
		return E_FAIL;
	}
	BOOL bres = GetLogonFromToken(hToken, strUser, strdomain);

	CloseHandle(hToken);
	CloseHandle(hProcess);
	return bres ? S_OK : E_FAIL;
}

std::set<std::string> EnumProcess::EnumProcessForUser()
{
	std::set<std::string> temp;

	// Get the list of process identifiers.

	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;

	if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
	{
		return temp;
	}


	// Calculate how many process identifiers were returned.

	cProcesses = cbNeeded / sizeof(DWORD);

	// Print the name and process identifier for each process.

	for (i = 0; i < cProcesses; i++)
	{
		if (aProcesses[i] != 0)
		{
			std::string a;
			std::string b;
			GetUserFromProcess(aProcesses[i], a, b);
			temp.insert(a);
		}
	}

	return temp;
}
