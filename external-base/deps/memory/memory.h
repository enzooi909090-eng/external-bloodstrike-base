#pragma once

#include <windows.h>
#include <TlHelp32.h>
#include <string>
#include <memory>
#include <cstdint>

typedef LONG NTSTATUS;
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

typedef NTSTATUS(NTAPI *pNtReadVirtualMemory)(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    PVOID Buffer,
    ULONG NumberOfBytesToRead,
    PULONG NumberOfBytesRead);

typedef NTSTATUS(NTAPI *pNtWriteVirtualMemory)(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    PVOID Buffer,
    ULONG NumberOfBytesToWrite,
    PULONG NumberOfBytesWritten);

extern pNtReadVirtualMemory NtReadVirtualMemory;
extern pNtWriteVirtualMemory NtWriteVirtualMemory;

class c_memory
{
public:
    c_memory() = default;
    ~c_memory() = default;

    std::uint32_t find_process_id(const std::wstring &process_name);
    std::uint64_t find_module_address(const std::wstring &module_name);
    bool attach_to_process(const std::wstring &process_name);

    std::string read_string(std::uint64_t address);

    template <typename T>
    T read(std::uint64_t address)
    {
        T buffer{};
        if (NtReadVirtualMemory)
        {
            NtReadVirtualMemory(
                process_handle,
                reinterpret_cast<void *>(address),
                &buffer,
                sizeof(T),
                nullptr);
        }
        return buffer;
    }

    template <typename T>
    void write(std::uint64_t address, T value)
    {
        if (NtWriteVirtualMemory)
        {
            NtWriteVirtualMemory(
                process_handle,
                reinterpret_cast<void *>(address),
                &value,
                sizeof(T),
                nullptr);
        }
    }

    std::string ReadCUtlSymbolLarge(uintptr_t address);

    std::uint32_t get_process_id() const { return process_id; }
    std::uint64_t get_module_address() const { return base_address; }
    HANDLE get_process_handle() const { return process_handle; }

private:
    std::uint32_t process_id{};
    std::uint64_t base_address{};
    HANDLE process_handle{};
};

extern std::unique_ptr<c_memory> memory;

bool init_nt_functions();