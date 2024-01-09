std::wstring s2ws(const std::string& str) {
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    wchar_t* wstr = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wstr, len);
    std::wstring result(wstr);
    delete[] wstr;
    return result;
}

HANDLE connectToNamedPipe(const std::wstring& pipeName) {
    HANDLE pipe = CreateFile(
        pipeName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );
    return pipe;
}

void sendToNamedPipe(HANDLE pipe, const std::wstring& message){
    DWORD bytesWritten;
    WriteFile(pipe, message.c_str(), static_cast<DWORD>(message.size()) * sizeof(wchar_t), &bytesWritten, NULL);
}

struct subscriber {
    std::string name;
    HANDLE pipe;
};