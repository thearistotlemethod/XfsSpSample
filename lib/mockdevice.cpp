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

static DWORD WINAPI MockDeviceAlarmLoop(LPVOID lpParam);

static eventcb cbFunc = NULL;
static HANDLE hThread = NULL;
static DWORD threadId = 0;
static HANDLE mutex = NULL;

/*
 * @brief 
 * Opens the target device for communication and sets up event callbacks.
 * @param cb 
 * @return int 0 on success, a negative value on failure.
 */
int OpenDevice(eventcb cb) {
	int rv = -1;
	if (mutex == NULL) {
		mutex = CreateMutex(NULL, FALSE, NULL);
		if (mutex == NULL) {
			return rv;
		}
	}

	if (hThread == NULL) {
		hThread = CreateThread(NULL, 0, MockDeviceAlarmLoop, 0, 0, &threadId);
	}

	Sleep(1000);
	WaitForSingleObject(mutex, INFINITE);
	cbFunc = cb;
	rv = 0;
	ReleaseMutex(mutex);
	return rv;
}

/*
 * @brief 
 * Closes the target device, terminating communication.
 * @param cb 
 * @return int 0 on success, a negative value on failure.
 */
int CloseDevice(void) {
	return 0;
}

/*
 * @brief 
 * Reset the target device
 * @param cb 
 * @return int 0 on success, a negative value on failure.
 */
int ResetDevice(void) {
	int rv = -1;
	WaitForSingleObject(mutex, INFINITE);
	
    rv = 0;

	ReleaseMutex(mutex);
	return rv;
}

/*
 * @brief 
 * Reset alarm value
 * @param cb 
 * @return int 0 on success, a negative value on failure.
 */
int ResetAlarm(void) {
	int rv = -1;
	WaitForSingleObject(mutex, INFINITE);

    rv = 0;

	ReleaseMutex(mutex);
	return rv;
}

/*
 * @brief 
 * Entry point for a thread that simulates alarm events from a mock device.
 * @param lpParam - A pointer to user-defined data passed to the thread.
 * @return int 0 on success, a negative value on failure.
 */
static DWORD WINAPI MockDeviceAlarmLoop(LPVOID lpParam) {
    int alarm = 0, data = 0;
	while (1)
	{
		Sleep(30000);
		WaitForSingleObject(mutex, INFINITE);

        if (alarm % 2) {
            if (cbFunc)
                cbFunc(WFS_SRVE_ALM_DEVICE_SET, data);
        }
        else {
            if (cbFunc)
                cbFunc(WFS_SRVE_ALM_DEVICE_RESET, data);
        }

       

        alarm++;
        data++;

		ReleaseMutex(mutex);
	}

	return 0;
}