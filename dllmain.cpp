// dllmain.cpp : Defines the entry point for the DLL application.
#ifdef _DEBUG
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE
#include <shlobj.h>
#include <Shlwapi.h>
#include <filesystem>
#include "CLCLPlugin.h"
#include "string.h"
#include "memory.h"
#include "winwrappers.h"
#ifndef IDD_PYTHON0
#include "resource.h"
#endif

using namespace std;

/* Define */
#define	INI_FILE_NAME	TEXT("tool_python.ini")

/* Global Variables */
HINSTANCE hInst;
TCHAR app_path[MAX_PATH];
wstring ini_path;

TOOL_EXEC_INFO* dlg_tei = nullptr;

static int init_count = 0;

// dialog variables:
wstring edit_module_name;
wstring edit_function_name;

/* Local Function Prototypes */
static BOOL dll_initialize(void);
static BOOL dll_uninitialize(void);

static int run_python(DATA_INFO* di, const TCHAR* str_script, const TCHAR* str_func);
static void fill_python_functions(wndcombobox& cb, const wstring module_name);
static BOOL CALLBACK dlg_python0(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/*
 * DllMain 
 */
int WINAPI DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		hInst = hModule;
		dll_initialize();
		break;

    case DLL_PROCESS_DETACH:
		if (lpReserved) {
			// Complete process will end. 
		}
		// Unfortunately, if we call Py_FinalizeEx() here, we get a crash when 
		// the dll is loaded for the second time.
		//int err = Py_FinalizeEx();
		dll_uninitialize();
		break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

/*
 * get_tool_info_w
 * Get tool information
 *
 *	argument:
 *		hWnd - the calling window
 *		index - the index of the acquisition (from 0)
 *		tgi - tool retrieval information
 *
 *	Return value:
 *		TRUE - has tools to get next
 *		FALSE - end of acquisition
 */
__declspec(dllexport) BOOL CALLBACK get_tool_info_w(const HWND hWnd, const int index, TOOL_GET_INFO* tgi)
{
	switch (index) {
	default: return FALSE;
	case 0:
		LoadString(hInst, IDS_PY_FUNC_TEXT, tgi->title, BUF_SIZE - 1);
		lstrcpy(tgi->func_name, TEXT("cb_python0"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_MENU | CALLTYPE_MENU_COPY_PASTE | CALLTYPE_VIEWER;
		return TRUE;
	}

	return FALSE;
}

/*
 * dll_initialize
 */
static BOOL dll_initialize(void)
{
	//TCHAR app_path[MAX_PATH];
	TCHAR* p, * r;

	// Handle NULL instead of hInst indicates to look 
	// for the path of the exe instead of the current dll.
	GetModuleFileName(NULL, app_path, MAX_PATH - 1);
	for (p = r = app_path; *p != TEXT('\0'); p++) {
#ifndef UNICODE
		if (IsDBCSLeadByte((BYTE)*p) == TRUE) {
			p++;
			continue;
		}
#endif	// UNICODE
		if (*p == TEXT('\\') || *p == TEXT('/')) {
			r = p;
		}
	}
	*r = TEXT('\0');

	UINT portable = 0;
	wstring clcl_ini_path = ::string_format(TEXT("%s\\%s"), app_path, TEXT("clcl_app.ini"));
	wstring ini_dir;
	if (::PathFileExists(clcl_ini_path.c_str()) == TRUE) {
		portable = ::GetPrivateProfileInt(TEXT("GENERAL"), TEXT("portable"), 0, clcl_ini_path.c_str());
	}
	if (portable == 1) {
		// locate tool_clip.ini in the same folder as clcl.ini besides clcl.exe
		ini_path = ::string_format(TEXT("%s\\%s"), app_path, INI_FILE_NAME);
		ini_dir.assign(app_path);
	}
	else {
		// For release mode (not portable app) set ini_path to the same folder as %LOCALAPPDATA%\CLCL\clcl.ini
		// so that we have write access.
		// See C:\home\wilf\Projects\Clcl.cvs\main.c line 2234 ff
		TCHAR local_app_data[MAX_PATH];
		if (SUCCEEDED(::SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, local_app_data))) {
			ini_dir = ::string_format(TEXT("%s\\CLCL"), local_app_data);
			ini_path = ::string_format(TEXT("%s\\%s"), ini_dir.c_str(), INI_FILE_NAME);
			::CreateDirectory(ini_dir.c_str(), NULL);
		}
	}

	if (ini_path.empty())
		return FALSE;

	// We do the initializing just once from here, because according to python C API documentation:
	// > Some extensions may not work properly if their initialization routine is called more than 
	// > once; this can happen if an application calls Py_Initialize() and Py_FinalizeEx() more 
	// > than once.
	if (init_count > 0) {
		// Python is already initialized, so we just increase the count and return.
		return TRUE;
	}

	TCHAR exe_path[MAX_PATH];
	// Handle NULL instead of hInst indicates to look 
	// for the path of the exe instead of the current dll.
	GetModuleFileName(NULL, exe_path, MAX_PATH - 1);

	// python 実行準備
	// prepare to run python
	PyStatus status;
	PyConfig config;
	PyConfig_InitPythonConfig(&config);

	// optional but recommended
	status = PyConfig_SetString(&config, &config.program_name, exe_path);
	if (PyStatus_Exception(status)) {
		throw(new exception());
	}

	status = Py_InitializeFromConfig(&config);
	if (PyStatus_Exception(status)) {
		throw(new exception());
	}
	PyConfig_Clear(&config);

	PyObject* sys = PyImport_ImportModule("sys");
	PyObject* sys_path = PyObject_GetAttrString(sys, "path");
	PyObject* dir = PyUnicode_DecodeFSDefault("./python/");
	PyObject* dir2 = PyUnicode_DecodeFSDefault(".");
	PyObject* dir3 = PyUnicode_FromWideChar(ini_dir.c_str(),-1);

	PyList_Append(sys_path, dir);
	PyList_Append(sys_path, dir2);
	PyList_Append(sys_path, dir3);
	Py_DECREF(dir3);
	Py_DECREF(dir2);
	Py_DECREF(dir);
	Py_DECREF(sys_path);
	Py_DECREF(sys);

	init_count++;

	return TRUE;
}

/*
 * dll_uninitialize
 */
static BOOL dll_uninitialize(void)
{
	return TRUE;
}

/*
 * cb_python0_tool
 * Tool processing
 *
 *	argument:
 *		hWnd - the calling window
 *		tei - tool execution information
 *		tdi - item information for tools
 *
 *	Return value:
 *		TOOL_
 */
__declspec(dllexport) int CALLBACK cb_python0(const HWND hWnd, TOOL_EXEC_INFO* tei, TOOL_DATA_INFO* tdi)
{
	wstring mod_func;
	wstring mod;
	wstring func;
	// If command line is given get module name and function name from there
	if (tei->cmd_line != nullptr && *tei->cmd_line != TEXT('\0')) {
		mod_func.assign(tei->cmd_line);
		size_t pos = mod_func.find(TEXT(":"));
		if (pos != wstring::npos) {
			mod = mod_func.substr(0, pos);
			func = mod_func.substr(pos + 1);
			if (func.empty()) {
				func = TEXT("main");
			}
		} 
		else {
			mod = mod_func;
			func = TEXT("main");
		}
	} 
	else {
		mod = TEXT("sc2");
		func = TEXT("main");
	}

	DATA_INFO* di = nullptr;
	int ret = TOOL_SUCCEED;

	for (; tdi != NULL; tdi = tdi->next) {
		if (SendMessage(hWnd, WM_ITEM_CHECK, 0, (LPARAM)tdi->di) == -1) {
			continue;
		}

#ifdef UNICODE
		di = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_GET_FORMAT_TO_ITEM, (WPARAM)TEXT("UNICODE TEXT"), (LPARAM)tdi->di);
#else
		di = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_GET_FORMAT_TO_ITEM, (WPARAM)TEXT("TEXT"), (LPARAM)tdi->di);
#endif
		if (di != NULL && di->data != NULL) {
			ret |= run_python(di, mod.c_str(), func.c_str());
		}
	}


	return ret;
}

/*
 * cb_python0_property
 * Show properties
 *
 *	argument:
 *		hWnd - handle of the options window
 *		tei - tool execution information
 *
 *	Return value:
 *		TRUE - with properties
 *		FALSE - no property
 */
__declspec(dllexport) BOOL CALLBACK cb_python0_property(const HWND hWnd, TOOL_EXEC_INFO* tei)
{
	DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_PYTHON0), hWnd, dlg_python0, (LPARAM)tei);
	return TRUE;
}

/*
 *  python実行して結果をテキストでクリップボードへ
 *  execute the python script and copy the result to the clipboard as text.
 */
static int run_python(DATA_INFO* di, const TCHAR* str_script, const TCHAR* str_func)
{
	HANDLE ret;
	BYTE* to_mem;
	TCHAR* r;
	TCHAR* data_in;
	TCHAR* data_out = nullptr;
	int size;

	// コピー元ロック
	// lock the source
	if ((data_in = (TCHAR*) GlobalLock(di->data)) == NULL) {
		return TOOL_ERROR;
	}

	// get the lenhth of the source
	size = lstrlen(data_in) + 1;

	PyObject* runscriptobj = PyUnicode_FromWideChar(str_script, -1);

	if (runscriptobj == NULL) {
		GlobalUnlock(di->data);
		return TOOL_ERROR;
	}

	PyObject* pModule = PyImport_Import(runscriptobj);
	if (pModule == NULL) {
		Py_DECREF(runscriptobj);
		GlobalUnlock(di->data);
		return TOOL_ERROR;
	}

	Py_DECREF(runscriptobj);

	wstring func_name;
	func_name.assign(str_func);
	PyObject* pFunc = PyObject_GetAttrString(pModule, utf16_to_utf8(func_name).c_str());
	// pFunc is a new reference 

	if (pFunc == nullptr || !PyCallable_Check(pFunc)) {
		Py_XDECREF(pFunc);
		Py_DECREF(pModule);
		GlobalUnlock(di->data);
		return TOOL_ERROR;
	}

	// set the one and only function parameter
	PyObject* pArgs = PyTuple_New(1);
	PyObject* pValue = PyUnicode_FromWideChar(data_in, lstrlen(data_in));
	PyTuple_SetItem(pArgs, 0, pValue);

	// python 実行
	// execute python
	PyObject* pResult = PyObject_CallObject(pFunc, pArgs);
	Py_XDECREF(pArgs);
	// pValue is now owned by pArgs, so we must not DECREF it separately
	Py_XDECREF(pFunc);
	Py_XDECREF(pModule);
	if (pResult != NULL) {
		Py_ssize_t resultLen = 0;
		if (PyUnicode_Check(pResult))
			data_out = PyUnicode_AsWideCharString(pResult, &resultLen);
		Py_DECREF(pResult);
	}

	if (data_out == nullptr) {
		GlobalUnlock(di->data);
		return TOOL_ERROR; 
	}

	// サイズを取得
	// get the size
	size = lstrlen((TCHAR*)data_out) + 1;

	// コピー先確保
	// allocate memory for the copy destination
	if ((ret = GlobalAlloc(GHND, sizeof(TCHAR) * size)) == NULL) {
		GlobalUnlock(di->data);
		PyMem_Free(data_out);
		return TOOL_ERROR;
	}

	// コピー先ロック
	// lock the copy destination
	if ((to_mem = (BYTE*) GlobalLock(ret)) == NULL) {
		GlobalFree(ret);
		GlobalUnlock(di->data);
		PyMem_Free(data_out);
		return TOOL_ERROR;
	}

	r = lstrcpy((TCHAR*)to_mem, data_out);
	// This memory must be freed again!
	PyMem_Free(data_out);

	GlobalUnlock(ret);
	GlobalUnlock(di->data);

	GlobalFree(di->data);
	di->data = ret;
	di->size = sizeof(TCHAR) * size;

	return TOOL_DATA_MODIFIED;
}

static bool init_python0(HWND hDlg, const TOOL_EXEC_INFO* tei = nullptr)
{
	// initialization from cmd_line
	wstring s;
	if (tei && tei->cmd_line)
		s.assign(tei->cmd_line);

	size_t nsep = s.find(TEXT(":"));
	if (nsep != wstring::npos && nsep > 0) {
		edit_module_name = s.substr(0, nsep);
		edit_function_name = s.substr(nsep + 1);
	}
	else {
		edit_module_name.clear();
		edit_function_name.clear();
	}

	// set the GUI controls from edit_module_name and edit_function_name
	wndctrl(hDlg, IDC_EDIT_PYTHON_MODULE).set_window_text(edit_module_name);

	// fill the combo box with the functions of the module
	wndcombobox cb_py_func(hDlg, IDC_COMBO_PYFUNC_SELECT);
	fill_python_functions(cb_py_func, edit_module_name);
	if (cb_py_func.select_string(edit_function_name) < 0)
		cb_py_func.select_index(-1); // clear combobox

	return true;
}

static wstring get_python_module_name(const HWND hWnd)
{
	wstring fname;
	OPENFILENAME of;
	wchar_t file_name[MAX_PATH];
	std::filesystem::path fpath;

	lstrcpy(file_name, L".py\0");

	try
	{
		ZeroMemory(&of, sizeof(OPENFILENAME));
		of.lStructSize = sizeof(OPENFILENAME);
		of.hInstance = hInst;
		of.hwndOwner = hWnd;
		of.lpstrTitle = L"Python Module";
		of.lpstrInitialDir = app_path ? app_path : L".";
		of.lpstrFilter = L"Python files\0*.py\0\0";
		of.lpstrDefExt = L"py";
		of.nFilterIndex = 1;
		of.lpstrFile = file_name;
		of.nMaxFile = MAX_PATH - 1;
		of.Flags = OFN_FILEMUSTEXIST;

		if (GetOpenFileName(&of) == FALSE) {
			return fname;
		}

		fpath.assign(of.lpstrFile);
		//is_character_file(fpath) && 
		if (fpath.has_stem() && (fpath.extension() == L".py" || fpath.extension().empty())) {
			fname = fpath.stem().wstring();
		}
	}
	catch (...)
	{
		fname.clear();
		return fname;
	}

	return fname;
}

static void fill_python_functions(wndcombobox& cb, const wstring module_name)
{
	cb.reset_content();
	if (module_name.length() <= 0) 
		return;

	PyObject* runscriptobj = PyUnicode_FromWideChar(module_name.c_str(), -1);

	if (runscriptobj == NULL)
		return;

	PyObject* pModule = PyImport_Import(runscriptobj);
	Py_DECREF(runscriptobj);

	if (pModule == NULL) {
		return;
	}

	// 1. Get the list of all attributes in the module (equivalent to dir(module))
	PyObject* pDirList = PyObject_Dir(pModule);
	Py_XDECREF(pModule);

	if (!pDirList) {
		return;
	}

	// 2. Iterate through the attribute list
	Py_ssize_t listSize = PyList_Size(pDirList);
	for (Py_ssize_t i = 0; i < listSize; i++) {
		PyObject* pAttrName = PyList_GetItem(pDirList, i); // Borrowed reference

		// 3. Get the actual attribute object from the module
		PyObject* pAttrValue = PyObject_GetAttr(pModule, pAttrName);
		if (!pAttrValue) {
			PyErr_Clear(); // Skip attributes that can't be accessed
			continue;
		}

		// 4. Check if the attribute is callable (a function, method, or class)
		if (PyCallable_Check(pAttrValue)) {
			// Convert the Python string name to a C string
			wchar_t* func_name = PyUnicode_AsWideCharString(pAttrName, 0);
			if (func_name) {
				cb.add_string(func_name);
				PyMem_Free(func_name);
			}
		}

		// Clean up the value reference
		Py_DECREF(pAttrValue);
	}

	// Clean up the list reference
	Py_DECREF(pDirList);
}

// dialog for python call
static BOOL CALLBACK dlg_python0(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (hDlg == NULL)
		return FALSE;

	switch (uMsg) {
	case WM_INITDIALOG:
		dlg_tei = (TOOL_EXEC_INFO*)lParam;
		return (BOOL)init_python0(hDlg, dlg_tei);
		break;

	case WM_CLOSE:
		EndDialog(hDlg, FALSE);
		break;

	case EN_KILLFOCUS:
		if (LOWORD(wParam) == IDC_EDIT_PYTHON_MODULE) {
			wstring module_name = wndctrl(hDlg, IDC_EDIT_PYTHON_MODULE).get_window_text();
			if (module_name != edit_module_name) {
				wndcombobox cb_py_func(hDlg, IDC_COMBO_PYFUNC_SELECT);
				fill_python_functions(cb_py_func, module_name);
				if (cb_py_func.select_string(edit_function_name) < 0) {
					cb_py_func.select_index(-1); // clear combobox
				}
			}
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
			case IDOK:
			{
				// get edit_module_name and edit_function_name from their GUI controls.
				wstring module_name = wndctrl(hDlg, IDC_EDIT_PYTHON_MODULE).get_window_text();

				wstring combo_function_name = 
					wndcombobox(hDlg, IDC_COMBO_PYFUNC_SELECT).get_current_selection_text();

				if (combo_function_name.length() <= 0) {
					// show message that no valid function name is selected
					break;
				}

				edit_module_name = module_name;
				edit_function_name = combo_function_name;

				// Separate the two parts.
				wstring s = edit_module_name + wstring(TEXT(":")) + edit_function_name;

				// By returning the content to tei->cmdline we enable CLCLSet.exe 
				// to store edit_module_name and edit_function_name to clcl.ini.
				// Thus there can be several tool items of python0 with different
				// module and function names.
				if (dlg_tei && dlg_tei->cmd_line)
					::mem_free((void**)&dlg_tei->cmd_line);

				if (dlg_tei)
					dlg_tei->cmd_line = ::alloc_copy(s.c_str());

				EndDialog(hDlg, TRUE);
				break;
			}

			case IDCANCEL:
			{
				EndDialog(hDlg, FALSE);
				break;
			}

			case EN_KILLFOCUS:
			{
				if (LOWORD(wParam) == IDC_EDIT_PYTHON_MODULE) {
					wstring module_name = wndctrl(hDlg, IDC_EDIT_PYTHON_MODULE).get_window_text();
					if (module_name != edit_module_name) {
						wndcombobox cb_py_func(hDlg, IDC_COMBO_PYFUNC_SELECT);
						fill_python_functions(cb_py_func, module_name);
						if (cb_py_func.select_string(edit_function_name) < 0) {
							cb_py_func.select_index(-1); // clear combobox
						}
					}
				}
				break;
			}

			case IDC_BUTTON_PYMODULE_SELECT:
			{
				wndctrl edit_py_module(hDlg, IDC_EDIT_PYTHON_MODULE);
				wstring prev_module_name = edit_py_module.get_window_text();
				wstring module_name = get_python_module_name(hDlg);
				if (module_name.empty()) {
					// show message that no valid module name is selected
					break;
				} 
				else if (module_name != prev_module_name) {
					edit_py_module.set_window_text(module_name);
					wndcombobox cb_py_func(hDlg, IDC_COMBO_PYFUNC_SELECT);
					fill_python_functions(cb_py_func, module_name);
					if (cb_py_func.select_string(edit_function_name) < 0) {
						cb_py_func.select_index(-1); // clear combobox
					}
				}

				break;
			}

			/*
			case CBN_DROPDOWN:
				if (LOWORD(wParam) == IDC_COMBO_PYFUNC_SELECT) {
					wstring module_name = wndctrl(hDlg, IDC_EDIT_PYTHON_MODULE).get_window_text();
					fill_python_functions(module_name);
				}
				break;
			*/
		}

		break;

	default:
		return FALSE;
	}
	return TRUE;
}

/* End of source */

