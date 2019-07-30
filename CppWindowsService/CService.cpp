/****************************** Module Header ******************************\
* Module Name:  SampleService.cpp
* Project:      CppWindowsService
* Copyright (c) Microsoft Corporation.
* 
* Provides a sample service class that derives from the service base class - 
* CServiceBase. The sample service logs the service start and stop 
* information to the Application event log, and shows how to run the main 
* function of the service in a thread pool worker thread.
* 
* This source is subject to the Microsoft Public License.
* See http://www.microsoft.com/en-us/openness/resources/licenses.aspx#MPL.
* All other rights reserved.
* 
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, 
* EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED 
* WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#pragma region Includes
#include "CService.h"
#include "ThreadPool.h"
#pragma endregion

#include <cpr/cpr.h>
#include <Lmcons.h>
#include <comdef.h>
#include <stdio.h>
#include <psapi.h>
#include <windows.h>

#include <string>
#include <fstream>
#include <sstream>
#include <set>
#include <nlohmann/json.hpp>
#include "EnumProcess.h"

// for convenience
using json = nlohmann::json;

CService::CService(PWSTR pszServiceName,
                   BOOL fCanStop,
                   BOOL fCanShutdown,
                   BOOL fCanPauseContinue)
	: CServiceBase(pszServiceName, fCanStop, fCanShutdown, fCanPauseContinue)
{
	m_fStopping = FALSE;

	// Create a manual-reset event that is not signaled at first to indicate 
	// the stopped signal of the service.
	m_hStoppedEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	if (m_hStoppedEvent == nullptr)
	{
		throw GetLastError();
	}
}


CService::~CService(void)
{
	if (m_hStoppedEvent)
	{
		CloseHandle(m_hStoppedEvent);
		m_hStoppedEvent = nullptr;
	}
}


std::string getComputerName()
{
#define INFO_BUFFER_SIZE 32767
	CHAR infoBuf[INFO_BUFFER_SIZE];
	DWORD bufCharCount = INFO_BUFFER_SIZE;

	// Get and display the name of the computer.
	if (!GetComputerNameA(infoBuf, &bufCharCount))
		printf("Error: GetComputerName");

	return std::string(infoBuf);
}

// Get Exe path
std::string ExePath()
{
	char buffer[MAX_PATH];
	GetModuleFileNameA(nullptr, buffer, MAX_PATH);
	std::string::size_type pos = std::string(buffer).find_last_of("\\/");
	return std::string(buffer).substr(0, pos);
}

void CService::OnSessionChange()
{
	// Read config file
	std::ifstream inFile;
	inFile.open(ExePath() + "\\" + "/config.txt"); //open the input file
	std::stringstream strStream;
	strStream << inFile.rdbuf(); //read the file
	std::string str = strStream.str(); //str holds the content of the file

	// Construct Rest Url
	// std::string str = "localhost:8000";
	std::string callurl = "http://" + str + "/cc/ping";

	std::ofstream ofFile;
	ofFile.open(ExePath() + "\\" + "/file.txt"); //open the input file
	ofFile << "session change" << std::endl;
	ofFile.close();
	json j;
	j["computer"] = getComputerName();
	j["username"] = EnumProcess::EnumProcessForUser();

	auto r = Post(cpr::Url{callurl},
	              cpr::Body{j.dump()},
	              cpr::Header{{"Content-Type", "application/json"}}, cpr::Timeout{2 * 1000});
}

//
//   FUNCTION: CService::OnStart(DWORD, LPWSTR *)
//
//   PURPOSE: The function is executed when a Start command is sent to the 
//   service by the SCM or when the operating system starts (for a service 
//   that starts automatically). It specifies actions to take when the 
//   service starts. In this code sample, OnStart logs a service-start 
//   message to the Application log, and queues the main service function for 
//   execution in a thread pool worker thread.
//
//   PARAMETERS:
//   * dwArgc   - number of command line arguments
//   * lpszArgv - array of command line arguments
//
//   NOTE: A service application is designed to be long running. Therefore, 
//   it usually polls or monitors something in the system. The monitoring is 
//   set up in the OnStart method. However, OnStart does not actually do the 
//   monitoring. The OnStart method must return to the operating system after 
//   the service's operation has begun. It must not loop forever or block. To 
//   set up a simple monitoring mechanism, one general solution is to create 
//   a timer in OnStart. The timer would then raise events in your code 
//   periodically, at which time your service could do its monitoring. The 
//   other solution is to spawn a new thread to perform the main service 
//   functions, which is demonstrated in this code sample.
//
void CService::OnStart(DWORD dwArgc, LPWSTR* lpszArgv)
{
	// Log a service start message to the Application log.
	WriteEventLogEntry(L"CppWindowsService in OnStart",
	                   EVENTLOG_INFORMATION_TYPE);
	// Queue the main service function for execution in a worker thread.
	CThreadPool::QueueUserWorkItem(&CService::ServiceWorkerThread, this);
}

//
//   FUNCTION: CService::ServiceWorkerThread(void)
//
//   PURPOSE: The method performs the main function of the service. It runs 
//   on a thread pool worker thread.
//
void CService::ServiceWorkerThread(void)
{
	// std::ofstream ofFile;
	// ofFile.open(ExePath() + "\\" + "/status.txt"); //open the input file
	// ofFile << "start" << std::endl;
	// ofFile.close();

	// Periodically check if the service is stopping.
	while (!m_fStopping)
	{
		// just sleep since we are using callbacks
		Sleep(1000);

		// Perform main service function here...
		// // prepare json data for sending
		// json j;
		// j["computer"] = getComputerName();
		// j["username"] = EnumProcessForUser();
		//
		// auto r = cpr::Post(cpr::Url{ callurl },
		// 	cpr::Body{ j.dump() },
		// 	cpr::Header{ {"Content-Type", "application/json"} }, cpr::Timeout{ 2 * 1000 });
		//
		// Sleep(30 * 1000); // run every 30 seconds
	}

	// Signal the stopped event.
	SetEvent(m_hStoppedEvent);
}

//
//   FUNCTION: CService::OnStop(void)
//
//   PURPOSE: The function is executed when a Stop command is sent to the 
//   service by SCM. It specifies actions to take when a service stops 
//   running. In this code sample, OnStop logs a service-stop message to the 
//   Application log, and waits for the finish of the main service function.
//
//   COMMENTS:
//   Be sure to periodically call ReportServiceStatus() with 
//   SERVICE_STOP_PENDING if the procedure is going to take long time. 
//
void CService::OnStop()
{
	// Log a service stop message to the Application log.
	WriteEventLogEntry(L"CppWindowsService in OnStop",
	                   EVENTLOG_INFORMATION_TYPE);

	// Indicate that the service is stopping and wait for the finish of the 
	// main service function (ServiceWorkerThread).
	m_fStopping = TRUE;
	if (WaitForSingleObject(m_hStoppedEvent, INFINITE) != WAIT_OBJECT_0)
	{
		throw GetLastError();
	}
}
