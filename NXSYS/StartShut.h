#ifdef NXSYSMac

int StartUpNXSYS (void* hInstance, void* window, const char * initial_layout_name, const char* initial_demo_file,
                  int nCmdShow);

#else


int StartUpNXSYS (HINSTANCE hInstance, HWND window, const char * initial_layout_name, const char* initial_demo_file,
                  int nCmdShow);
#endif

void ShutDownNXSYS();
bool GetLayout(const char * layoutName, bool Review);
