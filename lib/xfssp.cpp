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

#include "pch.h"
#include "mockdevice.h"
#include "xfssp.h"

/*
 * @brief 
 * Sends an event with associated data to XFS.
 * @param evt - The event identifier, specifying the type of event to send.
 * @param data - Additional data associated with the event.
 * @return int 0 on success, a negative value on failure.
 */
int WFPSendEvent(int evt, int data)
{
	if (!g_wfs_event.empty())
	{
		std::map<HWND, WFS_EVENTS>::iterator it;
		for (it = g_wfs_event.begin(); it != g_wfs_event.end(); ++it)
		{
			if (((*it).second.dwEvent & SERVICE_EVENTS) == SERVICE_EVENTS
				|| ((*it).second.dwEvent & USER_EVENTS) == USER_EVENTS
				|| ((*it).second.dwEvent & SYSTEM_EVENTS) == SYSTEM_EVENTS
				|| ((*it).second.dwEvent & EXECUTE_EVENTS) == EXECUTE_EVENTS)
			{
				LPWFSRESULT lpWFSResult;
				if (WFMAllocateBuffer(sizeof(WFSRESULT), WFS_MEM_ZEROINIT, (LPVOID*)&lpWFSResult) != WFS_SUCCESS)
				{
					return WFS_ERR_INTERNAL_ERROR;
				}

				lpWFSResult->hResult = WFS_SERVICE_EVENT;
				lpWFSResult->hService = (*it).second.hService;
				lpWFSResult->RequestID = NULL;					
				lpWFSResult->lpBuffer = NULL;
				lpWFSResult->u.dwEventID = evt;

				WFMAllocateMore(sizeof(DWORD), lpWFSResult, &lpWFSResult->lpBuffer);
				LPWORD lpwLampThreshold = (LPWORD)lpWFSResult->lpBuffer;
				*lpwLampThreshold = data;

				SendMessage((*it).first, WFS_USER_EVENT, 0, (LPARAM)lpWFSResult);
			}
		}
	}

	return 0;
}

/*
 * @brief 
 * Entry point for a thread that opens device.
 * @param lpParam - A pointer to user-defined data passed to the thread.
 * @return int 0 on success, a negative value on failure.
 */
DWORD WINAPI WFPOpenProcess(LPVOID lpParam)
{
	LPWFSRESULT lpWfsResult = (LPWFSRESULT)(lpParam);
	HWND hWindowReturn = (HWND)(lpWfsResult->lpBuffer);

	if (OpenDevice(WFPSendEvent))
		lpWfsResult->hResult = WFS_ERR_DEV_NOT_READY;

	SendMessage(hWindowReturn, WFS_OPEN_COMPLETE, 0, (LPARAM)lpParam);
	return 0;
}

/*
 * @brief 
 * Processes the required versions of SPI (Service Provider Interface) and service
 * versions, returning the corresponding versions.
 * @param dwSPIVersionsRequired - The required SPI version.
 * @param dwSrvcVersionsRequired - The required service version.
 * @param lpSPIVersion - Pointer to a variable that will receive the SPI version.
 * @param lpSrvcVersion - Pointer to a variable that will receive the service version.
 *
 * @return HRESULT - WFS_SUCCESS on success, an error code on failure. Specific error codes
 *                  should be documented elsewhere in the project.
 *
 */
HRESULT ProcessVersions(const DWORD dwSPIVersionsRequired, const DWORD dwSrvcVersionsRequired, \
	LPWFSVERSION& lpSPIVersion, LPWFSVERSION& lpSrvcVersion)
{
	HRESULT hr = WFS_SUCCESS;

	if (lpSPIVersion)
	{
		memset(lpSPIVersion, 0, sizeof(WFSVERSION));
		lpSPIVersion->wVersion = 0x0000;
		lpSPIVersion->wLowVersion = 0x0000;
		lpSPIVersion->wHighVersion = 0x0000;
		memcpy(lpSPIVersion->szDescription, "XFSSP 0.00-3.30", strlen("XFSSP 0.00-3.30"));
	}

	if (lpSrvcVersion)
	{
		memset(lpSrvcVersion, 0, sizeof(WFSVERSION));
		lpSrvcVersion->wVersion = 0x0000;
		lpSrvcVersion->wLowVersion = 0x0000;
		lpSrvcVersion->wHighVersion = 0x0000;
		memcpy(lpSrvcVersion->szDescription, "XFSSP 0.00-3.30", strlen("XFSSP 0.00-3.30"));
	}

	WORD wSpi_HV = 0, wSpi_LV = 0, wSrv_HV = 0, wSrv_LV = 0;

	wSpi_HV = LOWORD(dwSPIVersionsRequired);
	wSpi_LV = HIWORD(dwSPIVersionsRequired);

	wSrv_HV = LOWORD(dwSrvcVersionsRequired);
	wSrv_LV = HIWORD(dwSrvcVersionsRequired);

	if (wSpi_HV > 0xff03)
		hr = WFS_ERR_SPI_VER_TOO_HIGH;
	else if (wSrv_HV > 0xff03)
		hr = WFS_ERR_SRVC_VER_TOO_HIGH;
	else if (wSpi_LV < 0x0001)
		hr = WFS_ERR_SPI_VER_TOO_LOW;
	else if (wSrv_LV < 0x0001)
		hr = WFS_ERR_SRVC_VER_TOO_LOW;

	return hr;

}

/*
 * @brief 
 * Opens a XFS service provider, initializing the connection
 * and specifying various parameters.
 *
 * @param hService - The service handle to be associated with the session being opened.
 * @param lpszLogicalName - Points to a null-terminated string containing the pre-defined logical name of a service. It is a 
 *	high level name such as "SYSJOURNAL1," "PASSBOOKPTR3" or "ATM02," that is used by 
 *	the XFS Manager and the Service Provider as a key to obtain the specific configuration 
 *	information they need.
 * @param hApp - The application handle to be associated with the session being opened.
 * @param lpszAppID - Pointer to a null terminated string containing the application ID; the pointer may be NULL if 
 *	the ID is not used.
 * @param dwTraceLevel - The trace level for debugging purposes.
 * @param dwTimeOut - Number of milliseconds to wait for completion.
 * @param hWnd - The window handle which is to receive the completion message for this request.
 * @param reqId - Request identification number.
 * @param hProvider - Service Provider handle supplied by the XFS Manager - used by the Service Provider to 
 *	identify itself when calling the WFMReleaseDLL function.
 * @param dwSPIVersionsRequired - Specifies the range of XFS SPI versions that the XFS Manager can support. 
 * @param lpSPIVersion - Pointer to the data structure that is to receive SPI version support information and (optionally) 
 *	other details about the SPI implementation (returned parameter).
 * @param dwSrvcVersionsRequired - Service-specific interface versions required.
 * @param lpSrvcVersion - Pointer to the service-specific interface implementation information.
 *
 * @return HRESULT - WFS_SUCCESS on success, an error code on failure. Specific error codes
 *                  should be documented elsewhere in the project.
 *
 */
HRESULT WINAPI WFPOpen(HSERVICE hService, LPSTR lpszLogicalName, HAPP hApp, LPSTR lpszAppID, DWORD dwTraceLevel, DWORD dwTimeOut, HWND hWnd, REQUESTID reqId, HPROVIDER hProvider, DWORD dwSPIVersionsRequired, LPWFSVERSION lpSPIVersion, DWORD dwSrvcVersionsRequired, LPWFSVERSION lpSrvcVersion)
{
	WINDOWINFO callWindow;
	callWindow.cbSize = sizeof(WINDOWINFO);
	if (hWnd == NULL || (!GetWindowInfo(hWnd, &callWindow) && (GetLastError() == ERROR_INVALID_HANDLE)))
	{
		return WFS_ERR_INVALID_HWND;
	}

	if (lpSPIVersion == NULL || lpSrvcVersion == NULL)
	{
		return WFS_ERR_INVALID_POINTER;
	}

	ProcessVersions(dwSPIVersionsRequired, dwSrvcVersionsRequired, lpSPIVersion, lpSrvcVersion);

	g_hProvider = hProvider;
	g_h_services[hService] = true;

	LPWFSRESULT lpWFSResult;
	if (WFMAllocateBuffer(sizeof(WFSRESULT), WFS_MEM_SHARE, (LPVOID*)&lpWFSResult) != WFS_SUCCESS)
	{
		return WFS_ERR_INTERNAL_ERROR;
	}

	lpWFSResult->RequestID = reqId;
	lpWFSResult->hService = hService;
	lpWFSResult->lpBuffer = (LPVOID)(hWnd);
	lpWFSResult->hResult = WFS_SUCCESS;

	HANDLE hOpenThread;
	DWORD dwThreadId;
	hOpenThread = CreateThread(NULL, 0, WFPOpenProcess, lpWFSResult, 0, &dwThreadId);
	return WFS_SUCCESS;
}

/*
 * @brief 
 * Entry point for a thread that closes device.
 * @param lpParam - A pointer to user-defined data passed to the thread.
 * @return int 0 on success, a negative value on failure.
 */
DWORD WINAPI WFPCloseProcess(LPVOID lpParam)
{
	LPWFSRESULT lpWfsResult = (LPWFSRESULT)(lpParam);
	HWND hWindowReturn = (HWND)(lpWfsResult);

	CloseDevice();

	SendMessage(hWindowReturn, WFS_CLOSE_COMPLETE, 0, (LPARAM)lpParam);
	return 0;
}

/*
 * @brief 
 * Closes a XFS service provider, terminating the connection.
 *
 * @param hService - Handle to the Service Provider
 * @param hWnd - The window handle which is to receive the completion message for this request.
 * @param reqId - Request identification number
 *
 * @return HRESULT - WFS_SUCCESS on success, an error code on failure. Specific error codes
 *                  should be documented elsewhere in the project.
 *
 */
HRESULT WINAPI WFPClose(HSERVICE hService, HWND hWnd, REQUESTID reqId)
{
	if (hService == NULL || g_h_services.find(hService) == g_h_services.end())
	{
		return WFS_ERR_INVALID_HSERVICE;
	}

	WaitForSingleObject(g_lock_mutex, INFINITE);
	g_lock_state = UNLOCKED;
	g_lock_hservice = NULL;
	ReleaseMutex(g_lock_mutex);

	g_h_services.erase(hService);

	LPWFSRESULT lpWFSResult;
	if (WFMAllocateBuffer(sizeof(WFSRESULT), WFS_MEM_SHARE, (LPVOID*)&lpWFSResult) != WFS_SUCCESS)
	{
		return WFS_ERR_INTERNAL_ERROR;
	}

	lpWFSResult->RequestID = reqId;
	lpWFSResult->hService = hService;
	lpWFSResult->lpBuffer = (LPVOID)(hWnd);
	lpWFSResult->hResult = WFS_SUCCESS;

	HANDLE hCloseThread;
	DWORD dwThreadId;
	hCloseThread = CreateThread(NULL, 0, WFPCloseProcess, lpWFSResult, 0, &dwThreadId);

	return WFS_SUCCESS;
}

/*
 * @brief 
 * Entry point for a thread that locks device.
 * @param lpParam - A pointer to user-defined data passed to the thread.
 * @return int 0 on success, a negative value on failure.
 */
DWORD WINAPI WFPLockProcess(LPVOID lpParam)
{
	LPWFSRESULT lpWfsResult = (LPWFSRESULT)(lpParam);
	HWND hWindowReturn = (HWND)(lpWfsResult);

	WaitForSingleObject(g_lock_mutex, INFINITE);

	g_lock_state = LOCKED;
	g_lock_hservice = lpWfsResult->hService;

	ReleaseMutex(g_lock_mutex);

	SendMessage(hWindowReturn, WFS_LOCK_COMPLETE, 0, (LPARAM)lpParam);
	return 0;
}

/*
 * @brief 
 *
 * Locks a XFS service provider, preventing access by other processes.
 *
 * @param hService - Handle to the Service Provider
 * @param dwTimeOut - Number of milliseconds to wait for completion (WFS_INDEFINITE_WAIT to specify a 
 *	request that will wait until completion).
 * @param hWnd - The window handle which is to receive the completion message for this request.
 * @param reqId - Request identification number.
 *
 * @return HRESULT - WFS_SUCCESS on success, an error code on failure. Specific error codes
 *                  should be documented elsewhere in the project.
 *
 */
HRESULT WINAPI WFPLock(HSERVICE hService, DWORD dwTimeOut, HWND hWnd, REQUESTID reqId)
{
	if (hService == NULL || g_h_services.find(hService) == g_h_services.end())
	{
		return WFS_ERR_INVALID_HSERVICE;
	}

	WaitForSingleObject(g_lock_mutex, INFINITE);

	if (g_lock_state == LOCKED || g_lock_state == LOCK_PENDING)
	{
		ReleaseMutex(g_lock_mutex);
		return WFS_ERR_LOCKED;
	}

	g_lock_state = LOCK_PENDING;

	ReleaseMutex(g_lock_mutex);

	LPWFSRESULT lpWFSResult;
	if (WFMAllocateBuffer(sizeof(WFSRESULT), WFS_MEM_SHARE, (LPVOID*)&lpWFSResult) != WFS_SUCCESS)
	{
		return WFS_ERR_INTERNAL_ERROR;
	}

	lpWFSResult->RequestID = reqId;
	lpWFSResult->hService = hService;
	lpWFSResult->lpBuffer = (LPVOID)(hWnd);
	lpWFSResult->hResult = WFS_SUCCESS;

	HANDLE hCloseThread;
	DWORD dwThreadId;
	hCloseThread = CreateThread(NULL, 0, WFPLockProcess, lpWFSResult, 0, &dwThreadId);

	return WFS_SUCCESS;
}

/*
 * @brief 
 * Entry point for a thread that unlocks device.
 * @param lpParam - A pointer to user-defined data passed to the thread.
 * @return int 0 on success, a negative value on failure.
 */
DWORD WINAPI WFPUnLockProcess(LPVOID lpParam)
{
	LPWFSRESULT lpWfsResult = (LPWFSRESULT)(lpParam);
	HWND hWindowReturn = (HWND)(lpWfsResult);

	WaitForSingleObject(g_lock_mutex, INFINITE);
	g_lock_state = UNLOCKED;
	g_lock_hservice = NULL;
	ReleaseMutex(g_lock_mutex);

	SendMessage(hWindowReturn, WFS_UNLOCK_COMPLETE, 0, (LPARAM)lpParam);
	return 0;
}

/*
 * @brief 
 *
 * Releases a service that has been locked by a previous WFPLock function.
 *
 * @param hService - Handle to the Service Provider.
 * @param hWnd - The window handle which is to receive the completion message for this request.
 * @param reqId - Request identification number.
 *
 * @return HRESULT - WFS_SUCCESS on success, an error code on failure. Specific error codes
 *                  should be documented elsewhere in the project.
 *
 */
HRESULT WINAPI WFPUnlock(HSERVICE hService, HWND hWnd, REQUESTID reqId)
{
	if (hService == NULL || g_h_services.find(hService) == g_h_services.end())
	{
		return WFS_ERR_INVALID_HSERVICE;
	}

	WaitForSingleObject(g_lock_mutex, INFINITE);
	if (g_lock_state == UNLOCKED)
	{
		ReleaseMutex(g_lock_mutex);
		return WFS_ERR_LOCKED;
	}
	ReleaseMutex(g_lock_mutex);

	LPWFSRESULT lpWFSResult;
	if (WFMAllocateBuffer(sizeof(WFSRESULT), WFS_MEM_SHARE, (LPVOID*)&lpWFSResult) != WFS_SUCCESS)
	{
		return WFS_ERR_INTERNAL_ERROR;
	}

	lpWFSResult->RequestID = reqId;
	lpWFSResult->hService = hService;
	lpWFSResult->lpBuffer = (LPVOID)(hWnd);
	lpWFSResult->hResult = WFS_SUCCESS;

	HANDLE hCloseThread;
	DWORD dwThreadId;
	hCloseThread = CreateThread(NULL, 0, WFPUnLockProcess, lpWFSResult, 0, &dwThreadId);

	return WFS_SUCCESS;
}

/*
 * @brief 
 * Entry point for a thread that registers client
 * @param lpParam - A pointer to user-defined data passed to the thread.
 * @return int 0 on success, a negative value on failure.
 */
DWORD WINAPI WFPRegisterProcess(LPVOID lpParam)
{
	LPWFSRESULT lpWfsResult = (LPWFSRESULT)(lpParam);
	HWND hWindowReturn = (HWND)(lpWfsResult->lpBuffer);

	SendMessage(hWindowReturn, WFS_REGISTER_COMPLETE, 0, (LPARAM)lpWfsResult);
	return 0;
}

/*
 * @brief 
 *
 * Registers a client event handler for a specific event class.
 *
 * @param hService - Handle to the Service Provider.
 * @param dwEventClass - The class(es) of events for which the application is registering. Specified as a set of bit masks 
 *	that can be logically ORed together.
 * @param hWndReg - The window handle which is to be registered to receive the specified messages.
 * @param hWnd - The window handle which is to receive the completion message for this request.
 * @param reqId - Request identification number.
 *
 * @return HRESULT - WFS_SUCCESS on success, an error code on failure. Specific error codes
 *                  should be documented elsewhere in the project.
 *
 */
HRESULT WINAPI WFPRegister(HSERVICE hService, DWORD dwEventClass, HWND hWndReg, HWND hWnd, REQUESTID reqId)
{
	if (hService == NULL || g_h_services.find(hService) == g_h_services.end())
		return WFS_ERR_INVALID_HSERVICE;

	if ((dwEventClass & SERVICE_EVENTS) != SERVICE_EVENTS
		&& (dwEventClass & USER_EVENTS) != USER_EVENTS
		&& (dwEventClass & SYSTEM_EVENTS) != SYSTEM_EVENTS
		&& (dwEventClass & EXECUTE_EVENTS) != EXECUTE_EVENTS)
		return WFS_ERR_USER_ERROR;

	if (g_wfs_event.find(hWndReg) != g_wfs_event.end())
	{
		g_wfs_event[hWndReg].dwEvent |= dwEventClass;
	}
	else
	{
		WFS_EVENTS wfsEvent;
		wfsEvent.hService = hService;
		wfsEvent.dwEvent = dwEventClass;
		g_wfs_event[hWndReg] = wfsEvent;
	}

	LPWFSRESULT lpWFSResult;
	if (WFMAllocateBuffer(sizeof(WFSRESULT), WFS_MEM_SHARE, (LPVOID*)&lpWFSResult) != WFS_SUCCESS)
	{
		return WFS_ERR_INTERNAL_ERROR;
	}

	lpWFSResult->RequestID = reqId;
	lpWFSResult->hService = hService;
	lpWFSResult->lpBuffer = hWndReg;
	lpWFSResult->u.dwCommandCode = dwEventClass;

	HANDLE hGetInfoThread;
	DWORD dwThreadId;
	hGetInfoThread = CreateThread(NULL, 0, WFPRegisterProcess, lpWFSResult, 0, &dwThreadId);

	return WFS_SUCCESS;
}

/*
 * @brief 
 * Entry point for a thread that deregisters client
 * @param lpParam - A pointer to user-defined data passed to the thread.
 * @return int 0 on success, a negative value on failure.
 */
DWORD WINAPI WFPDeRegisterProcess(LPVOID lpParam)
{
	LPWFSRESULT lpWfsResult = (LPWFSRESULT)(lpParam);
	HWND hWindowReturn = (HWND)(lpWfsResult->lpBuffer);
	lpWfsResult->lpBuffer = NULL;

	SendMessage(hWindowReturn, WFS_DEREGISTER_COMPLETE, 0, (LPARAM)lpWfsResult);
	return 0;
}

/*
 * @brief 
 *
 * Deregisters a client event handler for a specific event class.
 *
 * @param hService - Handle to the Service Provider.
 * @param dwEventClass - The class(es) of events for which the application is registering. Specified as a set of bit masks 
 *	that can be logically ORed together.
 * @param hWndReg - The window handle which is to be registered to receive the specified messages.
 * @param hWnd - The window handle which is to receive the completion message for this request.
 * @param reqId - Request identification number.
 *
 * @return HRESULT - WFS_SUCCESS on success, an error code on failure. Specific error codes
 *                  should be documented elsewhere in the project.
 *
 */
HRESULT WINAPI WFPDeregister(HSERVICE hService, DWORD dwEventClass, HWND hWndReg, HWND hWnd, REQUESTID reqId)
{
	if (hWndReg == NULL) {
		g_wfs_event.clear();
	}
	else if (g_wfs_event.find(hWndReg) != g_wfs_event.end())
	{
		g_wfs_event[hWndReg].dwEvent &= (~dwEventClass);

		if (g_wfs_event[hWndReg].dwEvent == 0)
			g_wfs_event.erase(hWndReg);
	}
	else
	{
		return WFS_ERR_INVALID_HWNDREG;
	}

	LPWFSRESULT lpWFSResult;
	if (WFMAllocateBuffer(sizeof(WFSRESULT), WFS_MEM_SHARE, (LPVOID*)&lpWFSResult) != WFS_SUCCESS)
	{
		return WFS_ERR_INTERNAL_ERROR;
	}

	lpWFSResult->RequestID = reqId;
	lpWFSResult->hService = hService;
	lpWFSResult->lpBuffer = hWnd;
	lpWFSResult->u.dwCommandCode = dwEventClass;

	HANDLE hGetInfoThread;
	DWORD dwThreadId;
	hGetInfoThread = CreateThread(NULL, 0, WFPDeRegisterProcess, lpWFSResult, 0, &dwThreadId);
	return WFS_SUCCESS;
}

/*
 * @brief 
 *
 * Provide informations
 *
 * @param wfs_result - Pointer to the WFSRESULT structure containing the result status.
 *
 */
void ProcessGetInfoStatus(LPWFSRESULT wfs_result)
{
	HRESULT res = WFMAllocateMore(sizeof(WFSALMSTATUS), wfs_result, &wfs_result->lpBuffer);
	if (res != WFS_SUCCESS)
	{
		wfs_result->hResult = WFS_ERR_INTERNAL_ERROR;
	}
	else
	{
		LPWFSALMSTATUS lpStatus = (LPWFSALMSTATUS)wfs_result->lpBuffer;

		lpStatus->fwDevice = WFS_ALM_DEVONLINE;
		lpStatus->bAlarmSet = FALSE;
		lpStatus->wAntiFraudModule = WFS_ALM_AFMOK;
		lpStatus->lpszExtra = NULL;
	}
}

/*
 * @brief 
 *
 * Provide capabilities
 *
 * @param wfs_result - Pointer to the WFSRESULT structure containing the result status.
 *
 */
void ProcessGetInfoCapabilities(LPWFSRESULT wfs_result)
{
	HRESULT res = WFMAllocateMore(sizeof(WFSALMCAPS), wfs_result, &wfs_result->lpBuffer);
	if (res != WFS_SUCCESS)
	{
		wfs_result->hResult = WFS_ERR_INTERNAL_ERROR;
	}
	else
	{
		LPWFSALMCAPS lpCaps = (LPWFSALMCAPS)wfs_result->lpBuffer;

		lpCaps->wClass = WFS_SERVICE_CLASS_ALM;
		lpCaps->bProgrammaticallyDeactivate = TRUE;
		lpCaps->lpszExtra = NULL;
		lpCaps->bAntiFraudModule = TRUE;
		lpCaps->lpdwSynchronizableCommands = NULL;
	}
}

/*
 * @brief 
 * Entry point for a thread that performs information retrieval.
 * @param lpParam - A pointer to user-defined data passed to the thread.
 * @return int 0 on success, a negative value on failure.
 */
DWORD WINAPI WFPGetInfoProcess(LPVOID lpParam)
{
	LPWFSRESULT lpWfsResult = (LPWFSRESULT)(lpParam);
	HWND hWindowReturn = (HWND)(lpWfsResult->lpBuffer);

	if (lpWfsResult->u.dwCommandCode == WFS_INF_ALM_CAPABILITIES)
	{
		ProcessGetInfoCapabilities(lpWfsResult);
		lpWfsResult->hResult = WFS_SUCCESS;
	}
	else if (lpWfsResult->u.dwCommandCode == WFS_INF_ALM_STATUS)
	{
		ProcessGetInfoStatus(lpWfsResult);
		lpWfsResult->hResult = WFS_SUCCESS;
	}

	SendMessage(hWindowReturn, WFS_GETINFO_COMPLETE, 0, (LPARAM)lpParam);
	return 0;
}

/*
 * @brief 
*
 * Retrieves various kinds of information from the specified Service Provider.
 *
 * @param hService - Handle to the Service Provider.
 * @param dwCategory - Specifies the category of the query (e.g. for a printer, WFS_INF_PTR_STATUS to request 
 *	status or WFS_INF_PTR_CAPABILITIES to request capabilities). The available categories 
 *	depend on the service class, the Service Provider and the service. The information requested 
 *	can be either static or dynamic, e.g. basic service capabilities (static) or current service status 
 *	(dynamic).
 * @param lpQueryDetails - Pointer to the data structure to be passed to the Service Provider, containing further details to 
 *	make the query more precise, e.g. a form name. (Many queries have no input parameters, in 
 *	which case this pointer is NULL.)
 * @param dwTimeOut - Number of milliseconds to wait for completion (WFS_INDEFINITE_WAIT to specify a 
 *	request that will wait until completion).
 * @param hWnd - The window handle which is to receive the completion message for this request.
 * @param reqId - Request identification number.
 *
 * @return HRESULT - WFS_SUCCESS on success, an error code on failure. Specific error codes
 *                  should be documented elsewhere in the project.
 *
 */
HRESULT WINAPI WFPGetInfo(HSERVICE hService, DWORD dwCategory, LPVOID lpQueryDetails, DWORD dwTimeOut, HWND hWnd, REQUESTID reqId)
{
	if (hService == NULL || g_h_services.find(hService) == g_h_services.end())
	{
		return WFS_ERR_INVALID_HSERVICE;
	}

	LPWFSRESULT lpWFSResult;
	if (WFMAllocateBuffer(sizeof(WFSRESULT), WFS_MEM_SHARE | WFS_MEM_ZEROINIT, (LPVOID*)&lpWFSResult) != WFS_SUCCESS)
	{
		return WFS_ERR_INTERNAL_ERROR;
	}

	lpWFSResult->RequestID = reqId;
	lpWFSResult->hService = hService;
	lpWFSResult->lpBuffer = (LPVOID)(hWnd);
	lpWFSResult->u.dwCommandCode = dwCategory;


	HANDLE hCloseThread;
	DWORD dwThreadId;
	hCloseThread = CreateThread(NULL, 0, WFPGetInfoProcess, lpWFSResult, 0, &dwThreadId);

	return WFS_SUCCESS;
}

/*
 * @brief 
 *
 * Executes a command to reset alarms
 *
 * @param wfs_result - Pointer to the WFSRESULT structure containing the result status.
 *
 */
void WFPExecuteResetAlarmCommand(LPWFSRESULT wfs_result)
{
	wfs_result->hResult = WFS_SUCCESS;
	if(ResetAlarm())
		wfs_result->hResult = WFS_ERR_INTERNAL_ERROR;
	
	wfs_result->lpBuffer = NULL;
}

/*
 * @brief 
 *
 * Executes a command to reset alarm command
 *
 * @param wfs_result - Pointer to the WFSRESULT structure containing the result status.
 *
 */
void WFPExecuteResetDeviceCommand(LPWFSRESULT wfs_result)
{
	wfs_result->hResult = WFS_SUCCESS;
	if (ResetDevice())
		wfs_result->hResult = WFS_ERR_INTERNAL_ERROR;

	wfs_result->lpBuffer = NULL;
}

/*
 * @brief 
 * Entry point for a thread responsible for executing SP operations.
 * @param lpParam - A pointer to user-defined data passed to the thread.
 * @return int 0 on success, a negative value on failure.
 */
DWORD WINAPI WFPExecuteThread(LPVOID lpParam)
{
	while (TRUE)
	{
		WaitForSingleObject(g_wfs_queue_mutex, INFINITE);
		if (!g_wfs_msg_queue.empty())
		{
			WFS_MSG* msg = g_wfs_msg_queue.front();
			LPWFSRESULT lpWfsResult = msg->lpWFSResult;
			HWND hWindowReturn = (HWND)msg->hWnd;
			g_wfs_msg_queue.pop_front();
			ReleaseMutex(g_wfs_queue_mutex);

			if (msg->bCancelled == TRUE)
			{
				msg->lpWFSResult->hResult = WFS_ERR_CANCELED;
			}
			else
			{
				if (lpWfsResult->u.dwCommandCode == WFS_CMD_ALM_RESET_ALARM)
				{
					WFPExecuteResetAlarmCommand(lpWfsResult);
				}
				else if (lpWfsResult->u.dwCommandCode == WFS_CMD_ALM_RESET)
				{
					WFPExecuteResetDeviceCommand(lpWfsResult);
				}
			}
			SendMessage(hWindowReturn, WFS_EXECUTE_COMPLETE, NULL, (LPARAM)lpWfsResult);

			free(msg);
		}
		else
		{
			ReleaseMutex(g_wfs_queue_mutex);
		}
		Sleep(1000L);
	}

	return 0;
}

/*
 * @brief 
 *
 * Sends asynchronous service class specific commands to a Service Provider.
 *
 * @param hService - Handle to the Service Provider.
 * @param dwCommand - Command to be executed.
 * @param lpCmdData - Pointer to the data structure to be passed.
 * @param dwTimeOut - Number of milliseconds to wait for completion (WFS_INDEFINITE_WAIT to specify a 
 *	request that will wait until completion).
 * @param hWnd - The window handle which is to receive the completion message for this request.
 * @param reqId - Request identification number.
 *
 * @return HRESULT - WFS_SUCCESS on success, an error code on failure. Specific error codes
 *                  should be documented elsewhere in the project.
 *
 */
HRESULT WINAPI WFPExecute(HSERVICE hService, DWORD dwCommand, LPVOID lpCmdData, DWORD dwTimeOut, HWND hWnd, REQUESTID reqId)
{
	if (hService == NULL || g_h_services.find(hService) == g_h_services.end())
	{
		return WFS_ERR_INVALID_HSERVICE;
	}

	WaitForSingleObject(g_lock_mutex, INFINITE);
	if (g_lock_state == LOCKED && g_lock_hservice != hService)
	{
		ReleaseMutex(g_lock_mutex);
		return WFS_ERR_LOCKED;
	}
	ReleaseMutex(g_lock_mutex);

	if (dwCommand == WFS_CMD_ALM_SYNCHRONIZE_COMMAND || dwCommand == WFS_CMD_ALM_SET_ALARM || dwCommand == WFS_CMD_ALM_RESET_ALARM)
	{
		return WFS_ERR_UNSUPP_COMMAND;
	}

	LPWFSRESULT lpWFSResult;
	if (WFMAllocateBuffer(sizeof(WFSRESULT), WFS_MEM_SHARE, (LPVOID*)&lpWFSResult) != WFS_SUCCESS)
	{
		return WFS_ERR_INTERNAL_ERROR;
	}

	lpWFSResult->RequestID = reqId;
	lpWFSResult->hService = hService;
	lpWFSResult->u.dwCommandCode = dwCommand;

	WFS_MSG* msgData = (WFS_MSG*)malloc(sizeof(WFS_MSG));
	msgData->lpWFSResult = lpWFSResult;
	msgData->hWnd = hWnd;
	msgData->bCancelled = false;
	msgData->lpDataReceived = NULL;

	DWORD dwThreadId;
	if (hExecuteThread == NULL)
	{
		hExecuteThread = CreateThread(NULL, 0, WFPExecuteThread, 0, 0, &dwThreadId);
	}

	WaitForSingleObject(g_wfs_queue_mutex, INFINITE);
	g_wfs_msg_queue.push_back(msgData);
	ReleaseMutex(g_wfs_queue_mutex);

	return WFS_SUCCESS;
}

/*
 * @brief 
 *
 * Cancels the specified (or every) asynchronous request being performed on the specified Service Provider, before its 
 *	(their) completion.
 *
 * @param hService - Handle to the Service Provider.
 * @param reqId - The request identifier (NULL to cancel all requests for the specified hService).
 *
 * @return HRESULT - WFS_SUCCESS on success, an error code on failure. Specific error codes
 *                  should be documented elsewhere in the project.
 *
 */
HRESULT WINAPI WFPCancelAsyncRequest(HSERVICE hService, REQUESTID reqId)
{
	if (hService == NULL || g_h_services.find(hService) == g_h_services.end())
	{
		return WFS_ERR_INVALID_HSERVICE;
	}

	WaitForSingleObject(g_wfs_queue_mutex, INFINITE);
	std::deque<WFS_MSG*>::iterator it;
	for (it = g_wfs_msg_queue.begin(); it != g_wfs_msg_queue.end(); ++it)
	{
		if ((*it)->lpWFSResult->hService == hService)
		{
			if ((reqId == NULL) || ((*it)->lpWFSResult->RequestID == reqId))
			{
				(*it)->bCancelled = true;
			}
		}
	}
	ReleaseMutex(g_wfs_queue_mutex);
	return WFS_SUCCESS;
}

/*
 * @brief 
 *
 * Sets the specified trace level(s) at run time, in and/or below the Service Provider.
 *
 * @param hService - Handle to the Service Provider.
 * @param dwTraceLevel - The level(s) of tracing being requested.
 *
 * @return HRESULT - WFS_SUCCESS on success, an error code on failure. Specific error codes
 *                  should be documented elsewhere in the project.
 *
 */
HRESULT WINAPI WFPSetTraceLevel(HSERVICE hService, DWORD dwTraceLevel)
{
	return WFS_SUCCESS;
}

/*
 * @brief 
 *
 * Asks the called Service Provider whether it is OK for the XFS Manager to unload the Service Provider’s DLL.
 *
 * @return HRESULT - WFS_SUCCESS on success, an error code on failure. Specific error codes
 *                  should be documented elsewhere in the project.
 *
 */
HRESULT WINAPI WFPUnloadService()
{
	return WFS_SUCCESS;
}