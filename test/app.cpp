/*
 *   Copyright (c) 2023 thearistotlemethod@gmail.com
 *   All rights reserved.

 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at

 *   http://www.apache.org/licenses/LICENSE-2.0

 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include"app.h"

XfsConnector::XfsConnector()
{
	hService = 0;
	cMessageWindowName.clear();
}

XfsConnector::~XfsConnector()
{
	std::cout << "~XfsConnector" << std::endl;
	DeInitXFS();
	EndWindowThread();
}

/**
 * Initialize XFS
 *
 * Initializes the Windows XFS service, preparing it for use.
 *
 * @return HRESULT - S_OK on success, an error code on failure.
 */
HRESULT XfsConnector::InitXFS()
{
	HRESULT hr = S_FALSE;
	hr = InitMessageWindow();
	if (S_OK != hr)
	{
		std::cout << "InitMessageWindow Error" << std::endl;
		return hr;
	}
	hr = OpenXFS();
	if (S_OK != hr)
	{
		std::cout << "OpenXFS Error" << std::endl;
		return hr;
	}

	return hr;
}

/**
 * Deinitialize XFS
 *
 * Deinitializes and cleans up resources related to the XFS service.
 *
 * @return HRESULT - S_OK on success, an error code on failure.
 */
HRESULT XfsConnector::DeInitXFS()
{
	HRESULT hr;

	hr = WFSUnlock(hService);
	if (hr != WFS_SUCCESS)
	{
		std::cout << "WFSUnlock Error" << std::endl;
	}
	hr = WFSDeregister(hService, SYSTEM_EVENTS | USER_EVENTS | SERVICE_EVENTS | EXECUTE_EVENTS, hWndCallerWindowHandle);
	if (hr != WFS_SUCCESS)
	{
		std::cout << "WFSDeregister Error" << std::endl;
	}
	hr = WFSClose(hService);
	if (hr != WFS_SUCCESS)
	{
		std::cout << "WFSClose Error" << std::endl;
	}
	hr = WFSCleanUp();
	if (hr != WFS_SUCCESS)
	{
		std::cout << "WFSCleanUp Error" << std::endl;
	}

	return hr;
}

/**
 * Open XFS
 *
 * Opens a connection to the XFS service.
 *
 * @return HRESULT - S_OK on success, an error code on failure.
 */
HRESULT XfsConnector::OpenXFS()
{
	HRESULT rv = 0;
	DWORD version = 0x0001ff03;
	WFSVERSION xfsVersion, srvcVersion, sPIVersion;
	REQUESTID           RequestID;
	LPREQUESTID         lpRequestID = &RequestID;
	HAPP				hApp = NULL;

	rv = WFSStartUp(version, &xfsVersion);
	if (rv != WFS_SUCCESS)
	{
		if (rv != WFS_ERR_ALREADY_STARTED)
			return rv;
	}
	else
	{
		PrintVerisonInformations("XFS", xfsVersion);		
	}

	rv = WFSCreateAppHandle(&hApp);
	if (rv != WFS_SUCCESS) {
		std::cout <<"WFSCreateAppHandle Can not create Handle" << std::endl;
		hApp = WFS_DEFAULT_HAPP;
	}

	rv = WFSAsyncOpen(const_cast<char*> (deviceLogicalName.c_str()),
		WFS_DEFAULT_HAPP,
		NULL,
		WFS_TRACE_ALL_API,
		1000000,
		&hService,
		hWndCallerWindowHandle,
		0x00030203,
		&srvcVersion,
		&sPIVersion,
		lpRequestID
	);

	if (rv != WFS_SUCCESS)
	{
		std::cout << "WFSAsyncOpen Error: " << rv << std::endl;
		return rv;
	}

	PrintVerisonInformations("SRVC", srvcVersion);
	PrintVerisonInformations("SPI", sPIVersion);

	if (hWndCallerWindowHandle)
	{
		rv = WFSAsyncRegister(hService
			, SYSTEM_EVENTS | USER_EVENTS | SERVICE_EVENTS | EXECUTE_EVENTS
			, hWndCallerWindowHandle, hWndCallerWindowHandle, lpRequestID);
		
		if (rv != WFS_SUCCESS)
		{				
			std::cout << "WFSRegister Error:" << rv << std::endl;
			return rv;
		}
	}
	else {
		std::cout << "Error: caller_window_handle is null" << std::endl;
		return S_FALSE;
	}

	return rv;
}

/**
 * Get Service Status
 *
 * Retrieves and prints the status of the XFS service.
 *
 * @return HRESULT - S_OK on success, an error code on failure.
 */
HRESULT XfsConnector::GetStatus()
{
	HRESULT hr, hrLog = 0;
	WFSRESULT* pResult = 0;
	REQUESTID           RequestID;
	LPREQUESTID         lpRequestID = &RequestID;

	LPWFSALMSTATUS lpStatus;

	hr = WFSAsyncGetInfo(hService, WFS_INF_ALM_STATUS, NULL, 400000, hWndCallerWindowHandle, lpRequestID);
	if (hr != WFS_SUCCESS)
	{
		std::cout <<"WFSGetInfo WFS_INF_ALM_STATUS Error" << std::endl;
	}

	return hr;
}

/**
 * Get Service Capabilities
 *
 * Retrieves and prints the capabilities of the XFS service.
 *
 * @return HRESULT - S_OK on success, an error code on failure.
 */
HRESULT XfsConnector::GetCapabilities()
{
	HRESULT hr, hrLog = 0;
	WFSRESULT* pResult = 0;
	REQUESTID           RequestID;
	LPREQUESTID         lpRequestID = &RequestID;

	LPWFSALMSTATUS lpStatus;

	hr = WFSAsyncGetInfo(hService, WFS_INF_ALM_CAPABILITIES, NULL, 400000, hWndCallerWindowHandle, lpRequestID);
	if (hr != WFS_SUCCESS)
	{
		std::cout << "WFSGetInfo WFS_INF_ALM_CAPABILITIES Error" << std::endl;
	}

	return hr;
}

/**
 * Cancel Asynchronous Request
 *
 * Cancels an asynchronous request initiated on the XFS service.
 *
 * @param RequestID - The request identifier to be canceled.
 * @return HRESULT - S_OK on success, an error code on failure.
 */
HRESULT XfsConnector::CancelAsyncReq(REQUESTID RequestID)
{
	HRESULT ret;

	std::cout <<"CancelAsyncReq Started..." << std::endl;
	ret = WFSCancelAsyncRequest(hService, RequestID);
	return ret;
}

/**
 * Initialize Message Window
 *
 * Initializes a message window for handling XFS service events.
 *
 * @return HRESULT - S_OK on success, an error code on failure.
 */
HRESULT XfsConnector::InitMessageWindow()
{
	std::cout << "Message Window For " << deviceLogicalName.c_str() << std::endl;

	hWindowCreatedEvent = CreateEvent(
		NULL,
		TRUE,
		FALSE,
		TEXT("WindowCreatedEvent")
	);
	if (hWindowCreatedEvent == NULL)
	{
		std::cout <<"CreateEvent failed" << std::endl;
		return S_FALSE;
	}
	CreateWindowThread();
	Sleep(1000);

	DWORD dwWaitResult;
	dwWaitResult = WaitForSingleObject(
		hWindowCreatedEvent,
		INFINITE);

	hWndCallerWindowHandle = FindWindowA(NULL, cMessageWindowName.c_str());	
	return S_OK;
}

/**
 * Lock Service
 *
 * Locks the XFS service, preventing access by other processes.
 *
 * @param pResult - Pointer to a WFSRESULT structure.
 * @return HRESULT - S_OK on success, an error code on failure.
 */
HRESULT XfsConnector::Lock(WFSRESULT* pResult)
{
	HRESULT hRetVal;

	hRetVal = WFSLock(hService, 10000, &pResult);
	if (hRetVal != WFS_SUCCESS)
	{
		hRetVal = WFSFreeResult(pResult);
		hRetVal = WFSUnlock(hService);
		return hRetVal;
	}
	hRetVal = WFSFreeResult(pResult);
	if (hRetVal != WFS_SUCCESS)
	{
		WFSUnlock(hService);
		return  hRetVal;
	}

	return hRetVal;
}

/**
 * Unlock Service
 *
 * Unlocks the XFS service, allowing access by other processes.
 *
 * @param pResult - Pointer to a WFSRESULT structure.
 * @return HRESULT - S_OK on success, an error code on failure.
 */
HRESULT XfsConnector::UnLock(WFSRESULT* pResult)
{
	HRESULT hRetVal;

	hRetVal = WFSFreeResult(pResult);

	if (hRetVal != WFS_SUCCESS)
	{
		std::cout << "Unlock Error" << std::endl;
	}

	hRetVal = WFSUnlock(hService);
	if (hRetVal != WFS_SUCCESS)
	{
		return hRetVal;
	}

	return hRetVal;
}

/**
 * Create Window Thread
 *
 * Creates a thread responsible for managing the message window.
 */
void XfsConnector::CreateWindowThread()
{
	MessageWindowThread = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)WindowThreadFunction,
		this,
		0,
		&MessageWindowThreadId);


	if (MessageWindowThread == NULL)
	{
		std::cout <<"MessageWindow thread Error" << std::endl;
		ExitProcess(3);
	}
}

/**
 * End Window Thread
 *
 * Ends and cleans up the message window thread.
 */
void XfsConnector::EndWindowThread()
{
	CloseHandle(MessageWindowThread);
	CloseHandle(hWindowCreatedEvent);
}

/**
 * Create Message Window
 *
 * Creates and registers a message window for handling XFS service events.
 *
 * @return HWND - The handle of the created message window.
 */
HWND XfsConnector::CreateMessageWindow()
{
	char cConsoleTitle[512];
	char cConsoleClass[512];
	LPCSTR lConsoleTitle;
	LPCSTR lConsoleClass;

	GetConsoleTitleA(cConsoleTitle, sizeof(cConsoleTitle));
	FindWindowA(NULL, cConsoleTitle);

	cMessageWindowName = "WINDOW.%s" + deviceLogicalName;
	sprintf_s(cConsoleClass, "CLASS.%s", deviceLogicalName.c_str());
	lConsoleClass = cConsoleClass;

	HINSTANCE hInstC = GetModuleHandle(0);
	WNDCLASS wc = { 0 };
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.hCursor = LoadCursor(hInstC, IDC_ARROW);
	wc.hIcon = LoadIcon(hInstC, IDI_APPLICATION);
	wc.hInstance = hInstC;
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = lConsoleClass;
	wc.style = CS_HREDRAW | CS_VREDRAW;

	if (!RegisterClass(&wc))
	{
		std::cout <<"RegisterClass Error" << std::endl;
		return NULL;
	}

	HWND hwndWindow = CreateWindow(lConsoleClass,
		cMessageWindowName.c_str(),
		WS_OVERLAPPEDWINDOW,
		520, 20, 300, 300,
		HWND_MESSAGE,
		NULL,
		hInstC, NULL);

	if (!SetEvent(hWindowCreatedEvent))
	{
		std::cout <<"SetEvent Error" << std::endl;
		return NULL;
	}

	hWndCallerWindowHandle = FindWindowA(NULL, cMessageWindowName.c_str());
	SetWindowLongPtr(hwndWindow, GWLP_USERDATA, (LONG_PTR)this);
	ShowWindow(hwndWindow, SW_SHOWNORMAL);
	UpdateWindow(hwndWindow);
	MSG msg;
	while (GetMessage(&msg, hwndWindow, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return hwndWindow;
}

/**
 * Message Procedure - Event Handling
 *
 * Handles messages related to XFS service events.
 *
 * @param hWnd - The window handle.
 * @param Msg - The message identifier.
 * @param wParam - Additional message information.
 * @param lParam - Additional message information.
 * @return LRESULT - The result of the message processing.
 */
LRESULT XfsConnector::MsgProcEventHandle(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	LPWFSRESULT lpWFSResult = NULL;

	WFMAllocateBuffer(sizeof(WFSRESULT), WFS_MEM_ZEROINIT, (void**)&lpWFSResult);
	lpWFSResult = (WFSRESULT*)lParam;

	if (lpWFSResult && lpWFSResult->u.dwEventID)
	{
		LPWORD lpSource = (LPWORD)lpWFSResult->lpBuffer;
		switch (lpWFSResult->u.dwEventID)
		{
		case WFS_SRVE_ALM_DEVICE_RESET:
			std::cout << "msg: " << Msg << " Event: WFS_SRVE_ALM_DEVICE_RESET" << "\tData: " << *lpSource << std::endl;
			break;
		case WFS_SRVE_ALM_DEVICE_SET:
			std::cout << "msg: " << Msg << " Event: WFS_SRVE_ALM_DEVICE_SET" << "\tData: " << *lpSource << std::endl;
			break;
		default:
			std::cout << "msg: " << Msg << " Event: " << lpWFSResult->u.dwEventID << "\tData: " << *lpSource << std::endl;
			break;
		}

	}

	WFMFreeBuffer((void**)lpWFSResult);
	lpWFSResult = NULL;

	return lParam;
}

/**
 * Message Procedure - Asynchronous Function Return Handling
 *
 * Handles messages related to asynchronous function return events.
 *
 * @param hWnd - The window handle.
 * @param Msg - The message identifier.
 * @param wParam - Additional message information.
 * @param lParam - Additional message information.
 * @return LRESULT - The result of the message processing.
 */
LRESULT XfsConnector::MsgProcAsyncFunctionReturn(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	LPWFSRESULT lpWFSResult = NULL;

	WFMAllocateBuffer(sizeof(WFSRESULT), WFS_MEM_ZEROINIT, (void**)&lpWFSResult);
	lpWFSResult = (WFSRESULT*)lParam;

	if (Msg == WFS_EXECUTE_COMPLETE)
	{
		switch (lpWFSResult->u.dwEventID)
		{
		case WFS_CMD_ALM_SET_ALARM:
			std::cout << " msg: WFS_CMD_ALM_SET_ALARM function returned" << std::endl;
			break;
		case WFS_CMD_ALM_RESET_ALARM:
			std::cout << " msg: WFS_CMD_ALM_RESET_ALARM function returned" << std::endl;
			break;
		case WFS_CMD_ALM_RESET:
			std::cout << " msg: WFS_CMD_ALM_RESET function returned" << std::endl;
			break;
		default:
			break;
		}
	}
	else if (Msg == WFS_GETINFO_COMPLETE)
	{
		switch (lpWFSResult->u.dwEventID)
		{
		case WFS_INF_ALM_STATUS:
			std::cout << "msg: WFS_INF_ALM_STATUS function returned" << std::endl;
			PrintAlarmStatus((LPWFSALMSTATUS)lpWFSResult->lpBuffer);
			break;
		case WFS_INF_ALM_CAPABILITIES:
			std::cout << "msg: WFS_INF_ALM_CAPABILITIES function returned" << std::endl;
			PrintCapabilities((LPWFSALMCAPS)lpWFSResult->lpBuffer);
			break;
		default:
			break;
		}
	}
	else if (Msg == WFS_OPEN_COMPLETE)
	{
		std::cout << "msg: WFS_OPEN_COMPLETE" << std::endl;
	}
	else if (Msg == WFS_REGISTER_COMPLETE)
	{
		std::cout << "msg: WFS_REGISTER_COMPLETE" << std::endl;
	}
	return lParam;
}

/**
 * Print Version Information
 *
 * Prints version information to the console.
 *
 * @param label - A label or description for the version information.
 * @param version - The WFSVERSION structure containing version details.
 */
void XfsConnector::PrintVerisonInformations(const std::string& label, WFSVERSION& versionInfo) {
	std::cout << label << ":\n\tVersion: " << std::hex << versionInfo.wVersion << versionInfo.wHighVersion << versionInfo.wLowVersion << std::endl;
	std::cout << "\tDescription: " << versionInfo.szDescription << std::endl;
	std::cout << "\tSystem Status: " << std::hex << versionInfo.szSystemStatus << std::endl;
}

/**
 * Print Alarm Status
 *
 * Prints alarm status to the console.
 *
 * @param lpStatus - Pointer to a WFSALMSTATUS structure containing alarm status.
 */
void XfsConnector::PrintAlarmStatus(LPWFSALMSTATUS lpStatus) {
	std::cout << "STATUS" << ":\n\tDevice: " << std::hex << lpStatus->fwDevice << std::endl;
	std::cout << "\tAlarmSet: " << lpStatus->bAlarmSet << std::endl;
	std::cout << "\tAntiFraudModule: " << std::hex << lpStatus->wAntiFraudModule << std::endl;
}

/**
 * Print Capabilities
 *
 * Prints service capabilities to the console.
 *
 * @param lpCaps - Pointer to a WFSALMCAPS structure containing service capabilities.
 */
void XfsConnector::PrintCapabilities(LPWFSALMCAPS lpCaps) {
	std::cout << "CAPABILITIES" << ":\n\tClass: " << std::hex << lpCaps->wClass << std::endl;
	std::cout << "\tProgrammaticallyDeactivate: " << lpCaps->bProgrammaticallyDeactivate << std::endl;
	std::cout << "\tAntiFraudModule: " << std::hex << lpCaps->bAntiFraudModule << std::endl;
}

/**
 * Thread Function
 *
 * Entry point for a thread responsible for executing XFS operations.
 *
 * @param lpParam - A pointer to user-defined data passed to the thread.
 * @return DWORD - The thread exit code. Typically, this function never returns
 *                explicitly, as it represents an ongoing operation execution loop.
 */
DWORD WINAPI XfsConnector::ThreadFunction(LPVOID lpParam)
{
	HANDLE hStdout;
	HWND childWindow;

	childWindow = CreateMessageWindow();

	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdout == INVALID_HANDLE_VALUE)
		return 1;

	return 0;
}

/**
 * Window Procedure
 *
 * Handles messages sent to the message window.
 *
 * @param hWnd - The window handle.
 * @param Msg - The message identifier.
 * @param wParam - Additional message information.
 * @param lParam - Additional message information.
 * @return LRESULT - The result of the message processing.
 */
LRESULT CALLBACK XfsConnector::WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	XfsConnector* obj = NULL;
	obj = (XfsConnector*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (obj == NULL)
		return DefWindowProc(hWnd, Msg, wParam, lParam);
	else
	{
		if (Msg == WFS_EXECUTE_EVENT ||
			Msg == WFS_SERVICE_EVENT ||
			Msg == WFS_USER_EVENT ||
			Msg == WFS_SYSTEM_EVENT)
			return obj->MsgProcEventHandle(hWnd, Msg, wParam, lParam);
		else if (Msg == WFS_OPEN_COMPLETE ||
			Msg == WFS_CLOSE_COMPLETE ||
			Msg == WFS_LOCK_COMPLETE ||
			Msg == WFS_UNLOCK_COMPLETE ||
			Msg == WFS_REGISTER_COMPLETE ||
			Msg == WFS_DEREGISTER_COMPLETE ||
			Msg == WFS_GETINFO_COMPLETE ||
			Msg == WFS_EXECUTE_COMPLETE)
			return obj->MsgProcAsyncFunctionReturn(hWnd, Msg, wParam, lParam);
	}
}

/**
 * WindowThreadFunction Method
 *
 * Entry point for the thread responsible for managing the message window used for handling
 * XFS service events.
 *
 * @param param - A pointer to user-defined data passed to the thread.
 *
 * @return DWORD - The thread exit code. Typically, this function never returns explicitly,
 *                as it represents an ongoing message window management loop.
 *
 */
DWORD WINAPI XfsConnector::WindowThreadFunction(LPVOID* param)
{
	XfsConnector* obj = (XfsConnector*)(param);
	obj->ThreadFunction(param);
	return 0;

}


