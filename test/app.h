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

#pragma once

#include<windows.h>
#include<iostream>

#include "XFSADMIN.H"
#include "XFSAPI.H"
#include "XFSALM.H"

 /**
  * XfsConnector Class
  *
  * A class responsible for managing communication and interaction with a Windows XFS
  * (Extension for Financial Services) service. It provides functions for initialization,
  * status retrieval, capabilities, opening the service, asynchronous request handling,
  * and message window creation.
  *
  * @remarks
  * - This class encapsulates the logic for managing a Windows XFS service, including
  *   its initialization, message window creation, event handling, and asynchronous request handling.
  * - It maintains a connection to a Windows XFS service and handles various operations related
  *   to the service.
  * - The class provides both public and private methods, where public methods are intended
  *   for external use, while private methods are for internal implementation details.
  */
class XfsConnector
{
private:
	HSERVICE			hService; // Service handle
	HWND				hWndCallerWindowHandle = 0; // Caller window handle
	HANDLE				MessageWindowThread = 0; // Thread for message window
	HANDLE				hWindowCreatedEvent = 0; // Event indicating message window creation
	DWORD				MessageWindowThreadId; // Thread ID of the message window

	const std::string deviceLogicalName = "MOCKDEVICE"; // Default device logical name
	std::string cMessageWindowName; // Message window name

public:
	// Constructors and Destructors
	XfsConnector(); // Constructor
	~XfsConnector(); // Destructor

public:
	// Public Methods
	HRESULT InitXFS(); // Initialize XFS
	HRESULT DeInitXFS(); // Deinitialize XFS
	HRESULT OpenXFS(); // Open the XFS service
	HRESULT GetStatus(); // Get service status
	HRESULT GetCapabilities(); // Get service capabilities	
	HRESULT CancelAsyncReq(REQUESTID); // Cancel an asynchronous request
	HRESULT InitMessageWindow(); // Initialize the message window
	HRESULT Lock(WFSRESULT*); // Lock the service
	HRESULT UnLock(WFSRESULT*); // Unlock the service

private:
	// Private Methods
	void CreateWindowThread(); // Create a window thread
	void EndWindowThread(); // End the window thread
	HWND CreateMessageWindow(); // Create a message window
	LRESULT MsgProcEventHandle(HWND, UINT, WPARAM, LPARAM); // Process event handling messages
	LRESULT MsgProcAsyncFunctionReturn(HWND, UINT, WPARAM, LPARAM); // Process asynchronous function return messages
	void PrintVerisonInformations(const std::string&, WFSVERSION&); // Print version information
	void PrintAlarmStatus(LPWFSALMSTATUS); // Print alarm status
	void PrintCapabilities(LPWFSALMCAPS); // Print capabilities

public:
	// Public Static Methods
	DWORD WINAPI ThreadFunction(LPVOID); // Thread entry function
	static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM); // Window procedure
	static DWORD WINAPI WindowThreadFunction(LPVOID*);
};