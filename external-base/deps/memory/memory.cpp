#include "memory.h"
#include <ntstatus.h>

pNtReadVirtualMemory NtReadVirtualMemory = nullptr;
pNtWriteVirtualMemory NtWriteVirtualMemory = nullptr;

bool init_nt_functions()
{
    if (NtReadVirtualMemory && NtWriteVirtualMemory)
    {
        return true;
    }

    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll)
    {
        return false;
    }

    NtReadVirtualMemory = reinterpret_cast<pNtReadVirtualMemory>(
        GetProcAddress(ntdll, "NtReadVirtualMemory"));

    NtWriteVirtualMemory = reinterpret_cast<pNtWriteVirtualMemory>(
        GetProcAddress(ntdll, "NtWriteVirtualMemory"));

    return NtReadVirtualMemory && NtWriteVirtualMemory;
}

std::uint32_t c_memory::find_process_id(const std::wstring &process_name)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    if (!Process32FirstW(snapshot, &entry))
    {
        CloseHandle(snapshot);
        return 0;
    }

    do
    {
        if (_wcsicmp(entry.szExeFile, process_name.c_str()) == 0)
        {
            CloseHandle(snapshot);
            return entry.th32ProcessID;
        }
    } while (Process32NextW(snapshot, &entry));

    CloseHandle(snapshot);
    return 0;
}

std::uint64_t c_memory::find_module_address(const std::wstring &module_name)
{
    if (!process_id || !process_handle)
        return 0;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process_id);
    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    MODULEENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    if (!Module32FirstW(snapshot, &entry))
    {
        CloseHandle(snapshot);
        return 0;
    }

    do
    {
        if (_wcsicmp(entry.szModule, module_name.c_str()) == 0)
        {
            CloseHandle(snapshot);
            return reinterpret_cast<std::uint64_t>(entry.modBaseAddr);
        }
    } while (Module32NextW(snapshot, &entry));

    CloseHandle(snapshot);
    return 0;
}

bool c_memory::attach_to_process(const std::wstring &process_name)
{
    init_nt_functions();

    process_id = find_process_id(process_name);
    if (!process_id)
        return false;

    process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id);
    if (!process_handle)
        return false;

    base_address = find_module_address(L"BloodStrike.exe");

    return process_handle != nullptr;
}

std::string c_memory::read_string(std::uint64_t address)
{
    std::string result;
    char buffer[256]{};

    for (int i = 0; i < 255; i += sizeof(buffer) - 1)
    {
        ULONG read = 0;
        NTSTATUS status = NtReadVirtualMemory(
            process_handle,
            reinterpret_cast<void *>(address + i),
            buffer,
            sizeof(buffer) - 1,
            &read);

        if (!NT_SUCCESS(status) || read == 0)
            break;

        buffer[read] = '\0';
        result += buffer;

        if (read < sizeof(buffer) - 1)
            break;
    }

    return result;
}

std::unique_ptr<c_memory> memory = std::make_unique<c_memory>();